#include "net_connection.h"

#include <cstdio>
#include "enet.h"
#include "Protocol.h"
#include "time_utils.h"

namespace NetConnection
{
	ConnectionState CurentState = ConnectionState::Disconnected;
	ENetHost* ClientHost = nullptr;
	ENetPeer* ServerPeer = nullptr;

	bool WasTimeout = false;
	uint64_t LastRTT = 0;

	void Init()
	{
		enet_initialize();
	}

	void Shutdown()
	{
		Disconnect();
		enet_deinitialize();
	}

	void BeginConnect(const char* address, uint16_t port)
	{
		WasTimeout = false;
		if (CurentState != ConnectionState::Disconnected)
			Disconnect();

		ClientHost = enet_host_create(nullptr, 1, 2, 0, 0);
		ENetAddress hostAddress = { 0 };
		enet_address_set_host(&hostAddress, address);
		hostAddress.port = port;
		ServerPeer = enet_host_connect(ClientHost, &hostAddress, 2, 0);

		CurentState = ConnectionState::Connecting;
	}

	void Disconnect()
	{
		if (ServerPeer)
		{
			enet_peer_disconnect(ServerPeer, 0);
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

					ENetPacket* packet = enet_packet_create(&ping, sizeof(C2S_Ping), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(ServerPeer, 0, packet);
					enet_host_flush(ClientHost);

					printf("[Client] Connected to server. Sent C2S_Ping packet on Channel 0 (timestamp: %llu ms).\n", ping.clientTimeMs);
				}
				else if (event.type == ENET_EVENT_TYPE_RECEIVE)
				{
					if (event.packet->dataLength >= sizeof(S2C_Pong))
					{
						uint8_t packetType = event.packet->data[0];
						if (packetType == static_cast<uint8_t>(PacketType::S2C_Pong))
						{
							const S2C_Pong* pong = reinterpret_cast<const S2C_Pong*>(event.packet->data);
							uint64_t nowMs = GetTimeMs();
							uint64_t rtt = (nowMs >= pong->clientTimeMs) ? (nowMs - pong->clientTimeMs) : 0;
							LastRTT = rtt;

							printf("[Client] Received S2C_Pong packet! RTT Latency: %llu ms (Server Uptime: %llu ms)\n", rtt, pong->serverTimeMs);
						}
					}
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
			}
		}
	}
}