#include "robot_ai.h"
#include "network_manager.h"
#include "player_state.h"

Vector2 CollidePlayerWithMap(const Vector2& oldPos, const Vector2& desiredPos, ServerPlayerList::ServerPlayer& player);

namespace RobotAI
{
    constexpr uint64_t AIStateInfoID = 10;

    class AIStateInfo : public ServerPlayerList::ServerPlayerExtraData
    {
    public:
        float ForwardTime = 0;
        float TurnTime = 0;
        bool LastTurnPositive = 0;

        InputState Input;
    };

    void UpdateRobot(ServerPlayerList::ServerPlayer& robot, NetworkManager& manager)
    {
        auto tick = manager.CurrentServerTick;

        float deltaTime = (1.0f / kDefaultTickRate);

        AIStateInfo* aiInfo = static_cast<AIStateInfo*>(robot.ExtensionData[AIStateInfoID].get());

        aiInfo->ForwardTime -= deltaTime;
        if (aiInfo->ForwardTime <= 0)
        {
            aiInfo->ForwardTime = float(GetRandomValue(1, 10));
            aiInfo->Input.Foward = GetRandomValue(100, 800) / 1000.0f;
        }

        aiInfo->TurnTime -= deltaTime;
        if (aiInfo->TurnTime <= 0)
        {
            aiInfo->TurnTime = float(GetRandomValue(1, 6));
            aiInfo->Input.Turn = GetRandomValue(-1000, 1000) / 1000.0f;

            aiInfo->LastTurnPositive = !aiInfo->LastTurnPositive;
        }

        aiInfo->Input.TurretAngle = robot.Transform.Rotation[1] + ((aiInfo->LastTurnPositive ? 1 : -1) * 45 * deltaTime);

        auto newPos = UpdatePlayerTransform(robot.Transform, aiInfo->Input, deltaTime, robot.Rules);

        robot.Transform.Position = CollidePlayerWithMap(robot.Transform.Position, newPos, robot);

        robot.LastAckedInputTick = tick;
        robot.TransformHistory[tick] = robot.Transform;
    }

    void SetupRobots()
    {
        auto& robot = ServerPlayerList::AddRobotPlayer();
        robot.UpdateFunctions.emplace_back(RobotAI::UpdateRobot);
        robot.Name = "Theta (Robot)";
        robot.Team = -1;
        robot.CollisionRadius = 3.0f;
        robot.Transform.Position = Vector2{ 20, 20 };

        robot.ExtensionData.insert_or_assign(AIStateInfoID, std::make_unique<AIStateInfo>());
    }
}
