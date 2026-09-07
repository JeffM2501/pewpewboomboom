#include "time_utils.h"
#include "log_system.h"
#include "text_utils.h"
#include "constants.h"
#include "raylib.h"

#include "player_state.h"
#include "player_list.h"

#include "network_manager.h"
#include "robot_ai.h"
#include "world_data.h"
#include "protocol.h"
#include "server_world.h"

bool Running = true;


Logger ServerLogger(ConsoleLogOutput);
NetworkManager NetManager;

ServerWorld World(500);

void SendWorldData(uint64_t playerID, void* sender)
{
	NetManager.Send(playerID, 0, World.InfoPacket, true);
	NetManager.Send(playerID, 0, World.Walls.Packet, true);
	for (auto& object : World.Objects)
	{
		NetManager.Send(playerID, 0, object->Packet, true);
	}
}

void PopulateWorld()
{
	ServerLogger.Log(LogLevel::Info, "Generating Simple World");

	int wallSize = int(World.Walls.GetBoundingCircle().Radius) * 2;

	for (auto i = 0; i < 100;)
	{
		BoundingCircle bounds;

		bounds.Center = Vector2{ float(GetRandomValue(-wallSize, wallSize)), float(GetRandomValue(-wallSize, wallSize)) };
		float size = float(GetRandomValue(5, 20));
		bounds.Radius = sqrtf((size / 2.0f) * (size / 2.0f));

		if (World.CanPlaceObject(bounds))
		{
			i++;
			auto& box = World.AddObject<ServerWorldBuilding>(bounds.Center, GetRandomValue(0, 8) * 45.0f, size);
			box.Packet.id = i;	
		}
	}
}

void ServerSetup()
{
	SetRandomSeed(uint32_t(std::chrono::system_clock::now().time_since_epoch().count()));
	ServerLogger.Log(LogLevel::Info, "Server is starting up...");

	PopulateWorld();

	NetManager.ServerEmpty.Add([](const bool&, void*)
		{
			Running = false;
		});

	NetManager.PlayerJoined.Add(SendWorldData);

	if (NetManager.Initialize(7777, kMaxPlayers))
	{
		ServerLogger.Log(LogLevel::Info, "Server started successfully on port 7777.");
	}
	else
	{
		ServerLogger.Log(LogLevel::Error, "Failed to start server on port 7777");
	}
}

void ServerCleanup()
{
	NetManager.Shutdown();
	ServerLogger.Log(LogLevel::Info, "Server is shutdown...");
}

void SendStateUpdates()
{
    ServerPlayerList::DoForEachPlayer([&](auto& player)
        {
            S2C_BeginStateSnapshot beginSnapshot;
            beginSnapshot.snapshotTick = NetManager.CurrentServerTick;
            beginSnapshot.playerCount = uint8_t(ServerPlayerList::GetPlayerCount());
            NetManager.Send(player.PlayerID, 1, beginSnapshot, false);
            ServerPlayerList::DoForEachPlayer([&](auto& otherPlayer)
                {
                    S2C_PlayerSnapshot snapshot;
                    snapshot.serverTick = NetManager.CurrentServerTick;
                    snapshot.playerId = otherPlayer.PlayerID;
                    snapshot.position[0] = otherPlayer.Transform.Position.x;
                    snapshot.position[1] = otherPlayer.Transform.Position.y;
                    snapshot.rotation[0] = otherPlayer.Transform.Rotation[0];
                    snapshot.rotation[1] = otherPlayer.Transform.Rotation[1];
                    snapshot.velocity[0] = otherPlayer.Transform.Velocity.x;
                    snapshot.velocity[1] = otherPlayer.Transform.Velocity.y;
                    NetManager.Send(player.PlayerID, 1, snapshot, false);
                }, true);
        }, false);
}

void UpdatePlayerHistories()
{
	uint64_t oldestTickToKeep = NetManager.CurrentServerTick;
	uint64_t maxTickHistory = 2 * kDefaultTickRate;

	if (oldestTickToKeep < maxTickHistory)
		oldestTickToKeep = 0;
	else
		oldestTickToKeep -= maxTickHistory;

	ServerPlayerList::DoForEachPlayer([&](ServerPlayerList::ServerPlayer& player)
		{
			for (auto itr = player.TransformHistory.begin(); itr != player.TransformHistory.end();)
			{
				if (itr->first < oldestTickToKeep)
					itr = player.TransformHistory.erase(itr);
				else
					break;
			}
		}
	, true);
}

int main(int argc, char* argv[])
{
	ServerSetup();
	RobotAI::SetupRobots();

	FixedTickAccumulator serverTick(ServerTickTime);

	while (Running && NetManager.GetHost())
	{
		// process things that happen on the server tick
		serverTick.ProcessTicks([](auto deltaTime)
			{
				NetManager.NewTick();

				// see if the players have updates this tick
				ServerPlayerList::DoForEachPlayer([](auto& player)
					{
						player.Update(NetManager);
					}, true);

				UpdatePlayerHistories();
                SendStateUpdates();
			});

		// process items that can happen anytime

		NetManager.PollEvents(ServerTickTimeMS);
	}

	ServerCleanup();
	return 0;
}