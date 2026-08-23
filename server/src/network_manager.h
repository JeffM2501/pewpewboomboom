#pragma once
#include "external/fix_win32_compatibility.h"
#include "enet.h"
#include "packet_processor.h"

#include "event_source.h"

class NetworkManager : public PacketProcessor
{
public:
    NetworkManager();
    ~NetworkManager();

    bool Initialize(int port, int maxPlayers);
    void Shutdown();

    void PollEvents(int timeoutMs);

    ENetHost* GetHost() const
    {
        return m_ServerHost;
    }

    EventSource<uint64_t> PlayerConnected;
    EventSource<uint64_t> PlayerDisconnected;

    void RemovePlayer(ENetPeer* peer, bool isDisconnect);

private:
    ENetHost* m_ServerHost = nullptr;
};
