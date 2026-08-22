#include "external/fix_win32_compatibility.h"

#include <stdio.h>

#include "enet.h"

#include "time_utils.h"
#include "log_system.h"
#include "protocol.h"
#include "packet_processor.h"
#include "text_utils.h"
#include "constants.h"
#include "raylib.h"

#include "player_state.h"

bool Running = true;

ENetHost* ServerHost = nullptr;
uint64_t ServerStartTimeMs = 0;
PacketProcessor Proessor;

Logger ServerLogger(ConsoleLogOutput);

static constexpr double ServerTickTime = double(kDefaultTickRate);
static constexpr int ServerTickTimeMS = static_cast<int>(1.0f / ServerTickTime * 1000.0f);

FixedTickAccumulator ServerTick(ServerTickTime);

std::unordered_map<enet_uint32, PlayerState> ServerPlayers;

void ProcessC2S_Ping(ENetPeer* sender, const C2S_Ping* ping);
void ProcessC2S_Goodbye(ENetPeer* sender, const C2S_Goodbye* goodbye);
void ProcessC2S_JoinRequest(ENetPeer* sender, const C2S_JoinRequest* goodbye);

uint64_t CurrentServerTick = 0;

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

void RemovePlayer(ENetPeer* peer)
{
	auto it = ServerPlayers.find(peer->connectID);
	if (it != ServerPlayers.end())
	{
		ServerLogger.Log(LogLevel::Info, "Removing player %s (ID: %llu) from server.", it->second.Name, it->second.PlayerID);
		ServerPlayers.erase(it);

		// send a remove player to all other players
	}
	else
	{
		ServerLogger.Log(LogLevel::Warning, "Attempted to remove unknown player with connectID %u.", peer->connectID);
	}
}

void ServerNetUpdate(double deltaTime)
{
	CurrentServerTick++;

	ENetEvent event;

	ServerLogger.Log(LogLevel::Verbose, "ServerNetUpdate: Waiting for events dt(%0.1f)ms", deltaTime * 1000);

	/* Wait up to 1000 milliseconds for an event. */
	while (enet_host_service(ServerHost, &event, ServerTickTimeMS) > 0)
	{
		switch (event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			ServerLogger.Log(LogLevel::Info, "A new client connected from %x:%d", event.peer->address.host, event.peer->address.port);

			ServerPlayers.emplace(event.peer->connectID, PlayerState{ uint64_t(event.peer->connectID), event.peer });
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			ServerLogger.Log(LogLevel::Verbose, "A packet of length %d containing %x was received from %x on channel %d.", event.packet->dataLength, event.packet->data, event.peer->data, event.channelID);

			Proessor.ProcessPacket(event.packet, event.peer);

			/* Clean up the packet now that we're done using it. */
			enet_packet_destroy(event.packet);

			break;

		case ENET_EVENT_TYPE_DISCONNECT:
			ServerLogger.Log(LogLevel::Info, "%x disconnected.", event.peer->data);
			RemovePlayer(event.peer);
			/* Reset the peer's client information. */
			event.peer->data = nullptr;

			if (ServerPlayers.empty())
			{
				Running = false;
			}
			break;

		case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			ServerLogger.Log(LogLevel::Info, "%x disconnected (timeout).", event.peer->data);
			RemovePlayer(event.peer);
			if (ServerPlayers.empty())
			{
				Running = false;
			}
			break;
		}
	}
}

void ProcessC2S_Ping(ENetPeer* sender, const C2S_Ping* ping)
{
	S2C_Pong pong;
	pong.clientTimeMs = ping->clientTimeMs;
	pong.serverTimeMs = GetTimeMs() - ServerStartTimeMs;
	pong.serverTick = CurrentServerTick;

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

	auto it = ServerPlayers.find(sender->connectID);
	if (it == ServerPlayers.end())
	{
		responce.result = S2C_JoinResponse::Result::Failure;
		ServerLogger.Log(LogLevel::Warning, "Received C2S_JoinRequest from unknown peer %x.", sender->address.host);
		Proessor.SendPacket(sender, 0, responce);
		enet_peer_disconnect_now(sender, 0);
		return;
	}

	it->second.PlayerID = uint64_t(sender->connectID);
	it->second.Name = "Steve";

	it->second.Transform.Position.x = float(GetRandomValue(-50, 50));
	it->second.Transform.Position.y = float(GetRandomValue(-50, 50));

	responce.result = S2C_JoinResponse::Result::Success;
	it->second.Name.CopyToBuffer(responce.actualName);

	responce.playerId = it->second.PlayerID;
	DataUtils::PackVector2(it->second.Transform.Position, responce.spawn);

	Proessor.SendPacket(sender, 0, responce);
	ServerLogger.Log(LogLevel::Info, "%x sent Join Response, ID %d name %s", sender->address.host, responce.playerId, responce.actualName);

	// send the world snapshot

	// send the player list as snapshots

	// send them to all other players

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