
#include <stdio.h>

#include "enet.h"
#include "external/fix_win32_compatibility.h"

#include "time_utils.h"
#include "log_system.h"
#include "protocol.h"
#include "packet_processor.h"
#include "text_utils.h"
#include "constants.h"

bool Running = true;

ENetHost* ServerHost = nullptr;
uint64_t ServerStartTimeMs = 0;
PacketProcessor Proessor;

Logger ServerLogger(ConsoleLogOutput);

static constexpr double ServerTickTime = double(kDefaultTickRate);
static constexpr int ServerTickTimeMS = static_cast<int>(1.0f / ServerTickTime * 1000.0f);

FixedTickAccumulator ServerTick(ServerTickTime);

void ProcessC2S_Ping(ENetPeer* sender, const C2S_Ping* ping);
void ProcessC2S_Goodbye(ENetPeer* sender, const C2S_Goodbye* goodbye);
void ProcessC2S_JoinRequest(ENetPeer* sender, const C2S_JoinRequest* goodbye);

void ServerSetup()
{
	ENetAddress address = { 0 };

	address.host = ENET_HOST_ANY;
	address.port = 7777;
	enet_initialize();
	ServerStartTimeMs = GetTimeMs();
	ServerLogger.Log(LogLevel::Info, "Server is starting up...");

	ServerHost = enet_host_create(&address /* the address to bind the server host to */,
		kMaxPlayers     /* allow up to 32 clients and/or outgoing connections */,
		2				/* allow up to 2 channels to be used, 0 and 1 */,
		0				/* assume any amount of incoming bandwidth */,
		0				/* assume any amount of outgoing bandwidth */);

	if (ServerHost)
	{
		ServerLogger.Log(LogLevel::Info, "Server started successfully on port %d.", address.port);
	}
	else
	{
		ServerLogger.Log(LogLevel::Error, "Failed to start server on port %d", address.port);
	}

	Proessor.RegisterProcessor<C2S_Ping>(PacketType::C2S_Ping, ProcessC2S_Ping);
	Proessor.RegisterProcessor<C2S_Goodbye>(PacketType::C2S_Goodbye, ProcessC2S_Goodbye);
	Proessor.RegisterProcessor<C2S_JoinRequest>(PacketType::C2S_JoinRequest, ProcessC2S_JoinRequest);
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

			Proessor.ProcessPacket(event.packet, event.peer);

			/* Clean up the packet now that we're done using it. */
			enet_packet_destroy(event.packet);

			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			ServerLogger.Log(LogLevel::Info, "%x disconnected.", event.peer->data);

			/* Reset the peer's client information. */

			event.peer->data = nullptr;
			break;

		case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			ServerLogger.Log(LogLevel::Info, "%x disconnected (timeout).", event.peer->data);
			break;
		}
	}
}

void ProcessC2S_Ping(ENetPeer* sender, const C2S_Ping* ping)
{
	S2C_Pong pong;
	pong.clientTimeMs = ping->clientTimeMs;
	pong.serverTimeMs = GetTimeMs() - ServerStartTimeMs;

	Proessor.SendPacket(sender, 0, pong);

	ServerLogger.Log(LogLevel::Info, "Received C2S_Ping from client, replied with S2C_Pong (Server Uptime: %llu ms)", pong.serverTimeMs);
}

void ProcessC2S_Goodbye(ENetPeer* sender, const C2S_Goodbye* goodbye)
{
	ServerLogger.Log(LogLevel::Info, "%x sent goodbye, reason %d.", sender->address.host, goodbye->reason);

	enet_peer_disconnect_now(sender, 0);
}

void ProcessC2S_JoinRequest(ENetPeer* sender, const C2S_JoinRequest* join)
{
    S2C_JoinResponse responce;
	
	CopyFixedSizeString(responce.actualName, join->desriredName, kMaxPlayers);
	responce.result = S2C_JoinResponse::Result::Success;
	responce.playerId = uint64_t(sender->connectID);
	responce.spawnX = 100;
	responce.spawnY = 100;
    Proessor.SendPacket(sender, 0, responce);

    ServerLogger.Log(LogLevel::Info, "%x sent Join Response, ID %d name %s", responce.playerId, responce.actualName);
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