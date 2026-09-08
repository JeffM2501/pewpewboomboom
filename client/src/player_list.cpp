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

static Vector2 CubicHermiteSpline(Vector2 p0, Vector2 v0, Vector2 p1, Vector2 v1, float t, float dt)
{
    float t2 = t * t;
    float t3 = t2 * t;

    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h10 = t3 - 2.0f * t2 + t;
    float h01 = -2.0f * t3 + 3.0f * t2;
    float h11 = t3 - t2;

    Vector2 m0 = v0 * dt;
    Vector2 m1 = v1 * dt;

    return (p0 * h00) + (m0 * h10) + (p1 * h01) + (m1 * h11);
}

void ClientPlayerState::UpdateInterpolatedTransform(double renderTick, float deltaTime)
{
    if (IsLocalPlayer)
    {
        return;
    }

    if (TransformHistory.empty())
    {
        return;
    }

    // If renderTick is behind the oldest snapshot in history, clamp to the oldest snapshot
    if (renderTick <= static_cast<double>(TransformHistory.begin()->first))
    {
        Transform = TransformHistory.begin()->second;
        return;
    }

    // Find upper bracket (first snapshot with tick > renderTick)
    uint64_t targetTickFloor = static_cast<uint64_t>(renderTick);
    auto upper = TransformHistory.upper_bound(targetTickFloor);

    // If renderTick is ahead of all received snapshots, extrapolate smoothly from the newest snapshot
    if (upper == TransformHistory.end())
    {
        auto newest = std::prev(TransformHistory.end());
        double overTicks = renderTick - static_cast<double>(newest->first);
        float dt = static_cast<float>(overTicks / static_cast<double>(kDefaultTickRate));

        Transform = newest->second;
        Transform.Position = Transform.Position + (newest->second.Velocity * dt);
        return;
    }

    // Lower bracket is immediately preceding the upper bracket
    auto lower = std::prev(upper);

    uint64_t t0 = lower->first;
    uint64_t t1 = upper->first;

    if (t1 <= t0)
    {
        Transform = lower->second;
        return;
    }

    // Normalized interpolation factor between surrounding snapshots
    float param = static_cast<float>((renderTick - static_cast<double>(t0)) / static_cast<double>(t1 - t0));
    param = Clamp(param, 0.0f, 1.0f);

    float intervalDt = static_cast<float>(t1 - t0) / static_cast<float>(kDefaultTickRate);
    Transform.Position = CubicHermiteSpline(lower->second.Position, lower->second.Velocity, upper->second.Position, upper->second.Velocity, param, intervalDt);
    Transform.Rotation[0] = LerpAngleDeg(lower->second.Rotation[0], upper->second.Rotation[0], param);
    Transform.Rotation[1] = LerpAngleDeg(lower->second.Rotation[1], upper->second.Rotation[1], param);
    Transform.Velocity = Vector2Lerp(lower->second.Velocity, upper->second.Velocity, param);
}

void ClientPlayerState::UpdateForTick(uint64_t currentTick)
{
    if (IsLocalPlayer)
    {
        return;
    }

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
        UpdatePlayerTransform(replayedTransform, replayIt->second, 1.0f / kDefaultTickRate, defaultRules);
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
