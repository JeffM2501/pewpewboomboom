#include "external/fix_win32_compatibility.h"
#include "client_network_manager.h"
#include "enet.h"
#include <cstdio>
#include "protocol.h"
#include "time_utils.h"
#include "packet_processor.h"
#include "text_utils.h"
#include "game.h"
#include "data_utils.h"
#include "raylib.h"

ClientNetworkManager Network;

ClientNetworkManager::ClientNetworkManager()
	: PingAccumualtor(0.5f, 2)
{
	enet_initialize();

	RegisterProcessor<S2C_Pong>(PacketType::S2C_Pong, ProcessS2C_Pong);
	RegisterProcessor<S2C_JoinResponse>(PacketType::S2C_JoinResponse, ProcessS2C_JoinResponse);
	RegisterProcessor<S2C_PlayerJoined>(PacketType::S2C_PlayerJoined, ProcessS2C_PlayerJoined);
	RegisterProcessor<S2C_PlayerDisconnected>(PacketType::S2C_PlayerDisconnected, ProcessS2C_PlayerDisconnected);
}

ClientNetworkManager::~ClientNetworkManager()
{
	Disconnect();
	enet_deinitialize();
}

FixedSizeString<kMaxPlayers>& ClientNetworkManager::GetPlayerName()
{
	return PlayerName;
}

void ClientNetworkManager::BeginConnect(const char* address, uint16_t port)
{
	WasTimeout = false;
	if (CurrentState != ConnectionState::Disconnected)
	{
		Disconnect();
	}

	ConnectionStartTime = GetTimeMs();

	ClientHost = enet_host_create(nullptr, 1, 2, 0, 0);
	ENetAddress hostAddress = { 0 };
	enet_address_set_host(&hostAddress, address);
	hostAddress.port = port;
	ServerPeer = enet_host_connect(ClientHost, &hostAddress, 2, 0);

	CurrentState = ConnectionState::Connecting;
}

void ClientNetworkManager::Disconnect()
{
	PlayerID = uint64_t(-1);
	ServerTickBase = 0;
	ServerTickSyncTimeMs = 0;
	ClientLastProcessedServerTick = 0;

	if (ServerPeer)
	{
		C2S_Goodbye bye;
		SendPacket(ServerPeer, 0, bye);
		enet_host_flush(ClientHost);

		enet_peer_disconnect_now(ServerPeer, 0);
		ServerPeer = nullptr;
	}

	if (ClientHost)
	{
		enet_host_destroy(ClientHost);
		ClientHost = nullptr;
	}

	CurrentState = ConnectionState::Disconnected;
}

ConnectionState ClientNetworkManager::GetState() const
{
	return CurrentState;
}

bool ClientNetworkManager::IsReady() const
{
	return CurrentState == ConnectionState::Connected && PlayerID != uint64_t(-1);
}

bool ClientNetworkManager::HadTimeout() const
{
	return WasTimeout;
}

uint64_t ClientNetworkManager::GetRTT() const
{
	return LastRTT;
}

Vector2 ClientNetworkManager::GetSpawn() const
{
	return Spawn;
}

uint64_t ClientNetworkManager::GetCurrentServerTick() const
{
	if (ServerTickSyncTimeMs == 0)
	{
		return 0;
	}

	uint64_t nowMs = GetTimeMs();
	if (nowMs < ServerTickSyncTimeMs)
	{
		return ServerTickBase;
	}

	double elapsedSec = (nowMs - ServerTickSyncTimeMs) / 1000.0;
	uint64_t ticksElapsed = static_cast<uint64_t>(elapsedSec * kDefaultTickRate);
	return ServerTickBase + ticksElapsed;
}

