#include "bandwidth_tracker.h"
#include <cstdio>
#include <algorithm>

static void FormatBytes(double bytes, char* out, size_t outSize)
{
    if (bytes >= 1024.0 * 1024.0)
    {
        snprintf(out, outSize, "%.2f MB", bytes / (1024.0 * 1024.0));
    }
    else if (bytes >= 1024.0)
    {
        snprintf(out, outSize, "%.2f KB", bytes / 1024.0);
    }
    else
    {
        snprintf(out, outSize, "%.0f B", bytes);
    }
}

static void FormatRate(double bytesPerSec, char* out, size_t outSize)
{
    if (bytesPerSec >= 1024.0 * 1024.0)
    {
        snprintf(out, outSize, "%.2f MB/s", bytesPerSec / (1024.0 * 1024.0));
    }
    else if (bytesPerSec >= 1024.0)
    {
        snprintf(out, outSize, "%.2f KB/s", bytesPerSec / 1024.0);
    }
    else
    {
        snprintf(out, outSize, "%.0f B/s", bytesPerSec);
    }
}

const char* BandwidthTracker::GetPacketTypeName(uint8_t type)
{
    switch (static_cast<PacketType>(type))
    {
        case PacketType::C2S_Ping:
            return "C2S_Ping";
        case PacketType::S2C_Pong:
            return "S2C_Pong";
        case PacketType::C2S_Goodbye:
            return "C2S_Goodbye";
        case PacketType::C2S_JoinRequest:
            return "C2S_JoinRequest";
        case PacketType::S2C_JoinResponse:
            return "S2C_JoinResponse";
        case PacketType::C2S_InputState:
            return "C2S_InputState";
        case PacketType::S2C_SetWorldInfo:
            return "S2C_SetWorldInfo";
        case PacketType::S2C_SetWorldObject:
            return "S2C_SetWorldObject";
        case PacketType::S2C_EventNotification:
            return "S2C_EventNotification";
        case PacketType::C2S_ChatMessage:
            return "C2S_ChatMessage";
        case PacketType::S2C_PlayerJoined:
            return "S2C_PlayerJoined";
        case PacketType::S2C_PlayerDisconnected:
            return "S2C_PlayerDisconnected";
        case PacketType::S2C_BeginStateSnapshot:
            return "S2C_BeginStateSnapshot";
        case PacketType::S2C_PlayerSnapshot:
            return "S2C_PlayerSnapshot";
        case PacketType::S2C_ChatMessage:
            return "S2C_ChatMessage";
        case PacketType::S2C_BulletSnapshot:
            return "S2C_BulletSnapshot";
        case PacketType::S2C_BulletDestroyed:
            return "S2C_BulletDestroyed";
        case PacketType::C2S_HitscanShot:
            return "C2S_HitscanShot";
        case PacketType::S2C_HitscanEffect:
            return "S2C_HitscanEffect";
        default:
            return "Unknown";
    }
}

BandwidthTracker::BandwidthTracker()
{
}

void BandwidthTracker::RecordPacketSent(uint8_t packetType, size_t byteSize)
{
    TotalPayloadBytesSent += byteSize;
    TotalPacketsSent++;
    WindowPayloadBytesSent += byteSize;
    WindowPacketsSent++;

    if (packetType < kMaxPacketTypes)
    {
        OutgoingStats[packetType].TotalBytes += byteSize;
        OutgoingStats[packetType].TotalPackets++;
        OutgoingStats[packetType].WindowBytes += byteSize;
        OutgoingStats[packetType].WindowPackets++;
    }
}

void BandwidthTracker::RecordPacketReceived(uint8_t packetType, size_t byteSize)
{
    TotalPayloadBytesReceived += byteSize;
    TotalPacketsReceived++;
    WindowPayloadBytesReceived += byteSize;
    WindowPacketsReceived++;

    if (packetType < kMaxPacketTypes)
    {
        IncomingStats[packetType].TotalBytes += byteSize;
        IncomingStats[packetType].TotalPackets++;
        IncomingStats[packetType].WindowBytes += byteSize;
        IncomingStats[packetType].WindowPackets++;
    }
}

