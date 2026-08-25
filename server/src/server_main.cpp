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

bool Running = true;

Logger ServerLogger(ConsoleLogOutput);
NetworkManager NetManager;
WorldData World;

void SendWorldData(uint64_t playerID, void* sender)
{
    S2C_SetWorldInfo worldInfo;
    worldInfo.objectCount = World.WorldObjects.size();
    CopyFixedSizeString(worldInfo.name, "Random World", kMaxChatLineSize);
    NetManager.Send(playerID, 0, worldInfo, true);

    for (auto& object : World.WorldObjects)
    {
        NetManager.Send(playerID, 0, object, true);
    }
}

void PopulateWorld()
{
    ServerLogger.Log(LogLevel::Info, "Generating Simple World");

    int worldSize = 500;

    S2C_SetWorldObject wall;
    wall.objType = S2C_SetWorldObject::ObjectType::Walls;
    wall.rotation = 0;
    wall.scale = worldSize*2.0f;
    wall.position[0] = 0;
    wall.position[1] = 0;
    wall.id = 0;
    World.WorldObjects.push_back(wall);

    for (auto i = 0; i < 50; i++)
    {
        S2C_SetWorldObject box;
        box.id = i + 1;
        box.objType = S2C_SetWorldObject::ObjectType::Building;
        box.rotation = float(GetRandomValue(-180,180));
        box.scale = float(GetRandomValue(10,20));
        box.position[0] = float(GetRandomValue(-worldSize, worldSize));
        box.position[1] = float(GetRandomValue(-worldSize, worldSize));
        World.WorldObjects.push_back(box);
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