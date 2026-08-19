#pragma once

#include "protocol.h"
#include "enet.h"
#include <functional>
#include <unordered_map>

class PacketProcessor
{
private:
    struct ProcessorInfo
    {
        std::function<void(ENetPeer*, const void*)> Processor = nullptr;
        size_t PacketSize = 0;
    };

    std::unordered_map<uint8_t, ProcessorInfo> Processors;
public:
    void RegisterProcessor(PacketType packetType, std::function<void(ENetPeer*, const void*)> processor, size_t packetSize);

    template<class T>
    void RegisterProcessor(PacketType packetType, std::function<void(ENetPeer*, const T*)> processor)
    {
        RegisterProcessor(packetType, [processor](ENetPeer* sender, const void* pData)
            {
                processor(sender, reinterpret_cast<const T*>(pData));
            },
            sizeof(T));
    }

    void ProcessPacket(ENetPacket* packet, ENetPeer* sender);

    template<class T>
    void SendPacket(ENetPeer* peer, int channel, T& data)
    {
        ENetPacket* packet = enet_packet_create(&data, sizeof(T), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, channel, packet);
    }
};