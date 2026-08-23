#include "external/fix_win32_compatibility.h"

#include <stdio.h>

#include "enet.h"

#include "time_utils.h"
#include "log_system.h"
#include "protocol.h"
#include "text_utils.h"
#include "constants.h"
#include "raylib.h"

#include "player_state.h"
#include "player_list.h"

#include "network_manager.h"
#include "robot_ai.h"

bool Running = true;

uint64_t ServerStartTimeMs = 0;


Logger ServerLogger(ConsoleLogOutput);

static constexpr double ServerTickTime = double(kDefaultTickRate);
static constexpr int ServerTickTimeMS = static_cast<int>(1.0f / ServerTickTime * 1000.0f);

FixedTickAccumulator ServerTick(ServerTickTime);

uint64_t CurrentServerTick = 0;

NetworkManager NetManager;

void ServerSetup()
{
    ServerStartTimeMs = GetTimeMs();
    ServerLogger.Log(LogLevel::Info, "Server is starting up...");

    if (NetManager.Initialize(7777, kMaxPlayers))
    {
        ServerLogger.Log(LogLevel::Info, "Server started successfully on port 7777.");
    }
    else
    {
        ServerLogger.Log(LogLevel::Error, "Failed to start server on port 7777");
    }

    auto& robot = ServerPlayerList::AddRobotPlayer();
    robot.UpdateFunctions.emplace_back(RobotAI::UpdateRobot);
    robot.Name = "Theta (Robot)";
    robot.Team = -1;
    robot.Transform.Position = Vector2{ 20, 20 };
}

void ServerCleanup()
{
    NetManager.Shutdown();
    ServerLogger.Log(LogLevel::Info, "Server is shutdown...");
}

void RemovePlayer(ENetPeer* peer, bool isDisconnect)
{
    if (!ServerPlayerList::PlayerExists(peer))
    {
        return;
    }

    auto& player = ServerPlayerList::GetPlayer(peer);

    auto playerID = player.PlayerID;

    ServerLogger.Log(LogLevel::Info, "Removing player %s (ID: %llu) from server.", player.Name.Data(), playerID);
    ServerPlayerList::RemovePlayer(peer);

    ServerPlayerList::DoForEachPlayer([playerID, isDisconnect](auto& playerInfo)
        {
            S2C_PlayerDisconnected deadPlayer;
            deadPlayer.playerId = playerID;
            deadPlayer.reason = isDisconnect ? S2C_PlayerDisconnected::Reason::Dissconnect : S2C_PlayerDisconnected::Reason::Quit;
            NetManager.SendPacket(playerInfo.Peer, 0, deadPlayer);
        });
}

void ServerNetUpdate(double deltaTime)
{
    CurrentServerTick++;

    // see if the players have update tasks
    ServerPlayerList::DoForEachPlayer([](auto& player)
        {
            player.Update();
        }, true);

    ServerLogger.Log(LogLevel::Verbose, "ServerNetUpdate: Waiting for events dt(%0.1f)ms", deltaTime * 1000);

    NetManager.PollEvents(ServerTickTimeMS);
}

int main(int argc, char* argv[])
{
    ServerSetup();

    while (Running && NetManager.GetHost())
    {
        ServerTick.ProcessTicks([](auto deltaTime)
            {
                ServerNetUpdate(deltaTime);
            });
    }

    ServerCleanup();
    return 0;
}