void ClientNetworkManager::Update()
{
	if (CurrentState != ConnectionState::Disconnected)
	{
		ENetEvent event;
		while (enet_host_service(ClientHost, &event, 0) > 0)
		{
			if (event.type == ENET_EVENT_TYPE_CONNECT)
			{
				CurrentState = ConnectionState::Connected;

				GetLogger().Log(LogLevel::Info, "[Client] Connected to server.");
				SendPing();

				bool valid = true;
				ConnectionEvents.OnConnect.Invoke(valid);

				SendJoin();
			}
			else if (event.type == ENET_EVENT_TYPE_RECEIVE)
			{
				ProcessPacket(event.packet, event.peer);
				enet_packet_destroy(event.packet);
			}
			else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
			{
				CurrentState = ConnectionState::Disconnected;
			}
			else if (event.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
			{
				CurrentState = ConnectionState::Disconnected;
				WasTimeout = true;
			}

			enet_host_flush(ClientHost);
		}

		if (CurrentState == ConnectionState::Connecting)
		{
			// see if we have waited too long
			if (GetTimeMs() - ConnectionStartTime > ConnectionTimeout)
			{
				WasTimeout = true;
				Disconnect();
			}
		}

		if (CurrentState == ConnectionState::Connected && ServerTickSyncTimeMs > 0)
		{
			uint64_t currentTick = GetCurrentServerTick();
			if (ClientLastProcessedServerTick == 0)
			{
				ClientLastProcessedServerTick = currentTick;
			}

			while (ClientLastProcessedServerTick < currentTick)
			{
				ClientLastProcessedServerTick++;
				ConnectionEvents.OnTick.Invoke(ClientLastProcessedServerTick);
			}
		}
	}

	PingAccumualtor.ProcessTicks([this](double)
		{
			SendPing();
		});
}

ClientNetworkManager::Events& ClientNetworkManager::GetEvents()
{
	return ConnectionEvents;
}

PlayerList& ClientNetworkManager::GetPlayerList()
{
	return Players;
}

void ClientNetworkManager::SendPing()
{
	if (CurrentState == ConnectionState::Disconnected)
		return;

    // Send C2S_Ping on channel 0 reliably upon connection
    C2S_Ping ping;
    ping.type = static_cast<uint8_t>(PacketType::C2S_Ping);
    ping.clientTimeMs = GetTimeMs();

    SendPacket(ServerPeer, 0, ping);

	GetLogger().Log(LogLevel::Verbose, "[Client] Sent C2S_Ping packet on Channel 0 (timestamp: %llu ms).", ping.clientTimeMs);
}

void ClientNetworkManager::SendJoin()
{
    C2S_JoinRequest join;
    CopyFixedSizeString(join.desriredName, PlayerName, sizeof(PlayerName));
    SendPacket(ServerPeer, 0, join);

    GetLogger().Log(LogLevel::Info, "[Client] Sent C2S_JoinRequest packet on Channel 0, desired name %s.", PlayerName.Data());
}

void ClientNetworkManager::ProcessS2C_Pong(PacketProcessor& processor, ENetPeer* sender, const S2C_Pong* pong)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	uint64_t nowMs = GetTimeMs();
	uint64_t rtt = (nowMs >= pong->clientTimeMs) ? (nowMs - pong->clientTimeMs) : 0;
	self.LastRTT = rtt;

	uint64_t latency = rtt / 2;
	self.ServerTickBase = pong->serverTick;
	self.ServerTickSyncTimeMs = pong->clientTimeMs + latency;

	GetLogger().Log(LogLevel::Info, "[Client] Received S2C_Pong packet! RTT Latency: %llu ms (Server Uptime: %llu ms, Server Tick: %llu)", rtt, pong->serverTimeMs, pong->serverTick);
}

void ClientNetworkManager::ProcessS2C_JoinResponse(PacketProcessor& processor, ENetPeer* sender, const S2C_JoinResponse* responce)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	self.PlayerName = responce->actualName;
	self.PlayerID = responce->playerId;
	self.Spawn = DataUtils::UnpackVector2(responce->spawn);

	auto localPlayerInfo = self.Players.AddPlayer(self.PlayerID);
	localPlayerInfo->Name = self.PlayerName;
	localPlayerInfo->IsLocalPlayer = true;

	self.ConnectionEvents.OnJoin.Invoke(self.PlayerID);
	self.ConnectionEvents.OnSpawn.Invoke(self.Spawn);
}

void ClientNetworkManager::ProcessS2C_PlayerJoined(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerJoined* joinInfo)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	auto localPlayerInfo = self.Players.AddPlayer(joinInfo->playerId);
	localPlayerInfo->Name = joinInfo->name;
	self.ConnectionEvents.OnPlayerJoin.Invoke(joinInfo->playerId);
}

void ClientNetworkManager::ProcessS2C_PlayerDisconnected(PacketProcessor& processor, ENetPeer* sender, const S2C_PlayerDisconnected* disconnectInfo)
{
	ClientNetworkManager& self = static_cast<ClientNetworkManager&>(processor);
	self.Players.RemovePlayer(disconnectInfo->playerId);
	self.ConnectionEvents.OnPlayerDisconnect.Invoke(disconnectInfo->playerId);
}
