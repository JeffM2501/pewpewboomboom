#include "network_manager.h"
#include "log_system.h"
#include "player_list.h"
#include "packet_handlers.h"

extern Logger ServerLogger;
extern bool Running;

NetworkManager::NetworkManager()
{
    PacketHandlers::RegisterAll(*this);
}

NetworkManager::~NetworkManager()
{
    Shutdown();
}

bool NetworkManager::Initialize(int port, int maxPlayers)
{
    ENetAddress address = { 0 };

    address.host = ENET_HOST_ANY;
    address.port = static_cast<enet_uint16>(port);

    if (enet_initialize() != 0)
    {
        return false;
    }

    m_ServerHost = enet_host_create(&address,
        maxPlayers,
        2,  // channels
        0,  // incoming bandwidth
        0   // outgoing bandwidth
    );

    return m_ServerHost != nullptr;
}

void NetworkManager::Shutdown()
{
    if (m_ServerHost)
    {
        enet_host_destroy(m_ServerHost);
        m_ServerHost = nullptr;
        enet_deinitialize();
    }
}

void  NetworkManager::RemovePlayer(ENetPeer* peer, bool isDisconnect)
{
    if (!ServerPlayerList::PlayerExists(peer))
    {
        return;
    }

    auto& player = ServerPlayerList::GetPlayer(peer);

    auto playerID = player.PlayerID;
    PlayerDisconnected.Invoke(playerID, this);

    ServerLogger.Log(LogLevel::Info, "Removing player %s (ID: %llu) from server.", player.Name.Data(), playerID);
    ServerPlayerList::RemovePlayer(peer);

    ServerPlayerList::DoForEachPlayer([playerID, isDisconnect, this](auto& playerInfo)
        {
            S2C_PlayerDisconnected deadPlayer;
            deadPlayer.playerId = playerID;
            deadPlayer.reason = isDisconnect ? S2C_PlayerDisconnected::Reason::Dissconnect : S2C_PlayerDisconnected::Reason::Quit;
            SendPacket(playerInfo.Peer, 0, deadPlayer);
        });
}

void NetworkManager::PollEvents(int timeoutMs)
{
    if (!m_ServerHost)
    {
        return;
    }

    ENetEvent event;

    while (enet_host_service(m_ServerHost, &event, timeoutMs) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                ServerLogger.Log(LogLevel::Info, "A new client connected from %x:%d", event.peer->address.host, event.peer->address.port);

                ;
                PlayerConnected.Invoke(ServerPlayerList::GetPlayer(event.peer).PlayerID, this);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                ServerLogger.Log(LogLevel::Verbose, "A packet of length %d containing %x was received from %x on channel %d.", event.packet->dataLength, event.packet->data, event.peer->data, event.channelID);

                ProcessPacket(event.packet, event.peer);

                /* Clean up the packet now that we're done using it. */
                enet_packet_destroy(event.packet);

                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                ServerLogger.Log(LogLevel::Info, "%x disconnected.", event.peer->data);
                RemovePlayer(event.peer, true);
                /* Reset the peer's client information. */
                event.peer->data = nullptr;

                if (ServerPlayerList::Empty())
                {
                    Running = false;
                }
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
            {
                ServerLogger.Log(LogLevel::Info, "%x disconnected (timeout).", event.peer->data);
                RemovePlayer(event.peer, true);
                if (ServerPlayerList::Empty())
                {
                    Running = false;
                }
                break;
            }
        }
    }
}
