#pragma once

#include "protocol.h"
#include "log_system.h"
#include <cstdint>
#include <string>
#include <vector>

struct PacketTypeStats
{
    uint64_t TotalBytes = 0;
    uint64_t TotalPackets = 0;
    uint64_t WindowBytes = 0;
    uint64_t WindowPackets = 0;
    double RateBytesPerSec = 0.0;
    double RatePacketsPerSec = 0.0;
};

class BandwidthTracker
{
public:
    BandwidthTracker();

    void RecordPacketSent(uint8_t packetType, size_t byteSize);
    void RecordPacketReceived(uint8_t packetType, size_t byteSize);

    void Update(uint64_t nowMs, uint64_t rawWireBytesSent = 0, uint64_t rawWireBytesReceived = 0);

    bool ShouldPrintReport(uint64_t nowMs) const;
    void PrintReport(Logger& logger, size_t humanPlayerCount, size_t robotCount, uint64_t nowMs = 0);

    std::string GetSummaryString() const;

    double GetUploadRateBytesPerSec() const
    {
        return LastUploadRateBytesPerSec;
    }

    double GetDownloadRateBytesPerSec() const
    {
        return LastDownloadRateBytesPerSec;
    }

    double GetUploadPacketsPerSec() const
    {
        return LastUploadPacketsPerSec;
    }

    double GetDownloadPacketsPerSec() const
    {
        return LastDownloadPacketsPerSec;
    }

    uint64_t GetTotalBytesSent() const
    {
        return TotalPayloadBytesSent;
    }

    uint64_t GetTotalBytesReceived() const
    {
        return TotalPayloadBytesReceived;
    }

    void SetReportIntervalMs(uint64_t intervalMs)
    {
        ReportIntervalMs = intervalMs;
    }

    static const char* GetPacketTypeName(uint8_t type);

private:
    static constexpr size_t kMaxPacketTypes = 32;

    uint64_t TotalPayloadBytesSent = 0;
    uint64_t TotalPayloadBytesReceived = 0;
    uint64_t TotalPacketsSent = 0;
    uint64_t TotalPacketsReceived = 0;

    uint64_t TotalWireBytesSent = 0;
    uint64_t TotalWireBytesReceived = 0;

    PacketTypeStats OutgoingStats[kMaxPacketTypes];
    PacketTypeStats IncomingStats[kMaxPacketTypes];

    uint64_t LastWindowTimeMs = 0;
    uint64_t WindowPayloadBytesSent = 0;
    uint64_t WindowPayloadBytesReceived = 0;
    uint64_t WindowPacketsSent = 0;
    uint64_t WindowPacketsReceived = 0;

    uint64_t WindowWireBytesSent = 0;
    uint64_t WindowWireBytesReceived = 0;
    uint64_t PrevRawWireBytesSent = 0;
    uint64_t PrevRawWireBytesReceived = 0;

    double LastUploadRateBytesPerSec = 0.0;
    double LastDownloadRateBytesPerSec = 0.0;
    double LastUploadPacketsPerSec = 0.0;
    double LastDownloadPacketsPerSec = 0.0;

    double LastWireUploadRateBytesPerSec = 0.0;
    double LastWireDownloadRateBytesPerSec = 0.0;

    uint64_t LastReportTimeMs = 0;
    uint64_t ReportIntervalMs = 5000;
};
