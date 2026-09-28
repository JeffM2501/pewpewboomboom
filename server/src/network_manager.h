#pragma once
#include "external/fix_win32_compatibility.h"
#include "player_list.h"
#include "enet.h"
#include "packet_processor.h"
#include "chat_filter_processor.h"

#include "event_source.h"
#include "bandwidth_tracker.h"

static constexpr double ServerTickTime = double(kDefaultTickRate);
static constexpr int ServerTickTimeMS = static_cast<int>(1.0f / ServerTickTime * 1000.0f);

class NetworkManager : public PacketProcessor
{
public:
    NetworkManager();
    ~NetworkManager();

    bool Initialize(int port, int maxPlayers);
    void Shutdown();

    void NewTick();

    void PollEvents(int timeoutMs);
    void Flush();

    ENetHost* GetHost() const
    {
        return ServerHost;
    }

    BandwidthTracker& GetBandwidthTracker()
    {
        return Bandwidth;
    }

    void UpdateBandwidth(uint64_t nowMs)
    {
        uint64_t wireSent = ServerHost ? ServerHost->totalSentData : 0;
        uint64_t wireRecv = ServerHost ? ServerHost->totalReceivedData : 0;
        Bandwidth.Update(nowMs, wireSent, wireRecv);
    }

    EventSource<uint64_t> PlayerConnected;
    EventSource<uint64_t> PlayerDisconnected;

    EventSource<bool> ServerEmpty;

    EventSource<uint64_t> PlayerJoined;

    EventSource<MachineGunHitBuildingEvent> OnMachineGunHitBuilding;
    EventSource<MachineGunHitTankEvent> OnMachineGunHitTank;

    void RemovePlayer(ENetPeer* peer, bool isDisconnect);

    template<class T>
    void Send(uint64_t playerID, int channel, T& data, bool reliable = true)
    {
        auto player = ServerPlayerList::GetPlayer(playerID);

        if (player == nullptr || player->Peer == nullptr)
        {
            return;
        }

        ENetPacket* packet = enet_packet_create(&data, sizeof(T), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
        enet_peer_send(player->Peer, channel, packet);

        uint8_t packetType = *reinterpret_cast<const uint8_t*>(&data);
        Bandwidth.RecordPacketSent(packetType, sizeof(T));
    }

    template<class T>
    void Broadcast(int channel, T& data, bool reliable = true, uint64_t excludedPlayerID = uint64_t(-1))
    {
        uint8_t packetType = *reinterpret_cast<const uint8_t*>(&data);
        ServerPlayerList::DoForEachPlayer([channel, &data, reliable, packetType, this](auto& player)
        {
            ENetPacket* packet = enet_packet_create(&data, sizeof(T), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
            enet_peer_send(player.Peer, channel, packet);
            Bandwidth.RecordPacketSent(packetType, sizeof(T));
        }, false, excludedPlayerID);
    }

    uint64_t ServerStartTimeMs = 0;
    uint64_t CurrentServerTick = 0;

    ChatFilterProcessor& GetChatProcessor() { return ChatProcessor; }

    std::function<Vector2(const Vector2& oldPos, const Vector2& desiredPos, ServerPlayerList::ServerPlayer& player)> ProcessPlayerUpdate = nullptr;
    std::function<void(ServerPlayerList::ServerPlayer& player)> SetupRemotePlayer = nullptr;
private:
    ENetHost* ServerHost = nullptr;
    ChatFilterProcessor ChatProcessor;
    BandwidthTracker Bandwidth;
};
