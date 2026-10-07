#include "bullet_manager.h"
#include "server_world.h"
#include "player_list.h"
#include "collisions.h"
#include "constants.h"
#include <cmath>

namespace BulletManager
{
    EventSource<BulletBuildingCollisionEvent> OnBulletHitBuilding;
    EventSource<MachineGunHitBuildingEvent> OnMachineGunHitBuilding;
    EventSource<MachineGunHitTankEvent> OnMachineGunHitTank;
    EventSource<BulletTankKillEvent> OnBulletKilledTank;

    EventSource<BulletBuildingCollisionEvent>& GetOnBulletHitBuilding()
    {
        return OnBulletHitBuilding;
    }

    EventSource<MachineGunHitBuildingEvent>& GetOnMachineGunHitBuilding()
    {
        return OnMachineGunHitBuilding;
    }

    EventSource<MachineGunHitTankEvent>& GetOnMachineGunHitTank()
    {
        return OnMachineGunHitTank;
    }

    EventSource<BulletTankKillEvent>& GetOnBulletKilledTank()
    {
        return OnBulletKilledTank;
    }

    static std::array<ServerBullet, kMaxBullets> BulletPool;
    static uint16_t NextBulletID = 1;
    static std::vector<DestroyedBulletInfo> DestroyedBulletsThisTick;
    static std::vector<CreatedBulletInfo> CreatedBulletsThisTick;
    static uint64_t CurrentTick = 0;

    void SetCurrentTick(uint64_t tick)
    {
        CurrentTick = tick;
    }

    void DestroyBullet(ServerBullet& bullet, Vector2 impactPos)
    {
        if (bullet.Active)
        {
            bullet.Active = false;
            DestroyedBulletsThisTick.push_back(DestroyedBulletInfo{ bullet.ID, impactPos });
        }
    }

    const std::vector<DestroyedBulletInfo>& GetDestroyedBullets()
    {
        return DestroyedBulletsThisTick;
    }

    void ClearDestroyedBullets()
    {
        DestroyedBulletsThisTick.clear();
    }

    const std::vector<CreatedBulletInfo>& GetCreatedBullets()
    {
        return CreatedBulletsThisTick;
    }

    void ClearCreatedBullets()
    {
        CreatedBulletsThisTick.clear();
    }

    void Init()
    {
        Clear();
    }

    void Clear()
    {
        for (auto& bullet : BulletPool)
        {
            bullet.Active = false;
        }
        NextBulletID = 1;
        DestroyedBulletsThisTick.clear();
        CreatedBulletsThisTick.clear();
    }

    ServerBullet* SpawnBullet(uint64_t ownerId, Vector2 muzzlePos, float angleDeg, float speed, float damage, uint8_t type)
    {
        for (auto& bullet : BulletPool)
        {
            if (!bullet.Active)
            {
                bullet.Active = true;
                bullet.ID = NextBulletID++;
                if (NextBulletID == 0)
                {
                    NextBulletID = 1;
                }
                bullet.OwnerID = ownerId;
                bullet.BulletType = type;
                bullet.Position = muzzlePos;
                bullet.Damage = damage;
                bullet.Radius = kBulletRadius;
                bullet.Lifetime = kBulletLifetime;

                float rad = angleDeg * DEG2RAD;
                bullet.Velocity = Vector2{ cosf(rad) * speed, sinf(rad) * speed };
                CreatedBulletsThisTick.push_back(CreatedBulletInfo{ bullet.ID, bullet.OwnerID, bullet.BulletType, bullet.Position, bullet.Velocity, CurrentTick });
                return &bullet;
            }
        }
        return nullptr;
    }

