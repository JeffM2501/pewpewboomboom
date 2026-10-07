#include "player_list.h"
#include "game.h"
#include "collisions.h"
#include "raymath.h"

#include <cmath>

ClientPlayerState* PlayerList::AddPlayer(uint64_t playerId, bool local)
{
    auto player = local ? std::make_unique<ClientLocalPlayerState>() : std::make_unique<ClientPlayerState>();

    ClientPlayerState* newPlayer = Players.insert_or_assign(playerId, std::move(player)).first->second.get();
    newPlayer->PlayerID = playerId;

    if (local)
    {
        LocalPlayer = static_cast<ClientLocalPlayerState*>(newPlayer);
        LocalPlayer->OwnerList = this;
    }

    return newPlayer;
}

ClientPlayerState* PlayerList::GetPlayer(uint64_t playerId)
{
    auto itr = Players.find(playerId);
    if (itr == Players.end())
        return nullptr;

    return itr->second.get();
}

void PlayerList::RemovePlayer(uint64_t playerId)
{
    auto itr = Players.find(playerId);
    if (itr == Players.end())
        return;

    Players.erase(itr);
}

void PlayerList::UpdatePlayerInfo(uint64_t playerId)
{

}

void PlayerList::DoForEachPlayer(std::function<void(ClientPlayerState*)> callback)
{
    for (auto& [id, state] : Players)
    {
        if (state != nullptr)
            callback(state.get());
    }
}

void ClientPlayerState::AddServerStateUpdate(uint64_t tick, PlayerTransform& transform)
{
    if (IsLocalPlayer)
    {
        TransformHistory[tick] = transform;
        return;
    }

    if (!InitializedInterp)
    {
        Transform = transform;
        SmoothedTransform = transform;
        PositionError = { 0.0f, 0.0f };
        RotationError[0] = 0.0f;
        RotationError[1] = 0.0f;
        InitializedInterp = true;
    }
    else
    {
        // Measure mismatch between current visual display (SmoothedTransform) and new authoritative state
        Vector2 posErr = Vector2Subtract(SmoothedTransform.Position, transform.Position);
        float rot0Err = AngleDiffDeg(transform.Rotation[0], SmoothedTransform.Rotation[0]);
        float rot1Err = AngleDiffDeg(transform.Rotation[1], SmoothedTransform.Rotation[1]);

        if (Vector2Length(posErr) > 8.0f)
        {
            // Teleport or extreme correction: snap directly
            PositionError = { 0.0f, 0.0f };
            RotationError[0] = 0.0f;
            RotationError[1] = 0.0f;
        }
        else
        {
            PositionError = posErr;
            RotationError[0] = rot0Err;
            RotationError[1] = rot1Err;
        }

        Transform = transform;
    }

    TransformHistory[tick] = transform;
}

void ClientPlayerState::UpdateInterpolatedTransform(float deltaTime)
{
    if (IsLocalPlayer)
    {
        return;
    }

    if (!InitializedInterp)
    {
        return;
    }

    // 1. Advance simulation prediction using current velocity and angular velocity
    Transform = PredictTransform(Transform, deltaTime);

    // 2. Exponentially decay smoothing error offsets towards zero
    float decay = expf(-15.0f * deltaTime);
    PositionError = Vector2Scale(PositionError, decay);
    RotationError[0] *= decay;
    RotationError[1] *= decay;

    if (Vector2Length(PositionError) < 0.005f)
    {
        PositionError = { 0.0f, 0.0f };
    }
    if (fabsf(RotationError[0]) < 0.05f)
    {
        RotationError[0] = 0.0f;
    }
    if (fabsf(RotationError[1]) < 0.05f)
    {
        RotationError[1] = 0.0f;
    }

    // 3. Compute smoothed render transform for visual display
    SmoothedTransform = Transform;
    SmoothedTransform.Position = Vector2Add(Transform.Position, PositionError);
    SmoothedTransform.Rotation[0] = fmodf(Transform.Rotation[0] + RotationError[0], 360.0f);
    if (SmoothedTransform.Rotation[0] < 0.0f)
    {
        SmoothedTransform.Rotation[0] += 360.0f;
    }
    SmoothedTransform.Rotation[1] = fmodf(Transform.Rotation[1] + RotationError[1], 360.0f);
    if (SmoothedTransform.Rotation[1] < 0.0f)
    {
        SmoothedTransform.Rotation[1] += 360.0f;
    }
}

