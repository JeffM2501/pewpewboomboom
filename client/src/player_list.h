#pragma once

#include "player_state.h"

#include <unordered_map>
#include <memory>
#include <functional>

class ClientPlayerState : public PlayerState
{
public:
    bool IsLocalPlayer = false;
};

class PlayerList
{
private:
    std::unordered_map<uint64_t, std::unique_ptr<ClientPlayerState>> Players;
public:

    ClientPlayerState* AddPlayer(uint64_t playerId);
    void RemovePlayer(uint64_t playerId);
    void UpdatePlayerInfo(uint64_t playerId);

    void DoForEachPlayer(std::function<void(ClientPlayerState*)> callback);

    size_t Size() { return Players.size(); }
};