#include "packet_processor.h"
#include <cstdio>

void PacketProcessor::RegisterProcessorBase(PacketType packetType, PacketReadFunction processor, size_t packetSize)
{
    Processors.insert_or_assign(uint8_t(packetType), ProcessorInfo{processor, packetSize});
}

void PacketProcessor::ProcessPacket(ENetPacket* packet, ENetPeer* sender)
{
    if (packet->dataLength >= sizeof(uint8_t))
    {
        uint8_t packetType = packet->data[0];

        auto processor = Processors.find(packetType);
        if (processor == Processors.end())
        {
            printf("[PacketProcessor] Unknown packet type %d received (len: %zu)\n", (int)packetType, packet->dataLength);
            fflush(stdout);
            return;
        }

        if (packet->dataLength < processor->second.PacketSize)
        {
            printf("[PacketProcessor] Packet type %d dropped: dataLength (%zu) < expected (%zu)\n", 
                (int)packetType, packet->dataLength, processor->second.PacketSize);
            fflush(stdout);
            return;
        }

        processor->second.Processor(*this, sender, packet->data);
    }
}