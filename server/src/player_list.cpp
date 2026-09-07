#define NOMINMAX

#include "player_list.h"
#include <limits>
#include <mutex>

namespace ServerPlayerList
{
    ServerPlayer::ServerPlayer(ENetPeer* peer)
    {
        Peer = peer;
    }

    static std::unordered_map<uint64_t, ServerPlayer> Players;
    static uint32_t LastRobotPlayerID = 0;
    static uint32_t RobotPlayerIDMask = std::numeric_limits<uint32_t>::max();

    static std::recursive_mutex  PlayerListLock;

    std::unordered_map<uint64_t, ServerPlayer>& GetPlayerList()
    {
        return Players;
    }

    ServerPlayer* GetPlayer(uint64_t playerID)
    {
        std::lock_guard guard(PlayerListLock);

        auto itr = Players.find(playerID);
        if (itr == Players.end())
        {
            return nullptr;
        }

        ServerPlayer& player = itr->second;

        return &player;
    }

    bool PlayerExists(uint64_t playerID)
    {
        std::lock_guard guard(PlayerListLock);
      
        return Players.contains(playerID);
    }

    ServerPlayer& GetPlayer(ENetPeer* peer)
    {
        std::lock_guard guard(PlayerListLock);

        uint64_t playerId = uint64_t(peer->connectID);

        auto itr = Players.find(playerId);
        if (itr == Players.end())
        {
            itr = Players.emplace(peer->connectID, peer).first;
            itr->second.PlayerID = playerId;
        }

        ServerPlayer& player = itr->second;

        return player;
    }

    bool PlayerExists(ENetPeer* peer)
    {
        return PlayerExists(uint64_t(peer->connectID));
    }

    bool RemovePlayer(ENetPeer* peer)
    {
        std::lock_guard guard(PlayerListLock);

        auto itr = Players.find(peer->connectID);
        if (itr == Players.end())
            return false;

        Players.erase(itr);
        return true;
    }

    size_t GetPlayerCount(bool includeRobots)
    {
        std::lock_guard guard(PlayerListLock);

        size_t count = 0;
        for (const auto& [id, player] : Players)
        {
            if (includeRobots || !player.IsRobot)
            {
                count++;
            }
        }

        return count;
    }

    ServerPlayer& AddRobotPlayer()
    {
        std::lock_guard guard(PlayerListLock);

        LastRobotPlayerID++;
        uint64_t id = RobotPlayerIDMask + LastRobotPlayerID;

        auto itr = Players.emplace(id, nullptr).first;
        itr->second.PlayerID = id;
        itr->second.IsRobot = true;

        return itr->second;
    }

    void DoForEachPlayer(std::function<void(ServerPlayer&)> function, bool includeRobots, uint64_t except )
    {
        if (!function)
            return;

        std::lock_guard guard(PlayerListLock);

        for (auto& [id, player] : Players)
        {
            if (id == except)
                continue;

            if (!includeRobots && player.Peer == nullptr)
                continue;

            function(player);
        }
    }

    bool Empty(bool includeRobots)
    {
        std::lock_guard guard(PlayerListLock);

        if (includeRobots || Players.empty())
            return Players.empty();

        for (auto& [id, playerInfo] : Players)
        {
            if (!playerInfo.IsRobot)
                return false;
        }

        return true;
    }
}