    void Update(float deltaTime, ServerWorld& world)
    {
        for (auto& bullet : BulletPool)
        {
            if (!bullet.Active)
            {
                continue;
            }

            bullet.Lifetime -= deltaTime;
            if (bullet.Lifetime <= 0.0f)
            {
                DestroyBullet(bullet, bullet.Position);
                continue;
            }

            Vector2 nextPos = Vector2Add(bullet.Position, Vector2Scale(bullet.Velocity, deltaTime));

            // 1. Check collision against outer walls
            Vector2 hitPoint;
            Vector2 hitNormal;
            if (world.Walls.Collider.IntersectPath(nextPos, bullet.Position, bullet.Radius, hitPoint, hitNormal))
            {
                DestroyBullet(bullet, hitPoint);
                continue;
            }

            // 2. Check collision against world obstacles
            bool hitObstacle = false;
            Vector2 obstacleHitPoint = nextPos;
            float travelDist = Vector2Distance(bullet.Position, nextPos);
            Vector2 sweepCenter = Vector2Scale(Vector2Add(bullet.Position, nextPos), 0.5f);
            BoundingCircle bulletBounds = { sweepCenter, travelDist * 0.5f + bullet.Radius + 1.0f };

            world.DoForEachServerObject(bulletBounds, [&](ServerWorldObject& obj)
            {
                if (hitObstacle)
                {
                    return;
                }

                if (obj.GetObjectType() == S2C_SetWorldObject::ObjectType::Building)
                {
                    auto* building = static_cast<ServerWorldBuilding*>(&obj);
                    Vector2 hitPoint = { 0.0f, 0.0f };
                    Vector2 hitNormal = { 0.0f, 0.0f };
                    Vector2 buildingPos = building->GetPosition();
                    Vector2 buildingSize = { building->GetScale(), building->GetScale() };
                    float buildingRot = building->GetRotation();

                    if (CheckBulletBuildingCollision(bullet.Position, nextPos, bullet.Radius, buildingPos, buildingSize, buildingRot, hitPoint, hitNormal))
                    {
                        BulletBuildingCollisionEvent eventData;
                        eventData.Bullet = &bullet;
                        eventData.Building = building;
                        eventData.BuildingID = building->GetID();
                        eventData.HitPoint = hitPoint;
                        eventData.HitNormal = hitNormal;
                        eventData.DestroyBullet = true;

                        OnBulletHitBuilding.Invoke(eventData, &world);

                        if (eventData.ShouldDestroyBullet())
                        {
                            hitObstacle = true;
                            obstacleHitPoint = hitPoint;
                            return;
                        }
                        else
                        {
                            bullet.Position = Vector2Add(hitPoint, Vector2Scale(hitNormal, 0.1f));
                            return;
                        }
                    }
                }
                else if (obj.GetObjectType() != S2C_SetWorldObject::ObjectType::Walls)
                {
                    Vector2 intersection;
                    Vector2 normal;
                    Vector2 testPos = nextPos;
                    if (obj.GetCollider().IntersectPath(testPos, bullet.Position, bullet.Radius, intersection, normal))
                    {
                        hitObstacle = true;
                        obstacleHitPoint = intersection;
                        return;
                    }
                }
            });

            if (hitObstacle)
            {
                DestroyBullet(bullet, obstacleHitPoint);
                continue;
            }

            // 3. Check collision against players
            bool hitPlayer = false;
            ServerPlayerList::DoForEachPlayer([&](ServerPlayerList::ServerPlayer& player)
            {
                if (hitPlayer || player.PlayerID == bullet.OwnerID || player.IsDead)
                {
                    return;
                }

                if (CheckCollisionCircles(nextPos, bullet.Radius, player.Transform.Position, player.CollisionRadius))
                {
                    hitPlayer = true;
                    DestroyBullet(bullet, nextPos);

                    if (player.FractionalHealth <= bullet.Damage)
                    {
                        player.FractionalHealth = 0.0f;
                        player.Health = 0;
                        player.IsDead = true;
                        player.RespawnTimer = 5.0f;
                        player.Deaths++;
                        ServerPlayerList::OnPlayerScoreUpdate.Invoke(player);

                        auto* attacker = ServerPlayerList::GetPlayer(bullet.OwnerID);
                        if (attacker)
                        {
                            attacker->Kills++;

                            ServerPlayerList::OnPlayerScoreUpdate.Invoke(*attacker);
                        }

                        BulletTankKillEvent killEvent;
                        killEvent.VictimID = player.PlayerID;
                        killEvent.KillerID = bullet.OwnerID;
                        killEvent.Position = player.Transform.Position;
                        OnBulletKilledTank.Invoke(killEvent, &world);
                    }
                    else
                    {
                        player.FractionalHealth -= bullet.Damage;
                        player.Health = static_cast<uint8_t>(std::ceil(player.FractionalHealth));
                    }
                }
            }, true);

            if (!hitPlayer)
            {
                bullet.Position = nextPos;
            }
        }
    }

    size_t GetActiveBulletCount()
    {
        size_t count = 0;
        for (const auto& bullet : BulletPool)
        {
            if (bullet.Active)
            {
                count++;
            }
        }
        return count;
    }

    const std::array<ServerBullet, kMaxBullets>& GetBullets()
    {
        return BulletPool;
    }
}
