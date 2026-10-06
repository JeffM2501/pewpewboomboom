#include "external/fix_win32_compatibility.h"
#include "packet_handlers.h"
#include "network_manager.h"
#include "protocol.h"
#include "player_list.h"
#include "log_system.h"
#include "time_utils.h"
#include "data_utils.h"
#include "raylib.h"
#include "world_data.h"
#include "bullet_manager.h"
#include "server_world.h"
#include "collisions.h"
#include "raymath.h"
#include <cmath>

extern Logger ServerLogger;
extern ServerWorld World;
// extern uint64_t ServerStartTimeMs;
// extern uint64_t CurrentServerTick;

namespace PacketHandlers
{
    static void ProcessC2S_Ping(PacketProcessor& processor, ENetPeer* sender, const C2S_Ping* ping)
    {
        NetworkManager& netManager = static_cast<NetworkManager&>(processor);

        S2C_Pong pong;
        pong.clientTimeMs = ping->clientTimeMs;
        pong.serverTimeMs = GetTimeMs() - netManager.ServerStartTimeMs;
        pong.serverTick = netManager.CurrentServerTick;

        processor.SendPacket(sender, 0, pong);

        ServerLogger.Log(LogLevel::Info, "Received C2S_Ping from client, replied with S2C_Pong (Server Uptime: %llu ms)", pong.serverTimeMs);
    }

    static void ProcessC2S_Goodbye(PacketProcessor& processor, ENetPeer* sender, const C2S_Goodbye* goodbye)
    {
        ServerLogger.Log(LogLevel::Info, "%x sent goodbye, reason %d.", sender->address.host, goodbye->reason);
        static_cast<NetworkManager&>(processor).RemovePlayer(sender, false);
        enet_peer_disconnect_now(sender, 0);
    }

    static std::string GetRandomName()
    {
        static const char* names[] = {
            "Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf", "Hotel",
            "India", "Juliet", "Kilo", "Lima", "Mike", "November", "Oscar", "Papa",
            "Quebec", "Romeo", "Sierra", "Tango", "Uniform", "Victor", "Whiskey",
            "X-ray", "Yankee", "Zulu"
        };
        int index = GetRandomValue(0, sizeof(names) / sizeof(names[0]) - 1);
        int index2 = GetRandomValue(0, sizeof(names) / sizeof(names[0]) - 1);

        char buffer[64];
        sprintf(buffer, "%s-%s%hx", names[index], names[index2], uint16_t(GetRandomValue(0, 65535)));
        return std::string(buffer);
    }

