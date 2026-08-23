#include "external/fix_win32_compatibility.h"
#include "packet_handlers.h"
#include "network_manager.h"
#include "protocol.h"
#include "player_list.h"
#include "log_system.h"
#include "time_utils.h"
#include "data_utils.h"
#include "raylib.h"

extern Logger ServerLogger;
extern uint64_t ServerStartTimeMs;
extern uint64_t CurrentServerTick;

namespace PacketHandlers
{
    static void ProcessC2S_Ping(PacketProcessor& processor, ENetPeer* sender, const C2S_Ping* ping)
    {
        S2C_Pong pong;
        pong.clientTimeMs = ping->clientTimeMs;
        pong.serverTimeMs = GetTimeMs() - ServerStartTimeMs;
        pong.serverTick = CurrentServerTick;

        processor.SendPacket(sender, 0, pong);

        ServerLogger.Log(LogLevel::Info, "Received C2S_Ping from client, replied with S2C_Pong (Server Uptime: %llu ms)", pong.serverTimeMs);
    }

    static void ProcessC2S_Goodbye(PacketProcessor& processor, ENetPeer* sender, const C2S_Goodbye* goodbye)
    {
        ServerLogger.Log(LogLevel::Info, "%x sent goodbye, reason %d.", sender->address.host, goodbye->reason);
        static_cast<NetworkManager&>(processor).RemovePlayer(sender, false);
        enet_peer_disconnect_now(sender, 0);
    }

    static void ProcessC2S_JoinRequest(PacketProcessor& processor, ENetPeer* sender, const C2S_JoinRequest* join)
    {
        S2C_JoinResponse responce;

        if (!ServerPlayerList::PlayerExists(sender))
        {
            responce.result = S2C_JoinResponse::Result::Failure;
            ServerLogger.Log(LogLevel::Warning, "Received C2S_JoinRequest from unknown peer %x.", sender->address.host);
            processor.SendPacket(sender, 0, responce);
            enet_peer_disconnect_now(sender, 0);
            return;
        }

        auto& player = ServerPlayerList::GetPlayer(sender);

        player.Name = join->desriredName;

        player.Transform.Position.x = float(GetRandomValue(-50, 50));
        player.Transform.Position.y = float(GetRandomValue(-50, 50));

        responce.result = S2C_JoinResponse::Result::Success;
        player.Name.CopyToBuffer(responce.actualName);

        responce.playerId = player.PlayerID;
        DataUtils::PackVector2(player.Transform.Position, responce.spawn);

        processor.SendPacket(sender, 0, responce);
        ServerLogger.Log(LogLevel::Info, "%x sent Join Response, ID %d name %s", sender->address.host, responce.playerId, responce.actualName);

        // send the world snapshot

        // send the player list to them
        ServerPlayerList::DoForEachPlayer([sender,&processor](auto& playerInfo)
            {
                S2C_PlayerJoined remotePlayer;
                remotePlayer.playerId = playerInfo.PlayerID;
                playerInfo.Name.CopyToBuffer(remotePlayer.name);
                processor.SendPacket(sender, 0, remotePlayer);
            }
            , true, player.PlayerID);

        // send them player updates of all current players
        

        // send them to all other players
        ServerPlayerList::DoForEachPlayer([&player,&processor](auto& playerInfo)
            {
                S2C_PlayerJoined newPlayer;
                newPlayer.playerId = player.PlayerID;
                player.Name.CopyToBuffer(newPlayer.name);
                processor.SendPacket(playerInfo.Peer, 0, newPlayer);
            }
            , false, player.PlayerID);
    }

    void RegisterAll(PacketProcessor& processor)
    {
        processor.RegisterProcessor<C2S_Ping>(PacketType::C2S_Ping, ProcessC2S_Ping);
        processor.RegisterProcessor<C2S_Goodbye>(PacketType::C2S_Goodbye, ProcessC2S_Goodbye);
        processor.RegisterProcessor<C2S_JoinRequest>(PacketType::C2S_JoinRequest, ProcessC2S_JoinRequest);
    }
}
