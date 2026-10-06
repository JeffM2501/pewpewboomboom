#include "time_utils.h"
#include "log_system.h"
#include "text_utils.h"
#include "constants.h"
#include "raylib.h"
#include "rlgl.h"

#include "player_state.h"
#include "player_list.h"

#include "network_manager.h"
#include "robot_ai.h"
#include "world_data.h"
#include "protocol.h"
#include "server_world.h"
#include "collisions.h"
#include "bullet_manager.h"

bool Running = true;


Logger ServerLogger(ConsoleLogOutput);
NetworkManager NetManager;

ServerWorld World(500);

static constexpr bool ShowDebugWindow = true;

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

	int id = 1;
	for (auto i = 0; i < 100;)
	{
		BoundingCircle bounds;

		bounds.Center = Vector2{ float(GetRandomValue(-wallSize, wallSize)), float(GetRandomValue(-wallSize, wallSize)) };
		float size = float(GetRandomValue(8, 30));
		bounds.Radius = Vector2Length(Vector2{ size, size });

		if (World.CanPlaceObject(bounds))
		{
			i++;
			auto& box = World.AddObject<ServerWorldBuilding>(bounds.Center, GetRandomValue(0, 45) * 45.0f, size);
			box.Packet.id = id++;
		}
	}

    for (auto i = 0; i < 150;)
    {
        BoundingCircle bounds;

        bounds.Center = Vector2{ float(GetRandomValue(-wallSize, wallSize)), float(GetRandomValue(-wallSize, wallSize)) };
        float size = float(GetRandomValue(100, 200))/ 100.0f;
        bounds.Radius = size;

        if (World.CanPlaceObject(bounds))
        {
            i++;
            auto& box = World.AddObject<ServerWorldBarrel>(bounds.Center, size);
            box.Packet.id = id++;
        }
    }

    for (auto i = 0; i < 100;)
    {
        BoundingCircle bounds;

        bounds.Center = Vector2{ float(GetRandomValue(-wallSize, wallSize)), float(GetRandomValue(-wallSize, wallSize)) };
        float size = float(GetRandomValue(100, 200)) / 100.0f;
        bounds.Radius = size;

        if (World.CanPlaceObject(bounds))
        {
            i++;
            auto& box = World.AddObject<ServerWorldBox>(bounds.Center, float(GetRandomValue(0, 180)), size);
            box.Packet.id = id++;
        }
    }
}

Vector2 CollidePlayerWithMap(const Vector2& oldPos, const Vector2& desiredPos, ServerPlayerList::ServerPlayer& player)
{
	BoundingCircle bounds = { desiredPos, 10 };
	Vector2 resolvedPos = World.Collide(oldPos, desiredPos, player.CollisionRadius, bounds);

	// you are dead, no moving
	if (player.IsDead)
		return oldPos;

	ServerPlayerList::DoForEachPlayer([&player, oldPos, &resolvedPos](ServerPlayerList::ServerPlayer& other)
	{
		if (other.PlayerID == player.PlayerID || other.IsDead)
		{
			return;
		}

		Vector2 intersectionPoint;
		Vector2 hitNormal;
		IntersectCircleCylinder(other.Transform.Position, other.CollisionRadius, resolvedPos, oldPos, player.CollisionRadius, intersectionPoint, hitNormal);
	}, true);

	resolvedPos = World.Collide(oldPos, resolvedPos, player.CollisionRadius, bounds);
	return resolvedPos;
}

void ResolveTankTankCollisions()
{
	std::vector<ServerPlayerList::ServerPlayer*> players;
	ServerPlayerList::DoForEachPlayer([&players](ServerPlayerList::ServerPlayer& player)
	{
		players.push_back(&player);
	}, true);

	for (size_t i = 0; i < players.size(); ++i)
	{
		for (size_t j = i + 1; j < players.size(); ++j)
		{
			auto* p1 = players[i];
			auto* p2 = players[j];

			if (p1->IsDead || p2->IsDead)
				continue;

			Vector2 hitNormal;
			float penetrationDepth = 0.0f;
			if (ResolveCircleCircleCollision(p1->Transform.Position, p1->CollisionRadius, p2->Transform.Position, p2->CollisionRadius, hitNormal, penetrationDepth))
			{
				BoundingCircle b1 = { p1->Transform.Position, 10 };
				p1->Transform.Position = World.Collide(p1->Transform.Position, p1->Transform.Position, p1->CollisionRadius, b1);

				BoundingCircle b2 = { p2->Transform.Position, 10 };
				p2->Transform.Position = World.Collide(p2->Transform.Position, p2->Transform.Position, p2->CollisionRadius, b2);
			}
		}
	}
}