void BandwidthTracker::Update(uint64_t nowMs, uint64_t rawWireBytesSent, uint64_t rawWireBytesReceived)
{
    if (LastWindowTimeMs == 0)
    {
        LastWindowTimeMs = nowMs;
        LastReportTimeMs = nowMs;
        PrevRawWireBytesSent = rawWireBytesSent;
        PrevRawWireBytesReceived = rawWireBytesReceived;
        return;
    }

    uint64_t elapsedMs = nowMs - LastWindowTimeMs;
    if (elapsedMs >= 1000)
    {
        double elapsedSec = double(elapsedMs) / 1000.0;

        LastUploadRateBytesPerSec = double(WindowPayloadBytesSent) / elapsedSec;
        LastDownloadRateBytesPerSec = double(WindowPayloadBytesReceived) / elapsedSec;
        LastUploadPacketsPerSec = double(WindowPacketsSent) / elapsedSec;
        LastDownloadPacketsPerSec = double(WindowPacketsReceived) / elapsedSec;

        if (rawWireBytesSent >= PrevRawWireBytesSent)
        {
            WindowWireBytesSent = rawWireBytesSent - PrevRawWireBytesSent;
            LastWireUploadRateBytesPerSec = double(WindowWireBytesSent) / elapsedSec;
            TotalWireBytesSent = rawWireBytesSent;
            PrevRawWireBytesSent = rawWireBytesSent;
        }

        if (rawWireBytesReceived >= PrevRawWireBytesReceived)
        {
            WindowWireBytesReceived = rawWireBytesReceived - PrevRawWireBytesReceived;
            LastWireDownloadRateBytesPerSec = double(WindowWireBytesReceived) / elapsedSec;
            TotalWireBytesReceived = rawWireBytesReceived;
            PrevRawWireBytesReceived = rawWireBytesReceived;
        }

        for (size_t i = 0; i < kMaxPacketTypes; ++i)
        {
            OutgoingStats[i].RateBytesPerSec = double(OutgoingStats[i].WindowBytes) / elapsedSec;
            OutgoingStats[i].RatePacketsPerSec = double(OutgoingStats[i].WindowPackets) / elapsedSec;
            OutgoingStats[i].WindowBytes = 0;
            OutgoingStats[i].WindowPackets = 0;

            IncomingStats[i].RateBytesPerSec = double(IncomingStats[i].WindowBytes) / elapsedSec;
            IncomingStats[i].RatePacketsPerSec = double(IncomingStats[i].WindowPackets) / elapsedSec;
            IncomingStats[i].WindowBytes = 0;
            IncomingStats[i].WindowPackets = 0;
        }

        WindowPayloadBytesSent = 0;
        WindowPayloadBytesReceived = 0;
        WindowPacketsSent = 0;
        WindowPacketsReceived = 0;

        LastWindowTimeMs = nowMs;
    }
}

bool BandwidthTracker::ShouldPrintReport(uint64_t nowMs) const
{
    if (LastReportTimeMs == 0)
    {
        return false;
    }

    return (nowMs >= LastReportTimeMs + ReportIntervalMs);
}

