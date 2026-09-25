#pragma once

#include "external/fix_win32_compatibility.h"
#include "raylib.h"

#include <vector>
#include <unordered_map>
#include <cstdint>
#include "event_source.h"
#include "text_utils.h"
#include "constants.h"
#include "player_list.h"
#include "packet_processor.h"
#include "time_utils.h"

struct S2C_BulletSnapshot;
struct S2C_BulletDestroyed;

struct ClientBullet
{
	uint16_t ID = 0;
	uint64_t OwnerID = 0;
	uint8_t BulletType = 0;
	Vector2 Position = { 0.0f, 0.0f };
	Vector2 Velocity = { 0.0f, 0.0f };
	float LastUpdatedTime = 0.0f;
};


enum class ConnectionState
{
	Idle,
	Disconnected,
	Connecting,
	Connected,
};

class ClientNetworkManager : public PacketProcessor
{
public:
	struct Events
	{
		EventSource<bool> OnConnect;
		EventSource<bool> OnDisconnect;
		EventSource<ClientPlayerState*> OnJoin;
		EventSource<uint64_t> OnPlayerJoin;
		EventSource<uint64_t> OnPlayerDisconnect;
		EventSource<Vector2> OnSpawn;
		EventSource<uint64_t> OnTick;
		EventSource<std::pair<uint64_t, std::string>> OnChatMessage;
		EventSource<S2C_BulletDestroyed> OnBulletDestroyed;
		EventSource<S2C_HitscanEffect> OnHitscanEffect;
		EventSource<MachineGunHitBuildingEvent> OnMachineGunHitBuilding;
		EventSource<MachineGunHitTankEvent> OnMachineGunHitTank;
	};

	ClientNetworkManager();
	~ClientNetworkManager();

	FixedSizeString<kMaxPlayers>& GetPlayerName();

	void BeginConnect(const char* address, uint16_t port);
	void Disconnect();
	void Update();

	bool HadTimeout() const;
	ConnectionState GetState() const;
	bool IsReady() const;
	uint64_t GetRTT() const;
	Vector2 GetSpawn() const;
	uint64_t GetCurrentServerTick() const;

	Events& GetEvents();
	PlayerList& GetPlayerList();

	const std::unordered_map<uint16_t, ClientBullet>& GetBullets() const
	{
		return Bullets;
	}
	void UpdateBullets(float deltaTime);

	void SentChatMessage(std::string_view message);

private:
	ConnectionState CurrentState = ConnectionState::Disconnected;
	ENetHost* ClientHost = nullptr;
	ENetPeer* ServerPeer = nullptr;

	bool WasTimeout = false;
	uint64_t LastRTT = 0;

	uint64_t ServerTickBase = 0;
	uint64_t ServerTickSyncTimeMs = 0;
	uint64_t ClientLastProcessedServerTick = 0;

	uint64_t LastReceivedServerTick = 0;

	uint64_t ConnectionStartTime = 0;
	uint64_t ConnectionTimeout = 10 * 1000;

	FixedSizeString<kMaxPlayers> PlayerName = "PlayerMcPlayerface";
	uint64_t PlayerID = uint64_t(-1);
	Vector2 Spawn = { 0.0f, 0.0f };

	Events ConnectionEvents;
	PlayerList Players;
	std::unordered_map<uint16_t, ClientBullet> Bullets;
	std::unordered_map<uint16_t, float> RecentlyDestroyedBullets;

	FixedTickAccumulator PingAccumualtor;

	void SendPing();
	void SendJoin();

	static void ProcessS2C_Pong(PacketProcessor& processor, ENetPeer* sender, const S2C_Pong* pong);
	static void ProcessS2C_JoinResponse(PacketProcessor& processor, ENetPeer* sender, const S2C_JoinResponse* responce);
	static void ProcessS2C_PlayerJoined(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerJoined* joinInfo);
	static void ProcessS2C_PlayerDisconnected(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerDisconnected* disconnectInfo);
	static void ProcessC2S_ChatMessage(PacketProcessor& processor, ENetPeer* sender, const C2S_ChatMessage* chatMessage);
	static void ProcessS2C_SetWorldInfo(PacketProcessor& processor, ENetPeer* sender, const S2C_SetWorldInfo * worldInfo);
	static void ProcessS2C_SetWorldObject(PacketProcessor& processor, ENetPeer* sender, const S2C_SetWorldObject* objectInfo);

    static void ProcessS2C_BeginStateSnapshot(PacketProcessor& processor, ENetPeer* sender, const S2C_BeginStateSnapshot* snapshot);
    static void ProcessS2C_PlayerSnapshot(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerSnapshot* snapshot);
    static void ProcessS2C_BulletSnapshot(PacketProcessor& processor, ENetPeer* sender, const S2C_BulletSnapshot* snapshot);
    static void ProcessS2C_BulletDestroyed(PacketProcessor& processor, ENetPeer* sender, const S2C_BulletDestroyed* packet);
    static void ProcessS2C_HitscanEffect(PacketProcessor& processor, ENetPeer* sender, const S2C_HitscanEffect* packet);
};

extern ClientNetworkManager Network;
