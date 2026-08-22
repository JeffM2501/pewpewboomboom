#pragma once

#include "player_state.h"

#include <unordered_map>
#include <memory>

class PlayerList
{
private:
    std::unordered_map<uint64_t, std::unique_ptr<PlayerState>> Players;
public:

    PlayerState* AddPlayer(uint64_t playerId);
    void RemovePlayer(uint64_t playerId);
    void UpdatePlayerInfo(uint64_t playerId);
};