void BandwidthTracker::PrintReport(Logger& logger, size_t humanPlayerCount, size_t robotCount, uint64_t nowMs)
{
    if (nowMs > 0)
    {
        LastReportTimeMs = nowMs;
    }

    char txRateStr[32];
    char rxRateStr[32];
    char totalTxStr[32];
    char totalRxStr[32];

    FormatRate(LastUploadRateBytesPerSec, txRateStr, sizeof(txRateStr));
    FormatRate(LastDownloadRateBytesPerSec, rxRateStr, sizeof(rxRateStr));
    FormatBytes(double(TotalPayloadBytesSent), totalTxStr, sizeof(totalTxStr));
    FormatBytes(double(TotalPayloadBytesReceived), totalRxStr, sizeof(totalRxStr));

    logger.Log(LogLevel::Info, "==================== [SERVER BANDWIDTH REPORT] ====================");
    logger.Log(LogLevel::Info, "Peers: %zu human client(s), %zu bot(s)", humanPlayerCount, robotCount);
    logger.Log(LogLevel::Info, "Upload (TX)  : %s (%.0f pkts/s) | Total Sent: %s (%llu pkts)",
        txRateStr, LastUploadPacketsPerSec, totalTxStr, TotalPacketsSent);
    logger.Log(LogLevel::Info, "Download (RX): %s (%.0f pkts/s) | Total Recv: %s (%llu pkts)",
        rxRateStr, LastDownloadPacketsPerSec, totalRxStr, TotalPacketsReceived);

    if (LastWireUploadRateBytesPerSec > 0.0 || LastWireDownloadRateBytesPerSec > 0.0)
    {
        char wireTxStr[32];
        char wireRxStr[32];
        FormatRate(LastWireUploadRateBytesPerSec, wireTxStr, sizeof(wireTxStr));
        FormatRate(LastWireDownloadRateBytesPerSec, wireRxStr, sizeof(wireRxStr));
        logger.Log(LogLevel::Info, "Wire UDP Rate: TX %s | RX %s", wireTxStr, wireRxStr);
    }

    logger.Log(LogLevel::Info, "Outgoing Packets (Top Types):");
    for (size_t i = 0; i < kMaxPacketTypes; ++i)
    {
        if (OutgoingStats[i].TotalPackets > 0 || OutgoingStats[i].RatePacketsPerSec > 0.0)
        {
            char rateStr[32];
            FormatRate(OutgoingStats[i].RateBytesPerSec, rateStr, sizeof(rateStr));
            double pct = (LastUploadRateBytesPerSec > 0.0) ? (OutgoingStats[i].RateBytesPerSec / LastUploadRateBytesPerSec * 100.0) : 0.0;
            logger.Log(LogLevel::Info, "  %-24s: %10s (%5.1f%%) | %4.0f pkts/s | Total: %llu pkts",
                GetPacketTypeName(uint8_t(i)), rateStr, pct, OutgoingStats[i].RatePacketsPerSec, OutgoingStats[i].TotalPackets);
        }
    }

    logger.Log(LogLevel::Info, "Incoming Packets (Top Types):");
    for (size_t i = 0; i < kMaxPacketTypes; ++i)
    {
        if (IncomingStats[i].TotalPackets > 0 || IncomingStats[i].RatePacketsPerSec > 0.0)
        {
            char rateStr[32];
            FormatRate(IncomingStats[i].RateBytesPerSec, rateStr, sizeof(rateStr));
            double pct = (LastDownloadRateBytesPerSec > 0.0) ? (IncomingStats[i].RateBytesPerSec / LastDownloadRateBytesPerSec * 100.0) : 0.0;
            logger.Log(LogLevel::Info, "  %-24s: %10s (%5.1f%%) | %4.0f pkts/s | Total: %llu pkts",
                GetPacketTypeName(uint8_t(i)), rateStr, pct, IncomingStats[i].RatePacketsPerSec, IncomingStats[i].TotalPackets);
        }
    }
    logger.Log(LogLevel::Info, "===================================================================");
}

std::string BandwidthTracker::GetSummaryString() const
{
    char txRateStr[32];
    char rxRateStr[32];
    FormatRate(LastUploadRateBytesPerSec, txRateStr, sizeof(txRateStr));
    FormatRate(LastDownloadRateBytesPerSec, rxRateStr, sizeof(rxRateStr));

    char buf[128];
    snprintf(buf, sizeof(buf), "TX: %s (%.0f p/s) | RX: %s (%.0f p/s)",
        txRateStr, LastUploadPacketsPerSec, rxRateStr, LastDownloadPacketsPerSec);
    return std::string(buf);
}
