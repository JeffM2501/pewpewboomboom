#pragma once
#include "external/fix_win32_compatibility.h"
#include "enet.h"

#include "player_state.h"
#include <unordered_map>
#include <functional>
#include <memory>

class NetworkManager;

namespace ServerPlayerList
{
    class ServerPlayerExtraData
    {
    public:
    };

    class ServerPlayer : public PlayerState
    {
    public:
        bool IsRobot = false;
        std::vector<std::function<void(ServerPlayer&, NetworkManager&)>> UpdateFunctions;

        uint64_t LastAckedInputTick = 0;

        ServerPlayer(ENetPeer* peer);

        void Update(NetworkManager &manager)
        {
            for (auto& func : UpdateFunctions)
                func(*this, manager);
        }

        std::unordered_map<uint64_t, std::unique_ptr<ServerPlayerExtraData>> ExtensionData;
    };

    std::unordered_map<uint64_t, ServerPlayer> &GetPlayerList();

    ServerPlayer& GetPlayer(ENetPeer* peer);
    ServerPlayer* GetPlayer(uint64_t playerID);
    bool PlayerExists(ENetPeer* peer);
    bool PlayerExists(uint64_t playerID);
    bool RemovePlayer(ENetPeer* peer);

    size_t GetPlayerCount(bool includeRobots = true);

    ServerPlayer& AddRobotPlayer();

    bool Empty(bool includeRobots = false);

    void DoForEachPlayer(std::function<void(ServerPlayer&)> function, bool includeRobots = false, uint64_t except = uint64_t(-1));
}