    static void ProcessC2S_JoinRequest(PacketProcessor& processor, ENetPeer* sender, const C2S_JoinRequest* join)
    {
        NetworkManager& netManager = static_cast<NetworkManager&>(processor);

        S2C_JoinResponse responce;

        if (!ServerPlayerList::PlayerExists(sender))
        {
            responce.result = S2C_JoinResponse::Result::Failure;
            ServerLogger.Log(LogLevel::Warning, "Received C2S_JoinRequest from unknown peer %x.", sender->address.host);
            processor.SendPacket(sender, 0, responce);
            enet_peer_disconnect_now(sender, 0);
            return;
        }

        auto& player = ServerPlayerList::GetPlayer(sender);

        CopyFixedSizeString(player.Name.Buffer(), GetRandomName().c_str(), player.Name.Capacity());

        player.Team = GetRandomValue(0, int(TeamColors::MAX) - 1);

        player.Transform.Position.x = float(GetRandomValue(-50, 50));
        player.Transform.Position.y = float(GetRandomValue(-50, 50));

        responce.result = S2C_JoinResponse::Result::Success;
        player.Name.CopyToBuffer(responce.actualName);

        responce.playerId = player.PlayerID;
        responce.team = player.Team;

        DataUtils::PackVector2(player.Transform.Position, responce.spawn);

        responce.bostMultiplier = player.Rules.BoostMultiplier;
        responce.maxSpeed = player.Rules.MaxSpeed;
        responce.turnSpeed = player.Rules.TurnSpeed;

        responce.collisionRadius = player.CollisionRadius;

        processor.SendPacket(sender, 0, responce);
        ServerLogger.Log(LogLevel::Info, "%x sent Join Response, ID %d name %s", sender->address.host, responce.playerId, responce.actualName);

        // tell the host that they joined, so that the world and other game specific data can be sent
        netManager.PlayerJoined.Invoke(player.PlayerID, &netManager);

        // send the player list to them
        ServerPlayerList::DoForEachPlayer([sender, &processor, &netManager](auto& playerInfo)
            {
                S2C_PlayerJoined remotePlayer;
                remotePlayer.playerId = playerInfo.PlayerID;
                remotePlayer.team = playerInfo.Team;
                remotePlayer.collisionRadius = playerInfo.CollisionRadius;
                playerInfo.Name.CopyToBuffer(remotePlayer.name);
                processor.SendPacket(sender, 0, remotePlayer);

               // if (!playerInfo.IsDead)
                {
                    S2C_PlayerSpawned remoteSpawn;
                    remoteSpawn.serverTick = netManager.CurrentServerTick;
                    remoteSpawn.playerId = playerInfo.PlayerID;
                    remoteSpawn.position[0] = playerInfo.Transform.Position.x;
                    remoteSpawn.position[1] = playerInfo.Transform.Position.y;
                    remoteSpawn.rotation = playerInfo.Transform.Rotation[0];
                    remoteSpawn.isDead = playerInfo.IsDead ? 1 : 0;
                    processor.SendPacket(sender, 0, remoteSpawn);
                }
            }
            , true, player.PlayerID);

        // send them to all other players
        ServerPlayerList::DoForEachPlayer([&player, &processor, &netManager](auto& playerInfo)
            {
                S2C_PlayerJoined newPlayer;
                newPlayer.playerId = player.PlayerID;
                newPlayer.collisionRadius = player.CollisionRadius;
                player.Name.CopyToBuffer(newPlayer.name);
                processor.SendPacket(playerInfo.Peer, 0, newPlayer);

                S2C_PlayerSpawned newSpawn;
                newSpawn.serverTick = netManager.CurrentServerTick;
                newSpawn.playerId = player.PlayerID;
                newSpawn.position[0] = player.Transform.Position.x;
                newSpawn.position[1] = player.Transform.Position.y;
                newSpawn.rotation = player.Transform.Rotation[0];
                newSpawn.isDead = player.IsDead ? 1 : 0;
                processor.SendPacket(playerInfo.Peer, 0, newSpawn);
            }
            , false, player.PlayerID);
    }

    static void ProcessC2S_ChatMessage(PacketProcessor& processor, ENetPeer* sender, const C2S_ChatMessage* chat) 
    {
        NetworkManager& netManager = static_cast<NetworkManager&>(processor);
        auto& player = ServerPlayerList::GetPlayer(sender);
        netManager.GetChatProcessor().PushChatMessage(player.PlayerID, chat->message);
    }

    static void ExecuteServerHitscan(NetworkManager& netManager, ServerPlayerList::ServerPlayer& shooter, uint64_t clientTick, float aimAngle);

    static void ProcessC2S_InputState(PacketProcessor& processor, ENetPeer* sender, const C2S_InputState* input)
    {
        NetworkManager& netManager = static_cast<NetworkManager&>(processor);

        auto& player = ServerPlayerList::GetPlayer(sender);

        InputState newInput;
        newInput.Foward = input->forward;
        newInput.Turn = input->turn;
        newInput.TurretAngle = input->aimDirection;
        newInput.Shoot = input->shoot;
        newInput.ShootMachineGun = input->shootMachineGun;
        newInput.Boost = input->boost;

        auto newPos = UpdatePlayerTransform(player.Transform, newInput, 1.0f/kDefaultTickRate, player.Rules);
        
        if (netManager.ProcessPlayerUpdate)
        {
            newPos = netManager.ProcessPlayerUpdate(player.Transform.Position, newPos, player);
        }

        player.Transform.Position = newPos;

        player.LastAckedInputTick = input->clientTick;

        if (newInput.Shoot && player.WeaponCooldown <= 0.0f && !player.IsDead)
        {
            float rad = newInput.TurretAngle * DEG2RAD;
            Vector2 forwardDir = { cosf(rad), sinf(rad) };
            Vector2 muzzlePos = Vector2Add(player.Transform.Position, Vector2Scale(forwardDir, player.CollisionRadius + 0.5f));

            BulletManager::SpawnBullet(player.PlayerID, muzzlePos, newInput.TurretAngle);
            player.WeaponCooldown = kRegularShotCooldown;
        }

        if (newInput.ShootMachineGun && player.MachineGunCooldown <= 0.0f && !player.IsDead)
        {
            ExecuteServerHitscan(netManager, player, input->clientTick, newInput.TurretAngle);
        }
    }

