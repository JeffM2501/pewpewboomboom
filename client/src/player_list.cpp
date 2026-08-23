#include "player_list.h"

ClientPlayerState* PlayerList::AddPlayer(uint64_t playerId)
{
    ClientPlayerState* newPlayer = Players.insert_or_assign(playerId, std::move(std::make_unique<ClientPlayerState>())).first->second.get();
    newPlayer->PlayerID = playerId;

    return newPlayer;
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
        callback(state.get());
    }
}
