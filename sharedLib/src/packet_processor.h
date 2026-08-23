#pragma once

#include "protocol.h"
#include "enet.h"
#include <functional>
#include <unordered_map>

static constexpr size_t UnknownPacketSize = size_t(-1);

class PacketProcessor
{
public:
    using PacketReadFunction = std::function<void(PacketProcessor&, ENetPeer*, const void*)>;

protected:
    ENetPeer* DefaultPeer = nullptr;

private:
    struct ProcessorInfo
    {
        PacketReadFunction Processor = nullptr;
        size_t PacketSize = UnknownPacketSize;
    };

    std::unordered_map<uint8_t, ProcessorInfo> Processors;
public:
    void RegisterProcessorBase(PacketType packetType, PacketReadFunction processor, size_t packetSize);

    template<class T>
    void RegisterProcessor(PacketType packetType, std::function<void(PacketProcessor&, ENetPeer*, const T*)> function)
    {
        RegisterProcessorBase(packetType, [function](PacketProcessor& processor, ENetPeer* sender, const void* pData)
            {
                function(processor, sender, reinterpret_cast<const T*>(pData));
            },
            sizeof(T));
    }

    void ProcessPacket(ENetPacket* packet, ENetPeer* sender);

    template<class T>
    void SendPacket(ENetPeer* peer, int channel, T& data, bool reliable = true)
    {
        if (peer == nullptr)
            peer = DefaultPeer;

        ENetPacket* packet = enet_packet_create(&data, sizeof(T), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
        enet_peer_send(peer, channel, packet);
    }
};