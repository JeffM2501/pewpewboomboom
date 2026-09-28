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
   
    TransformHistory[tick] = transform;
}

void ClientPlayerState::UpdateInterpolatedTransform(float deltaTime)
{
    if (IsLocalPlayer)
    {
        return;
    }

    if (TransformHistory.empty())
    {
        return;
    }

    uint64_t latestTick = TransformHistory.rbegin()->first;
    double idealTargetTick = (latestTick >= RemotePlayerHistoryOffset) ? double(latestTick - RemotePlayerHistoryOffset) : double(latestTick);

    if (!InitializedInterp)
    {
        RenderTick = idealTargetTick;
        InitializedInterp = true;
    }

    // Advance render playback time continuously
    double deltaTicks = double(deltaTime) * double(kDefaultTickRate);

    // Gently adjust playback rate to prevent clock drift from starving or bloating the buffer
    double diff = RenderTick - idealTargetTick;
    double driftThreshold = double(kNetworkSendIntervalTicks) + 1.0;
    if (diff > driftThreshold)
    {
        // Render tick is drifting too close to latest received snapshot; slow down slightly
        deltaTicks *= 0.9;
    }
    else if (diff < -driftThreshold)
    {
        // Render tick is falling too far behind; speed up slightly
        deltaTicks *= 1.1;
    }

    // If there is an extreme hitch or disconnect jump, snap to target
    if (fabs(diff) > 15.0)
    {
        RenderTick = idealTargetTick;
    }
    else
    {
        RenderTick += deltaTicks;
    }

    if (TransformHistory.size() == 1)
    {
        Transform = TransformHistory.begin()->second;
        return;
    }

    // Clamp or extrapolate based on the available snapshot history window
    if (RenderTick < double(TransformHistory.begin()->first))
    {
        Transform = TransformHistory.begin()->second;
        return;
    }

    if (RenderTick >= double(latestTick))
    {
        // Extrapolate forward along last known velocity during temporary packet delays
        const auto& latestTransform = TransformHistory.rbegin()->second;
        double overshootTicks = RenderTick - double(latestTick);
        float overshootSec = float(overshootTicks) / float(kDefaultTickRate);

        // Cap extrapolation window to 0.15s (~9 ticks) to avoid wandering too far on dropouts
        if (overshootSec > 0.15f)
        {
            overshootSec = 0.15f;
        }

        Transform = latestTransform;
        Transform.Position = Vector2Add(latestTransform.Position, Vector2Scale(latestTransform.Velocity, overshootSec));
        return;
    }

    // Find the enclosing snapshots: lower <= RenderTick < upper
    auto upper = TransformHistory.upper_bound(uint64_t(RenderTick));
    if (upper == TransformHistory.begin())
    {
        Transform = TransformHistory.begin()->second;
        return;
    }

    auto lower = std::prev(upper);
    uint64_t tickSpan = upper->first - lower->first;
    float t = 0.0f;
    if (tickSpan > 0)
    {
        t = float(RenderTick - double(lower->first)) / float(tickSpan);
        t = Clamp(t, 0.0f, 1.0f);
    }

    Transform.Position = Vector2Lerp(lower->second.Position, upper->second.Position, t);
    Transform.Rotation[0] = LerpAngleDeg(lower->second.Rotation[0], upper->second.Rotation[0], t);
    Transform.Rotation[1] = LerpAngleDeg(lower->second.Rotation[1], upper->second.Rotation[1], t);
    Transform.Velocity = Vector2Lerp(lower->second.Velocity, upper->second.Velocity, t);
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
                if (other->IsLocalPlayer)
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
