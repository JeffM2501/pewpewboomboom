#include "packet_processor.h"

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
        if (processor == Processors.end() || packet->dataLength < processor->second.PacketSize)
            return;

        processor->second.Processor(*this, sender, packet->data);
    }
}