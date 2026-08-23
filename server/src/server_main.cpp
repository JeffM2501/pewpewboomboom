#include "time_utils.h"
#include "log_system.h"
#include "text_utils.h"
#include "constants.h"
#include "raylib.h"

#include "player_state.h"
#include "player_list.h"

#include "network_manager.h"
#include "robot_ai.h"

bool Running = true;

Logger ServerLogger(ConsoleLogOutput);
NetworkManager NetManager;

void ServerSetup()
{
    ServerLogger.Log(LogLevel::Info, "Server is starting up...");

    NetManager.ServerEmpty.Add([](const bool&, void*)
        {
            Running = false;
        });

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