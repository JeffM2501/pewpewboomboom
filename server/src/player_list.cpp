#define NOMINMAX

#include "player_list.h"
#include <limits>

namespace ServerPlayerList
{
    ServerPlayer::ServerPlayer(ENetPeer* peer)
    {
        Peer = peer;
    }

    static std::unordered_map<uint64_t, ServerPlayer> Players;
    static uint32_t LastRobotPlayerID = 0;
    static uint32_t RobotPlayerIDMask = std::numeric_limits<uint32_t>::max();

    std::unordered_map<uint64_t, ServerPlayer>& GetPlayerList()
    {
        return Players;
    }

    ServerPlayer& GetPlayer(ENetPeer* peer)
    {
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
        uint64_t playerId = uint64_t(peer->connectID);

        return Players.contains(playerId);
    }

    bool RemovePlayer(ENetPeer* peer)
    {
        auto itr = Players.find(peer->connectID);
        if (itr == Players.end())
            return false;

        Players.erase(itr);
        return true;
    }

    ServerPlayer& AddRobotPlayer()
    {
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
