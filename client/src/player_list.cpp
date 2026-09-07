#include "player_list.h"
#include "game.h"

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
    Transform = transform; // let the update handle this 
    TransformHistory[tick] = transform;
}

void ClientPlayerState::UpdateInterpolatedTransform(float deltaTime)
{

}

void ClientPlayerState::UpdateForTick(uint64_t currentTick)
{

}

void ClientLocalPlayerState::AddServerStateUpdate(uint64_t tick, PlayerTransform& transform)
{
    // go find the input for this tick
    bool foundInput = false;
    for (auto itr = InputHistory.begin(); itr != InputHistory.end();)
    {
        if (itr->first <= tick)
        {
            itr = InputHistory.end();
        }
        else
        {
            foundInput = true;
            break;
        }
    }
    PlayerTransform newTransform = transform;
    if (!foundInput)
    {
        return;
    }

    static PlayerMovementRules defaultRules;

    for (auto& [futureTick, input] : InputHistory)
    {
        UpdatePlayerTransform(newTransform, input, 1.0f / kDefaultTickRate, defaultRules);
    }

    Vector2 delta = newTransform.Position - transform.Position;

    float len = Vector2Length(delta);
    if (len > 0.1f)
    {
        GetLogger().Log(LogLevel::Warning, "Local player prediction off by %f", len);
    }
}
