#pragma once
#include "external/fix_win32_compatibility.h"
#include "enet.h"

#include "player_state.h"
#include <unordered_map>
#include <functional>

namespace ServerPlayerList
{
    class ServerPlayer : public PlayerState
    {
    public:
        bool IsRobot = false;
        std::vector<std::function<void(ServerPlayer&)>> UpdateFunctions;

        ServerPlayer(ENetPeer* peer);

        void Update()
        {
            // TODO make this a list?
            for (auto& func : UpdateFunctions)
                func(*this);
        }
    };

    std::unordered_map<uint64_t, ServerPlayer> &GetPlayerList();

    ServerPlayer& GetPlayer(ENetPeer* peer);
    bool PlayerExists(ENetPeer* peer);
    bool RemovePlayer(ENetPeer* peer);

    ServerPlayer& AddRobotPlayer();

    bool Empty(bool includeRobots = false);

    void DoForEachPlayer(std::function<void(ServerPlayer&)> function, bool includeRobots = false, uint64_t except = uint64_t(-1));
}
