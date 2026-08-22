#include "external/fix_win32_compatibility.h"

#include "net_connection.h"
#include "enet.h"
#include <cstdio>
#
#include "protocol.h"
#include "time_utils.h"
#include "packet_processor.h"
#include "text_utils.h"
#include "game.h"
#include "data_utils.h"

#include "raylib.h"

namespace NetConnection
{
	ConnectionState CurentState = ConnectionState::Disconnected;
	ENetHost* ClientHost = nullptr;
	ENetPeer* ServerPeer = nullptr;

	bool WasTimeout = false;
	uint64_t LastRTT = 0;

	PacketProcessor Processor;

	uint64_t ConnectionStartTime = 0;

	uint64_t ConnectionTimeout = 10 * 1000;

	FixedSizeString<kMaxPlayers> PlayerName = "PlayerMcPlayerface";

	uint64_t PlayerID = uint64_t(-1);

	Vector2 Spawn = { 0,0 };

	static Events ConnectionEvents;

	Events& GetEvents()
	{
		return ConnectionEvents;
	}

	Vector2 GetSpawn()
	{
		return Spawn;
	}

	void ProcessS2C_Pong(ENetPeer* sender, const S2C_Pong* pong)
	{
		uint64_t nowMs = GetTimeMs();
		uint64_t rtt = (nowMs >= pong->clientTimeMs) ? (nowMs - pong->clientTimeMs) : 0;
		LastRTT = rtt;

		GetLogger().Log(LogLevel::Info, "[Client] Received S2C_Pong packet! RTT Latency: %llu ms (Server Uptime: %llu ms)", rtt, pong->serverTimeMs);
	}

	void ProcessS2C_JoinResponse(ENetPeer* sender, const S2C_JoinResponse* responce)
	{
		PlayerName = responce->actualName;
		PlayerID = responce->playerId;
		Spawn = DataUtils::UnpackVector2(responce->spawn);

		ConnectionEvents.OnJoin.Invoke(PlayerID);
		ConnectionEvents.OnSpawn.Invoke(Spawn);
	}

	void Init()
	{
		enet_initialize();

		Processor.RegisterProcessor<S2C_Pong>(PacketType::S2C_Pong, ProcessS2C_Pong);
		Processor.RegisterProcessor<S2C_JoinResponse>(PacketType::S2C_JoinResponse, ProcessS2C_JoinResponse);
	}

	void Shutdown()
	{
		Disconnect();
		enet_deinitialize();
	}

	FixedSizeString<kMaxPlayers>& GetPlayerName()
	{
		return PlayerName;
	}

	void BeginConnect(const char* address, uint16_t port)
	{
		WasTimeout = false;
		if (CurentState != ConnectionState::Disconnected)
			Disconnect();

		ConnectionStartTime = GetTimeMs();

		ClientHost = enet_host_create(nullptr, 1, 2, 0, 0);
		ENetAddress hostAddress = { 0 };
		enet_address_set_host(&hostAddress, address);
		hostAddress.port = port;
		ServerPeer = enet_host_connect(ClientHost, &hostAddress, 2, 0);

		CurentState = ConnectionState::Connecting;
	}

	void Disconnect()
	{
		PlayerID = uint64_t(-1);

		if (ServerPeer)
		{
			C2S_Goodbye bye;
			Processor.SendPacket(ServerPeer, 0, bye);
			enet_host_flush(ClientHost);

			enet_peer_disconnect_now(ServerPeer, 0);
			ServerPeer = nullptr;
		}

		if (ClientHost)
		{

			enet_host_destroy(ClientHost);
			ClientHost = nullptr;
		}

		CurentState = ConnectionState::Disconnected;
	}

	ConnectionState GetState()
	{
		return CurentState;
	}

	bool HadTimeout()
	{
		return WasTimeout;
	}

	uint64_t GetRTT()
	{
		return LastRTT;
	}

	void Update()
	{
		if (CurentState != ConnectionState::Disconnected)
		{
			ENetEvent event;
			while (enet_host_service(ClientHost, &event, 0) > 0)
			{
				if (event.type == ENET_EVENT_TYPE_CONNECT)
				{
					CurentState = ConnectionState::Connected;

					// Send C2S_Ping on channel 0 reliably upon connection
					C2S_Ping ping;
					ping.type = static_cast<uint8_t>(PacketType::C2S_Ping);
					ping.clientTimeMs = GetTimeMs();

					Processor.SendPacket(ServerPeer, 0, ping);
					GetLogger().Log(LogLevel::Info, "[Client] Connected to server. Sent C2S_Ping packet on Channel 0 (timestamp: %llu ms).", ping.clientTimeMs);

					bool valid = true;
					ConnectionEvents.OnConnect.Invoke(valid);

					C2S_JoinRequest join;
					CopyFixedSizeString(join.desriredName, PlayerName, sizeof(PlayerName));
					Processor.SendPacket(ServerPeer, 0, join);

					GetLogger().Log(LogLevel::Info, "[Client] Sent C2S_JoinRequest packet on Channel 0, desired name %s.", PlayerName);
				}
				else if (event.type == ENET_EVENT_TYPE_RECEIVE)
				{
					Processor.ProcessPacket(event.packet, event.peer);
					enet_packet_destroy(event.packet);
				}
				else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
				{
					CurentState = ConnectionState::Disconnected;
				}
				else if (event.type == ENET_EVENT_TYPE_DISCONNECT_TIMEOUT)
				{
					CurentState = ConnectionState::Disconnected;
					WasTimeout = true;
				}

				enet_host_flush(ClientHost);
			}

			if (CurentState == ConnectionState::Connecting)
			{
				// see if we have waited too long
				if (GetTimeMs() - ConnectionStartTime > ConnectionTimeout)
				{
					WasTimeout = true;
					Disconnect();
				}
			}
		}
	}
}