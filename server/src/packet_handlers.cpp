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
#include <cmath>

extern Logger ServerLogger;
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

        player.Name = join->desriredName;

        player.Transform.Position.x = float(GetRandomValue(-50, 50));
        player.Transform.Position.y = float(GetRandomValue(-50, 50));

        responce.result = S2C_JoinResponse::Result::Success;
        player.Name.CopyToBuffer(responce.actualName);

        responce.playerId = player.PlayerID;
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
        ServerPlayerList::DoForEachPlayer([sender,&processor](auto& playerInfo)
            {
                S2C_PlayerJoined remotePlayer;
                remotePlayer.playerId = playerInfo.PlayerID;
                remotePlayer.collisionRadius = playerInfo.CollisionRadius;
                playerInfo.Name.CopyToBuffer(remotePlayer.name);
                processor.SendPacket(sender, 0, remotePlayer);
            }
            , true, player.PlayerID);

        // send them to all other players
        ServerPlayerList::DoForEachPlayer([&player,&processor](auto& playerInfo)
            {
                S2C_PlayerJoined newPlayer;
                newPlayer.playerId = player.PlayerID;
                newPlayer.collisionRadius = player.CollisionRadius;
                player.Name.CopyToBuffer(newPlayer.name);
                processor.SendPacket(playerInfo.Peer, 0, newPlayer);
            }
            , false, player.PlayerID);
    }

    static void ProcessC2S_ChatMessage(PacketProcessor& processor, ENetPeer* sender, const C2S_ChatMessage* chat) 
    {
        NetworkManager& netManager = static_cast<NetworkManager&>(processor);
        auto& player = ServerPlayerList::GetPlayer(sender);
        netManager.GetChatProcessor().PushChatMessage(player.PlayerID, chat->message);
    }

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
            newPos = netManager.ProcessPlayerUpdate(player.Transform.Position, newPos, player);

        player.Transform.Position = newPos;

        player.TransformHistory[input->clientTick] = player.Transform;
        player.LastAckedInputTick = input->clientTick;

        if (newInput.Shoot && player.WeaponCooldown <= 0.0f && !player.IsDead)
        {
            float rad = newInput.TurretAngle * DEG2RAD;
            Vector2 forwardDir = { cosf(rad), sinf(rad) };
            Vector2 muzzlePos = Vector2Add(player.Transform.Position, Vector2Scale(forwardDir, player.CollisionRadius + 0.5f));

            BulletManager::SpawnBullet(player.PlayerID, muzzlePos, newInput.TurretAngle);
            player.WeaponCooldown = kRegularShotCooldown;
        }
    }

    static void ProcessC2S_HitscanShot(PacketProcessor& processor, ENetPeer* sender, const C2S_HitscanShot* shot)
    {
        NetworkManager& netManager = static_cast<NetworkManager&>(processor);
        if (!ServerPlayerList::PlayerExists(sender))
        {
            return;
        }

        auto& shooter = ServerPlayerList::GetPlayer(sender);
        if (shooter.IsDead)
        {
            return;
        }

        if (shooter.MachineGunCooldown > 0.15f)
        {
            return;
        }
        shooter.MachineGunCooldown = kMachineGunCooldown;

        Vector2 muzzle = { shot->muzzlePos[0], shot->muzzlePos[1] };
        Vector2 hit = { shot->hitPoint[0], shot->hitPoint[1] };
        float dist = Vector2Distance(muzzle, hit);

        if (shot->targetPlayerId != 0)
        {
            auto* target = ServerPlayerList::GetPlayer(shot->targetPlayerId);
            if (target && !target->IsDead)
            {
                if (target->FractionalHealth <= kMachineGunDamage)
                {
                    target->FractionalHealth = 0.0f;
                    target->Health = 0;
                    target->IsDead = true;
                    target->RespawnTimer = 5.0f;
                    target->Deaths++;
                    shooter.Kills++;
                    ServerLogger.Log(LogLevel::Info, "[Combat] Player %llu was killed by Player %llu with Machine Gun", target->PlayerID, shooter.PlayerID);
                }
                else
                {
                    target->FractionalHealth -= kMachineGunDamage;
                    target->Health = static_cast<uint8_t>(std::ceil(target->FractionalHealth));
                    ServerLogger.Log(LogLevel::Info, "[Combat] Player %llu was hit by Player %llu with Machine Gun (HP: %u)", target->PlayerID, shooter.PlayerID, target->Health);
                }

                MachineGunHitTankEvent tankEvent;
                tankEvent.ShooterID = shooter.PlayerID;
                tankEvent.TargetPlayerID = target->PlayerID;
                tankEvent.HitPoint = hit;
                tankEvent.Damage = kMachineGunDamage;
                tankEvent.Distance = dist;
                BulletManager::OnMachineGunHitTank.Invoke(tankEvent, &netManager);
                netManager.OnMachineGunHitTank.Invoke(tankEvent, &netManager);
            }
        }

        if (shot->hitBuildingId != 0)
        {
            MachineGunHitBuildingEvent buildingEvent;
            buildingEvent.ShooterID = shooter.PlayerID;
            buildingEvent.BuildingID = shot->hitBuildingId;
            buildingEvent.HitPoint = hit;
            buildingEvent.Distance = dist;
            BulletManager::OnMachineGunHitBuilding.Invoke(buildingEvent, &netManager);
            netManager.OnMachineGunHitBuilding.Invoke(buildingEvent, &netManager);
        }

        S2C_HitscanEffect effect;
        effect.shooterId = shooter.PlayerID;
        effect.targetPlayerId = shot->targetPlayerId;
        effect.hitBuildingId = shot->hitBuildingId;
        effect.startPoint[0] = shot->muzzlePos[0];
        effect.startPoint[1] = shot->muzzlePos[1];
        effect.endPoint[0] = shot->hitPoint[0];
        effect.endPoint[1] = shot->hitPoint[1];

        ServerPlayerList::DoForEachPlayer([&](auto& otherPlayer)
        {
            netManager.Send(otherPlayer.PlayerID, 1, effect, false);
        }, false, shooter.PlayerID);
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
