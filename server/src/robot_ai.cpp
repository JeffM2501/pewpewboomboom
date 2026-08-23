#include "robot_ai.h"
#include "network_manager.h"

namespace RobotAI
{
    void UpdateRobot(ServerPlayerList::ServerPlayer& robot, NetworkManager& manager)
    {
        auto tick = manager.CurrentServerTick;


        //manager.SendPacket(nullptr)
    }

    void SetupRobots()
    {
        auto& robot = ServerPlayerList::AddRobotPlayer();
        robot.UpdateFunctions.emplace_back(RobotAI::UpdateRobot);
        robot.Name = "Theta (Robot)";
        robot.Team = -1;
        robot.Transform.Position = Vector2{ 20, 20 };
    }
}
