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

	for (auto i = 0; i < 50;)
	{
		BoundingCircle bounds;
		bounds.Center = Vector2{ float(GetRandomValue(-250, 250)), float(GetRandomValue(-250, 250)) };
		float size = float(GetRandomValue(2, 10));
		bounds.Radius = sqrtf((size / 2.0f) * (size / 2.0f));

		if (World.CanPlaceObject(bounds))
		{
			i++;
			auto& box = World.AddObject<ServerWorldBox>(Vector2{ float(GetRandomValue(-250, 250)), float(GetRandomValue(-250, 250)) }, float(GetRandomValue(-180, 180)), float(GetRandomValue(2, 10)));
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
			});

		// process items that can happen anytime

		NetManager.PollEvents(ServerTickTimeMS);
	}

	ServerCleanup();
	return 0;
}