void ClientPlayerState::UpdateForTick(uint64_t currentTick)
{
    if (IsLocalPlayer)
    {
        return;
    }

    // Prune history older than 64 ticks (~1 second) behind current playback
    uint64_t baseTick = (RenderTick > 0.0) ? uint64_t(RenderTick) : currentTick;
    uint64_t maxHistory = 64;
    if (baseTick > maxHistory)
    {
        uint64_t oldest = baseTick - maxHistory;
        for (auto itr = TransformHistory.begin(); itr != TransformHistory.end();)
        {
            if (itr->first < oldest)
            {
                itr = TransformHistory.erase(itr);
            }
            else
            {
                break;
            }
        }
    }
}

void ClientLocalPlayerState::AddServerStateUpdate(uint64_t tick, PlayerTransform& transform)
{
    if (tick == 0)
    {
        return;
    }
    auto it = InputHistory.find(tick);
    if (it == InputHistory.end())
    {
        return;
    }

    // Prune acknowledged inputs older than tick
    for (auto pruneIt = InputHistory.begin(); pruneIt != it;)
    {
        pruneIt = InputHistory.erase(pruneIt);
    }

    // Remove the acknowledged input at tick and retrieve iterator to unacknowledged inputs
    auto unackedIt = InputHistory.erase(it);

    // Replay unacknowledged inputs forward from the server's authoritative state
    PlayerTransform replayedTransform = transform;
    for (auto replayIt = unackedIt; replayIt != InputHistory.end(); ++replayIt)
    {
        uint64_t replayTick = replayIt->first;
        auto newPos = UpdatePlayerTransform(replayedTransform, replayIt->second, 1.0f / kDefaultTickRate, Rules);
        BoundingCircle bounds = { newPos, 10 };

        if (World)
        {
            newPos = World->Collide(replayedTransform.Position, newPos, CollisionRadius, bounds);
        }

        if (OwnerList)
        {
            OwnerList->DoForEachPlayer([&](ClientPlayerState* other)
            {
                if (other->IsLocalPlayer || other->IsDead)
                {
                    return;
                }

                PlayerTransform otherTransform = other->GetTransformAtTick(replayTick);
                Vector2 hitPoint;
                Vector2 hitNormal;
                IntersectCircleCylinder(otherTransform.Position, other->CollisionRadius, newPos, replayedTransform.Position, CollisionRadius, hitPoint, hitNormal);
            });

            if (World)
            {
                newPos = World->Collide(replayedTransform.Position, newPos, CollisionRadius, bounds);
            }
        }

        replayedTransform.Position = newPos;
    }

    Vector2 delta = replayedTransform.Position - Transform.Position;
    float len = Vector2Length(delta);
    if (len > 0.05f)
    {
        if (len > 2.0f)
        {
            GetLogger().Log(LogLevel::Warning, "Local player prediction off by %f (hard snapping to tick %llu)", len, tick);
            Transform = replayedTransform;
        }
        else
        {
            Transform.Position = Vector2Lerp(Transform.Position, replayedTransform.Position, 0.4f);
            Transform.Velocity = replayedTransform.Velocity;
            Transform.Rotation[0] = LerpAngleDeg(Transform.Rotation[0], replayedTransform.Rotation[0], 0.4f);
            Transform.Rotation[1] = replayedTransform.Rotation[1];
        }
    }

    while (InputHistory.size() > 128)
    {
        InputHistory.erase(InputHistory.begin());
    }
}