    static void LogServerMachineGun(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        printf("%s\n", buffer);
        fflush(stdout);

        FILE* f = fopen("machinegun_server.log", "a");
        if (f)
        {
            fprintf(f, "%s\n", buffer);
            fflush(f);
            fclose(f);
        }
    }

    static void ExecuteServerHitscan(NetworkManager& netManager, ServerPlayerList::ServerPlayer& shooter, uint64_t clientTick, float aimAngle)
    {
        if (shooter.IsDead)
        {
            LogServerMachineGun("[MachineGun] Player %llu rejected: Shooter is dead", shooter.PlayerID);
            return;
        }

        if (shooter.MachineGunCooldown > 0.05f)
        {
            return;
        }
        shooter.MachineGunCooldown = kMachineGunCooldown;

        if (std::isnan(aimAngle) || std::isinf(aimAngle))
        {
            aimAngle = 0.0f;
        }

        float rad = aimAngle * DEG2RAD;
        Vector2 dir = { cosf(rad), sinf(rad) };
        Vector2 muzzle = Vector2Add(shooter.Transform.Position, Vector2Scale(dir, shooter.CollisionRadius + 0.5f));

        LogServerMachineGun("[MachineGun] Server Hitscan: Shooter=%llu clientTick=%llu aim=%.1f muzzle=(%.1f, %.1f)",
            shooter.PlayerID, clientTick, aimAngle, muzzle.x, muzzle.y);

        uint64_t currentTick = netManager.CurrentServerTick;
        uint64_t baseTick = (clientTick > 0 && clientTick <= currentTick) ? clientTick : currentTick;
        uint64_t rollbackTick = (baseTick >= kRemotePlayerHistoryOffsetTicks) ? (baseTick - kRemotePlayerHistoryOffsetTicks) : 0;
        uint64_t windowStart = (rollbackTick >= kLagCompensationToleranceTicks) ? (rollbackTick - kLagCompensationToleranceTicks) : 0;
        uint64_t windowEnd = (rollbackTick + kLagCompensationToleranceTicks <= currentTick) ? (rollbackTick + kLagCompensationToleranceTicks) : currentTick;

        float maxRange = 2000.0f;
        float closestDist = maxRange;
        Vector2 closestHitPoint = Vector2Add(muzzle, Vector2Scale(dir, maxRange));
        uint64_t hitPlayerId = 0;
        uint64_t hitBuildingId = 0;

        // 1. Raycast against outer walls
        float wallDist = 0.0f;
        Vector2 wallHit = { 0.0f, 0.0f };
        if (World.Walls.GetCollider().IntersectRay(muzzle, dir, wallDist, wallHit))
        {
            if (wallDist > 0.1f && wallDist < closestDist)
            {
                closestDist = wallDist;
                closestHitPoint = wallHit;
            }
        }

        // 2. Raycast against world objects (buildings, etc.)
        for (const auto& obj : World.Objects)
        {
            const auto& bounds = obj->GetBoundingCircle();
            float boundsDist = 0.0f;
            Vector2 boundsHit = { 0.0f, 0.0f };
            if (!IntersectRayCircle(muzzle, dir, bounds.Center, bounds.Radius + 1.0f, boundsDist, boundsHit) || boundsDist > closestDist)
            {
                continue;
            }

            float objDist = 0.0f;
            Vector2 objHit = { 0.0f, 0.0f };
            if (obj->GetCollider().IntersectRay(muzzle, dir, objDist, objHit))
            {
                if (objDist > (shooter.CollisionRadius * 0.5f) && objDist < closestDist)
                {
                    closestDist = objDist;
                    closestHitPoint = objHit;
                    if (obj->GetObjectType() == S2C_SetWorldObject::ObjectType::Building)
                    {
                        hitBuildingId = obj->GetID();
                    }
                    else
                    {
                        hitBuildingId = 0;
                    }
                }
            }
        }

        // 3. Raycast against players / tanks (using server lag compensation)
        ServerPlayerList::DoForEachPlayer([&](ServerPlayerList::ServerPlayer& other)
        {
            if (other.PlayerID == shooter.PlayerID || other.IsDead)
            {
                return;
            }

            float bestOtherDist = closestDist;
            Vector2 bestOtherHit = { 0.0f, 0.0f };
            bool hitOther = false;
            float radiusTol = other.CollisionRadius * 1.35f;

            // Check historical positions across lag compensation window
            for (uint64_t t = windowStart; t <= windowEnd; ++t)
            {
                PlayerTransform pastTransform = other.GetTransformAtTick(t);
                float pDist = 0.0f;
                Vector2 pHit = { 0.0f, 0.0f };
                if (IntersectRayCircle(muzzle, dir, pastTransform.Position, radiusTol, pDist, pHit))
                {
                    if (pDist >= 0.0f && pDist < bestOtherDist)
                    {
                        hitOther = true;
                        bestOtherDist = pDist;
                        bestOtherHit = pHit;
                    }
                }
            }

            // Also check current server transform
            float curDist = 0.0f;
            Vector2 curHit = { 0.0f, 0.0f };
            if (IntersectRayCircle(muzzle, dir, other.Transform.Position, radiusTol, curDist, curHit))
            {
                if (curDist >= 0.0f && curDist < bestOtherDist)
                {
                    hitOther = true;
                    bestOtherDist = curDist;
                    bestOtherHit = curHit;
                }
            }

            // Also check direct aim alignment towards other's center
            if (!hitOther)
            {
                Vector2 toTarget = Vector2Subtract(other.Transform.Position, muzzle);
                float distToTarget = Vector2Length(toTarget);
                if (distToTarget > 0.001f && distToTarget < bestOtherDist)
                {
                    float targetAngleDeg = atan2f(toTarget.y, toTarget.x) * RAD2DEG;
                    float angleDiff = fabsf(fmodf(targetAngleDeg - aimAngle + 180.0f, 360.0f) - 180.0f);
                    float maxAngleTol = atan2f(radiusTol, distToTarget) * RAD2DEG;
                    if (maxAngleTol < 5.0f)
                    {
                        maxAngleTol = 5.0f;
                    }
                    if (angleDiff <= maxAngleTol)
                    {
                        bestOtherDist = distToTarget;
                        bestOtherHit = other.Transform.Position;
                        hitOther = true;
                    }
                }
            }

            if (hitOther && bestOtherDist < closestDist)
            {
                closestDist = bestOtherDist;
                closestHitPoint = bestOtherHit;
                hitPlayerId = other.PlayerID;
                hitBuildingId = 0;
            }
        }, true);

        // 4. Apply authoritative results
        if (hitPlayerId != 0)
        {
            auto* target = ServerPlayerList::GetPlayer(hitPlayerId);
            if (target && !target->IsDead)
            {
                if (target->FractionalHealth <= kMachineGunDamage)
                {
                    target->FractionalHealth = 0.0f;
                    target->Health = 0;
                    target->IsDead = true;
                    target->RespawnTimer = 5.0f;
                    target->Deaths++;
                    ServerPlayerList::OnPlayerScoreUpdate.Invoke(*target);
                    shooter.Kills++;
                    ServerPlayerList::OnPlayerScoreUpdate.Invoke(shooter);

                    LogServerMachineGun("[Combat] Player %llu was KILLED by Player %llu with Machine Gun (Server Detected)", target->PlayerID, shooter.PlayerID);
                    ServerLogger.Log(LogLevel::Info, "[Combat] Player %llu was KILLED by Player %llu with Machine Gun (Server Detected)", target->PlayerID, shooter.PlayerID);

                    S2C_PlayerDespawned despawnPacket;
                    despawnPacket.serverTick = netManager.CurrentServerTick;
                    despawnPacket.playerId = target->PlayerID;
                    despawnPacket.position[0] = target->Transform.Position.x;
                    despawnPacket.position[1] = target->Transform.Position.y;
                    despawnPacket.reason = S2C_PlayerDespawned::Reason::Killed;
                    despawnPacket.killerId = shooter.PlayerID;
                    netManager.Broadcast(0, despawnPacket, true);
                }
                else
                {
                    target->FractionalHealth -= kMachineGunDamage;
                    target->Health = static_cast<uint8_t>(target->FractionalHealth + 0.5f);
                    LogServerMachineGun("[Combat] Player %llu was HIT by Player %llu with Machine Gun (HP: %u, Fractional: %.2f)",
                        target->PlayerID, shooter.PlayerID, target->Health, target->FractionalHealth);
                    ServerLogger.Log(LogLevel::Info, "[Combat] Player %llu was hit by Player %llu with Machine Gun (HP: %u)",
                        target->PlayerID, shooter.PlayerID, target->Health);
                }

                MachineGunHitTankEvent tankEvent;
                tankEvent.ShooterID = shooter.PlayerID;
                tankEvent.TargetPlayerID = target->PlayerID;
                tankEvent.HitPoint = closestHitPoint;
                tankEvent.Damage = kMachineGunDamage;
                tankEvent.Distance = closestDist;
                BulletManager::OnMachineGunHitTank.Invoke(tankEvent, &netManager);
                netManager.OnMachineGunHitTank.Invoke(tankEvent, &netManager);
            }
        }
        else if (hitBuildingId != 0)
        {
            LogServerMachineGun("[Combat] Player %llu hit Building %llu with Machine Gun at distance %.1f", shooter.PlayerID, hitBuildingId, closestDist);
            ServerLogger.Log(LogLevel::Info, "[Combat] Player %llu hit Building %llu with Machine Gun at distance %.1f", shooter.PlayerID, hitBuildingId, closestDist);

            MachineGunHitBuildingEvent buildingEvent;
            buildingEvent.ShooterID = shooter.PlayerID;
            buildingEvent.BuildingID = hitBuildingId;
            buildingEvent.HitPoint = closestHitPoint;
            buildingEvent.Distance = closestDist;
            BulletManager::OnMachineGunHitBuilding.Invoke(buildingEvent, &netManager);
            netManager.OnMachineGunHitBuilding.Invoke(buildingEvent, &netManager);
        }
        else
        {
            LogServerMachineGun("[Combat] Player %llu shot missed/hit terrain at distance %.1f", shooter.PlayerID, closestDist);
        }

        // 5. Broadcast authoritative hitscan effect to ALL clients
        S2C_HitscanEffect effect;
        effect.shooterId = shooter.PlayerID;
        effect.targetPlayerId = hitPlayerId;
        effect.hitBuildingId = hitBuildingId;
        effect.startPoint[0] = muzzle.x;
        effect.startPoint[1] = muzzle.y;
        effect.endPoint[0] = closestHitPoint.x;
        effect.endPoint[1] = closestHitPoint.y;

        ServerPlayerList::DoForEachPlayer([&](auto& otherPlayer)
        {
            netManager.Send(otherPlayer.PlayerID, 0, effect, true);
        }, false);
    }

