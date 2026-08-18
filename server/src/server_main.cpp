
#include <stdio.h>

#include "enet.h"
#include "external/fix_win32_compatibility.h"

#include "time_utils.h"
#include "log_system.h"
#include "Protocol.h"

bool Running = true;

ENetHost* ServerHost = nullptr;
uint64_t ServerStartTimeMs = 0;

Logger ServerLogger(ConsoleLogOutput);

static constexpr double ServerTickTime = 60.0f;
static constexpr int ServerTickTimeMS = static_cast<int>(1.0f / ServerTickTime * 1000.0f);

FixedTickAccumulator ServerTick(ServerTickTime);

void ServerSetup()
{
	ENetAddress address = { 0 };

	address.host = ENET_HOST_ANY;
	address.port = 7777;
	enet_initialize();
	ServerStartTimeMs = GetTimeMs();
	ServerLogger.Log(LogLevel::Info, "Server is starting up...");

	ServerHost = enet_host_create(&address /* the address to bind the server host to */,
		32      /* allow up to 32 clients and/or outgoing connections */,
		2      /* allow up to 2 channels to be used, 0 and 1 */,
		0      /* assume any amount of incoming bandwidth */,
		0      /* assume any amount of outgoing bandwidth */);

	if (ServerHost)
	{
		ServerLogger.Log(LogLevel::Info, "Server started successfully on port %d.", address.port);
	}
	else
	{
		ServerLogger.Log(LogLevel::Error, "Failed to start server on port %d", address.port);
	}
}

void ServerCleanup()
{
	if (ServerHost)
	{
		enet_host_destroy(ServerHost);
		ServerHost = nullptr;
	}
	ServerLogger.Log(LogLevel::Info, "Server is shutdown...");
	enet_deinitialize();
}

void ServerNetUpdate(double deltaTime)
{
	ENetEvent event;

	ServerLogger.Log(LogLevel::Verbose, "ServerNetUpdate: Waiting for events dt(%0.1f)ms", deltaTime * 1000);

	/* Wait up to 1000 milliseconds for an event. */
	while (enet_host_service(ServerHost, &event, ServerTickTimeMS) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			ServerLogger.Log(LogLevel::Info, "A new client connected from %x:%d", event.peer->address.host, event.peer->address.port);

			break;

		case ENET_EVENT_TYPE_RECEIVE:
			ServerLogger.Log(LogLevel::Verbose, "A packet of length %d containing %x was received from %x on channel %d.", event.packet->dataLength, event.packet->data, event.peer->data, event.channelID);

			if (event.packet->dataLength >= sizeof(C2S_Ping))
			{
				uint8_t packetType = event.packet->data[0];
				if (packetType == static_cast<uint8_t>(PacketType::C2S_Ping))
				{
					const C2S_Ping* ping = reinterpret_cast<const C2S_Ping*>(event.packet->data);

					S2C_Pong pong;
					pong.type = static_cast<uint8_t>(PacketType::S2C_Pong);
					pong.clientTimeMs = ping->clientTimeMs;
					pong.serverTimeMs = GetTimeMs() - ServerStartTimeMs;

					ENetPacket* pongPacket = enet_packet_create(&pong, sizeof(S2C_Pong), ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(event.peer, 0, pongPacket);

					ServerLogger.Log(LogLevel::Info, "Received C2S_Ping from client, replied with S2C_Pong (Server Uptime: %llu ms)", pong.serverTimeMs);
				}
			}

			/* Clean up the packet now that we're done using it. */
			enet_packet_destroy(event.packet);

			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			ServerLogger.Log(LogLevel::Info, "%x disconnected.", event.peer->data);

			/* Reset the peer's client information. */

			event.peer->data = nullptr;
		}
	}
}

int main(int argc, char* argv[])
{
	ServerSetup();

	while (Running && ServerHost)
	{
		ServerTick.ProcessTicks([](auto deltaTime) {ServerNetUpdate(deltaTime); });
	}

	ServerCleanup();
	return 0;
}