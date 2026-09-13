#include "player_list.h"
#include "game.h"

#include <cmath>

static float LerpAngleDeg(float a, float b, float t)
{
    float diff = fmodf(b - a + 180.0f, 360.0f);
    if (diff < 0.0f)
    {
        diff += 360.0f;
    }
    diff -= 180.0f;
    return a + diff * t;
}

ClientPlayerState* PlayerList::AddPlayer(uint64_t playerId, bool local)
{
    auto player = local ? std::make_unique<ClientLocalPlayerState>() : std::make_unique<ClientPlayerState>();

    ClientPlayerState* newPlayer = Players.insert_or_assign(playerId, std::move(player)).first->second.get();
    newPlayer->PlayerID = playerId;

    if (local)
        LocalPlayer = static_cast<ClientLocalPlayerState*>(newPlayer);

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

    if (InterpStartHistoryIndex == 0 || TransformHistory.empty() || TransformHistory.begin()->first > InterpStartHistoryIndex)
    {
        // nothing to interpolate
        return;
    }

    auto start = TransformHistory.find(InterpStartHistoryIndex);
    auto end = TransformHistory.find(InterpEndHistoryIndex);


    // we can't find the history items
    if (start == TransformHistory.end() && end == TransformHistory.end())
    {
        return;
    }

    // only one is valid, so just use that
    if (start == TransformHistory.end() || end == TransformHistory.end())
    {
        if (start == TransformHistory.end())
        {
            Transform = end->second;
        }
        else
        {
            Transform = start->second;
        }

        return;
    }

    // we have two valid interpolation positions, smooth that sucker out
    float param = Clamp(LastTickTime * kDefaultTickRate, 0.0f, 1.0f);

    Transform.Position = Vector2Lerp(start->second.Position, end->second.Position, param);
    Transform.Rotation[0] = LerpAngleDeg(start->second.Rotation[0], end->second.Rotation[0], param);
    Transform.Rotation[1] = LerpAngleDeg(start->second.Rotation[1], end->second.Rotation[1], param);

    LastTickTime += deltaTime;
}

void ClientPlayerState::UpdateForTick(uint64_t currentTick)
{
    if (IsLocalPlayer)
    {
        return;
    }

    LastTickTime = 0;
    InterpStartHistoryIndex = (currentTick >= RemotePlayerHistoryOffset) ? (currentTick - RemotePlayerHistoryOffset) : 0;
    InterpEndHistoryIndex = InterpStartHistoryIndex + 1;

    uint64_t maxHistory = 32;
    if (currentTick > maxHistory)
    {
        uint64_t oldest = currentTick - maxHistory;
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
    static PlayerMovementRules defaultRules;
    PlayerTransform replayedTransform = transform;
    for (auto replayIt = unackedIt; replayIt != InputHistory.end(); ++replayIt)
    {
        auto newPos = UpdatePlayerTransform(replayedTransform, replayIt->second, 1.0f / kDefaultTickRate, defaultRules);
        BoundingCircle bounds = { replayedTransform.Position, 10 };

        if (World)
            newPos = World->Collide(replayedTransform.Position, newPos, 1, bounds);

        replayedTransform.Position = newPos;
    }

    Vector2 delta = replayedTransform.Position - Transform.Position;
    float len = Vector2Length(delta);
    if (len > 0.1f)
    {
        GetLogger().Log(LogLevel::Warning, "Local player prediction off by %f (reconciling to tick %llu)", len, tick);
        Transform = replayedTransform;
    }

    while (InputHistory.size() > 128)
    {
        InputHistory.erase(InputHistory.begin());
    }
}