    static void ProcessC2S_HitscanShot(PacketProcessor& processor, ENetPeer* sender, const C2S_HitscanShot* shot)
    {
        NetworkManager& netManager = static_cast<NetworkManager&>(processor);

        if (!ServerPlayerList::PlayerExists(sender))
        {
            return;
        }

        auto& shooter = ServerPlayerList::GetPlayer(sender);
        ExecuteServerHitscan(netManager, shooter, shot->clientTick, shot->aimAngle);
    }

    void RegisterAll(PacketProcessor& processor)
    {
        processor.RegisterProcessor<C2S_Ping>(PacketType::C2S_Ping, ProcessC2S_Ping);
        processor.RegisterProcessor<C2S_Goodbye>(PacketType::C2S_Goodbye, ProcessC2S_Goodbye);
        processor.RegisterProcessor<C2S_JoinRequest>(PacketType::C2S_JoinRequest, ProcessC2S_JoinRequest);
        processor.RegisterProcessor<C2S_ChatMessage>(PacketType::C2S_ChatMessage, ProcessC2S_ChatMessage);
        processor.RegisterProcessor<C2S_InputState>(PacketType::C2S_InputState, ProcessC2S_InputState);
        processor.RegisterProcessor<C2S_HitscanShot>(PacketType::C2S_HitscanShot, ProcessC2S_HitscanShot);
    }
}
