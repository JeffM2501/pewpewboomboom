#include "player_list.h"

PlayerState* PlayerList::AddPlayer(uint64_t playerId)
{
    PlayerState* newPlayer = Players.insert_or_assign(playerId, std::move(std::make_unique<PlayerState>())).first->second.get();
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