void SetupPlayer(ServerPlayerList::ServerPlayer& player)
{
	player.CollisionRadius = 3.0f;
	player.Health = 100;
	player.FractionalHealth = 100.0f;
	player.WeaponCooldown = 0.0f;
	player.MachineGunCooldown = 0.0f;
	player.IsDead = false;
}

void ServerSetup()
{
	SetRandomSeed(uint32_t(std::chrono::system_clock::now().time_since_epoch().count()));
	ServerLogger.Log(LogLevel::Info, "Server is starting up...");

	BulletManager::Init();

	BulletManager::OnBulletHitBuilding.Add([](const BulletBuildingCollisionEvent& event, void*)
		{
			event.DestroyBullet = true;

			ServerLogger.Log(LogLevel::Info, "[Combat] Bullet %u (Owner %llu) hit Building %llu at (%.1f, %.1f) - Destroy: %s",
				event.Bullet ? event.Bullet->ID : 0,
				event.Bullet ? event.Bullet->OwnerID : 0,
				event.BuildingID, event.HitPoint.x, event.HitPoint.y,
				event.ShouldDestroyBullet() ? "true" : "false (overridden)");
		});

	BulletManager::OnMachineGunHitBuilding.Add([](const MachineGunHitBuildingEvent& event, void*)
		{
			ServerLogger.Log(LogLevel::Info, "[Combat] Player %llu hit Building %llu with Machine Gun at (%.1f, %.1f)",
				event.ShooterID, event.BuildingID, event.HitPoint.x, event.HitPoint.y);
		});

	BulletManager::OnMachineGunHitTank.Add([](const MachineGunHitTankEvent& event, void*)
		{
			ServerLogger.Log(LogLevel::Info, "[Combat] Player %llu hit Tank %llu with Machine Gun for %.2f dmg at (%.1f, %.1f)",
				event.ShooterID, event.TargetPlayerID, event.Damage, event.HitPoint.x, event.HitPoint.y);
		});

	PopulateWorld();

	NetManager.ServerEmpty.Add([](const bool&, void*)
		{
			Running = false;
		});

	NetManager.PlayerJoined.Add(SendWorldData);

	BulletManager::OnBulletKilledTank.Add([](const BulletTankKillEvent& event, void*)
	{
		S2C_PlayerDespawned despawnPacket;
		despawnPacket.serverTick = NetManager.CurrentServerTick;
		despawnPacket.playerId = event.VictimID;
		despawnPacket.position[0] = event.Position.x;
		despawnPacket.position[1] = event.Position.y;
		despawnPacket.reason = S2C_PlayerDespawned::Reason::Killed;
		despawnPacket.killerId = event.KillerID;
		NetManager.Broadcast(0, despawnPacket, true);
	});

	NetManager.ProcessPlayerUpdate = CollidePlayerWithMap;
	NetManager.SetupRemotePlayer = SetupPlayer;

	ServerPlayerList::OnPlayerScoreUpdate.Add([](const ServerPlayerList::ServerPlayer& player, void*)
	{
            ServerPlayerList::DoForEachPlayer([&player](const ServerPlayerList::ServerPlayer& other)
            {
                    S2C_PlayerScoreUpdate scorePacket;
                    scorePacket.playerId = player.PlayerID;
                    scorePacket.kills = player.Kills;
                    scorePacket.deaths = player.Deaths;
                    NetManager.Send(other.PlayerID, 1, scorePacket, false);
            });
	});

	if (NetManager.Initialize(7777, kMaxPlayers))
	{
		ServerLogger.Log(LogLevel::Info, "Server started successfully on port 7777 (Simulation: %d Hz, Network Send: %d Hz).", kSimulationTickRate, kNetworkSendRate);
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
    uint16_t bulletCount = uint16_t(BulletManager::GetActiveBulletCount());

    ServerPlayerList::DoForEachPlayer([&](auto& player)
        {
            S2C_BeginStateSnapshot beginSnapshot;
            beginSnapshot.snapshotTick = NetManager.CurrentServerTick;
            beginSnapshot.playerCount = uint8_t(ServerPlayerList::GetPlayerCount());
            beginSnapshot.bulletCount = bulletCount;
            NetManager.Send(player.PlayerID, 1, beginSnapshot, false);
            ServerPlayerList::DoForEachPlayer([&](auto& otherPlayer)
                {
                    S2C_PlayerSnapshot snapshot;
                    snapshot.serverTick = (otherPlayer.PlayerID == player.PlayerID && otherPlayer.LastAckedInputTick > 0) ? otherPlayer.LastAckedInputTick : NetManager.CurrentServerTick;
                    snapshot.playerId = otherPlayer.PlayerID;
                    snapshot.position[0] = otherPlayer.Transform.Position.x;
                    snapshot.position[1] = otherPlayer.Transform.Position.y;
                    snapshot.rotation[0] = otherPlayer.Transform.Rotation[0];
                    snapshot.rotation[1] = otherPlayer.Transform.Rotation[1];
                    snapshot.velocity[0] = otherPlayer.Transform.Velocity.x;
                    snapshot.velocity[1] = otherPlayer.Transform.Velocity.y;
                    snapshot.health = otherPlayer.Health;
                    snapshot.isDead = otherPlayer.IsDead ? 1 : 0;
                    NetManager.Send(player.PlayerID, 1, snapshot, false);
                }, true);

            for (const auto& bullet : BulletManager::GetBullets())
            {
                if (!bullet.Active)
                {
                    continue;
                }

                S2C_BulletSnapshot bulletSnapshot;
                bulletSnapshot.serverTick = NetManager.CurrentServerTick;
                bulletSnapshot.state.bulletId = bullet.ID;
                bulletSnapshot.state.ownerId = bullet.OwnerID;
                bulletSnapshot.state.bulletType = bullet.BulletType;
                bulletSnapshot.state.position[0] = bullet.Position.x;
                bulletSnapshot.state.position[1] = bullet.Position.y;
                bulletSnapshot.state.velocity[0] = bullet.Velocity.x;
                bulletSnapshot.state.velocity[1] = bullet.Velocity.y;

                NetManager.Send(player.PlayerID, 1, bulletSnapshot, false);
            }

            for (const auto& destroyed : BulletManager::GetDestroyedBullets())
            {
                S2C_BulletDestroyed bulletDestroyed;
                bulletDestroyed.serverTick = NetManager.CurrentServerTick;
                bulletDestroyed.bulletId = destroyed.ID;
                bulletDestroyed.position[0] = destroyed.Position.x;
                bulletDestroyed.position[1] = destroyed.Position.y;

                NetManager.Send(player.PlayerID, 1, bulletDestroyed, false);
            }

            for (const auto& created : BulletManager::GetCreatedBullets())
            {
                S2C_ShotCreated shotCreated;
                shotCreated.serverTick = NetManager.CurrentServerTick;
                shotCreated.bulletId = created.ID;
                shotCreated.ownerId = created.OwnerID;
                shotCreated.bulletType = created.BulletType;
                shotCreated.position[0] = created.Position.x;
                shotCreated.position[1] = created.Position.y;
                shotCreated.velocity[0] = created.Velocity.x;
                shotCreated.velocity[1] = created.Velocity.y;

                NetManager.Send(player.PlayerID, 1, shotCreated, false);
            }
        }, false);

    BulletManager::ClearDestroyedBullets();
    BulletManager::ClearCreatedBullets();
}

void UpdatePlayerHistories()
{
	uint64_t oldestTickToKeep = NetManager.CurrentServerTick;
	uint64_t maxTickHistory = 2 * kDefaultTickRate;

	if (oldestTickToKeep < maxTickHistory)
	{
		oldestTickToKeep = 0;
	}
	else
	{
		oldestTickToKeep -= maxTickHistory;
	}

	ServerPlayerList::DoForEachPlayer([&](ServerPlayerList::ServerPlayer& player)
	{
		player.TransformHistory[NetManager.CurrentServerTick] = player.Transform;

		for (auto itr = player.TransformHistory.begin(); itr != player.TransformHistory.end();)
		{
			if (itr->first < oldestTickToKeep)
			{
				itr = player.TransformHistory.erase(itr);
			}
			else
			{
				break;
			}
		}
	}, true);
}

void DrawDebugScene()
{
	Camera2D cam = { 0 };
	cam.zoom = GetScreenWidth() / World.Walls.Collider.Size.x;
	cam.offset.x = GetScreenWidth() * 0.5f;
    cam.offset.y = GetScreenHeight() * 0.5f;

	BeginMode2D(cam);
	DrawRectangleRec(Rectangle{ -World.Walls.Collider.Size.x, -World.Walls.Collider.Size.y, World.Walls.Collider.Size.x * 2, World.Walls.Collider.Size.y * 2 },
		DARKGRAY);

	for (const auto& obj : World.Objects)
	{
		rlPushMatrix();
		rlTranslatef(obj->Packet.position[0], obj->Packet.position[1], 0);
		rlRotatef(obj->Packet.rotation, 0, 0, 1);

		DrawRectangleRec(Rectangle{ -obj->Packet.scale, -obj->Packet.scale, obj->Packet.scale * 2.0f, obj->Packet.scale * 2.0f }, RED);
		rlPopMatrix();
	}

    ServerPlayerList::DoForEachPlayer([&](ServerPlayerList::ServerPlayer& player)
        {
			if (player.IsDead)
				return;

			DrawCircleV(player.Transform.Position, player.CollisionRadius, player.IsDead ? GRAY : BLUE);
        }
    , true);

    for (const auto& bullet : BulletManager::GetBullets())
    {
        if (bullet.Active)
        {
            DrawCircleV(bullet.Position, 1.0f, RED);
        }
    }
	EndMode2D();
	DrawText(TextFormat("Player Count %d", ServerPlayerList::GetPlayerCount()), 10, 10, 20, BLACK);
	std::string bwText = NetManager.GetBandwidthTracker().GetSummaryString();
	DrawText(bwText.c_str(), 10, 35, 20, GREEN);
}

int main(int argc, char* argv[])
{
	int robotCount = 1;

    if (argc > 1 && std::string(argv[1]) == "--robots")
    {
        robotCount = std::stoi(std::string(argv[2]));
    }
   
	ServerSetup();

	RobotAI::SetupRobots(robotCount);

	if (ShowDebugWindow)
	{
		InitWindow(800, 800, "World State");
	}

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
						if (player.WeaponCooldown > 0.0f)
						{
							player.WeaponCooldown -= (1.0f / kDefaultTickRate);
							if (player.WeaponCooldown < 0.0f)
							{
								player.WeaponCooldown = 0.0f;
							}
						}

						if (player.MachineGunCooldown > 0.0f)
						{
							player.MachineGunCooldown -= (1.0f / kDefaultTickRate);
							if (player.MachineGunCooldown < 0.0f)
							{
								player.MachineGunCooldown = 0.0f;
							}
						}

						if (player.IsDead)
						{
							player.RespawnTimer -= (1.0f / kDefaultTickRate);
							if (player.RespawnTimer <= 0.0f)
							{
								player.IsDead = false;
								player.Health = 100;
								player.FractionalHealth = 100.0f;
								player.Transform.Position = Vector2{ float(GetRandomValue(-50, 50)), float(GetRandomValue(-50, 50)) };
								BoundingCircle b = { player.Transform.Position, 10 };
								player.Transform.Position = World.Collide(player.Transform.Position, player.Transform.Position, player.CollisionRadius, b);

								S2C_PlayerSpawned spawnPacket;
								spawnPacket.serverTick = NetManager.CurrentServerTick;
								spawnPacket.playerId = player.PlayerID;
								spawnPacket.position[0] = player.Transform.Position.x;
								spawnPacket.position[1] = player.Transform.Position.y;
								spawnPacket.rotation = player.Transform.Rotation[0];
								NetManager.Broadcast(0, spawnPacket, true);
							}
						}

						player.Update(NetManager);
					}, true);

				BulletManager::Update(1.0f / kDefaultTickRate, World);

				ResolveTankTankCollisions();

				UpdatePlayerHistories();

				if ((NetManager.CurrentServerTick % kNetworkSendIntervalTicks) == 0)
				{
					SendStateUpdates();
					NetManager.Flush();
				}
			});

		// process items that can happen anytime

		NetManager.PollEvents(1);

		uint64_t nowMs = GetTimeMs();
		NetManager.UpdateBandwidth(nowMs);

		if (NetManager.GetBandwidthTracker().ShouldPrintReport(nowMs))
		{
			size_t totalPlayers = ServerPlayerList::GetPlayerCount(true);
			size_t humanPlayers = ServerPlayerList::GetPlayerCount(false);
			size_t robotCount = (totalPlayers >= humanPlayers) ? (totalPlayers - humanPlayers) : 0;
			NetManager.GetBandwidthTracker().PrintReport(ServerLogger, humanPlayers, robotCount, nowMs);
		}

		if (ShowDebugWindow)
		{
			BeginDrawing();
			ClearBackground(WHITE);
			DrawDebugScene();
			EndDrawing();
		}
	}

	if (ShowDebugWindow)
		CloseWindow();

	ServerCleanup();
	return 0;
}