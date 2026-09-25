#include "bullet_manager.h"
#include "server_world.h"
#include "player_list.h"
#include "collisions.h"
#include <cmath>

namespace BulletManager
{
    EventSource<BulletBuildingCollisionEvent> OnBulletHitBuilding;

    EventSource<BulletBuildingCollisionEvent>& GetOnBulletHitBuilding()
    {
        return OnBulletHitBuilding;
    }

    bool CheckBulletBuildingCollision(Vector2 startPos, Vector2 endPos, float radius, Vector2 buildingPos, Vector2 buildingSize, float rotationDeg, Vector2& outHitPoint, Vector2& outHitNormal)
    {
        Vector2 rel0 = Vector2Subtract(startPos, buildingPos);
        Vector2 localP0 = Vector2Rotate(rel0, -rotationDeg * DEG2RAD);

        Vector2 rel1 = Vector2Subtract(endPos, buildingPos);
        Vector2 localP1 = Vector2Rotate(rel1, -rotationDeg * DEG2RAD);

        Vector2 halfSize = buildingSize;
        Vector2 expandedHalfSize = { halfSize.x + radius, halfSize.y + radius };

        if (fabsf(localP0.x) <= expandedHalfSize.x && fabsf(localP0.y) <= expandedHalfSize.y)
        {
            float dxLeft = fabsf(localP0.x - (-halfSize.x));
            float dxRight = fabsf(localP0.x - halfSize.x);
            float dyTop = fabsf(localP0.y - (-halfSize.y));
            float dyBottom = fabsf(localP0.y - halfSize.y);

            float minD = dxLeft;
            Vector2 localNorm = { -1.0f, 0.0f };
            if (dxRight < minD)
            {
                minD = dxRight;
                localNorm = { 1.0f, 0.0f };
            }
            if (dyTop < minD)
            {
                minD = dyTop;
                localNorm = { 0.0f, -1.0f };
            }
            if (dyBottom < minD)
            {
                minD = dyBottom;
                localNorm = { 0.0f, 1.0f };
            }

            Vector2 clampedLocal = {
                fmaxf(-halfSize.x, fminf(halfSize.x, localP0.x)),
                fmaxf(-halfSize.y, fminf(halfSize.y, localP0.y))
            };
            outHitPoint = Vector2Add(buildingPos, Vector2Rotate(clampedLocal, rotationDeg * DEG2RAD));
            outHitNormal = Vector2Rotate(localNorm, rotationDeg * DEG2RAD);
            return true;
        }

        Vector2 dir = Vector2Subtract(localP1, localP0);
        float tMin = 0.0f;
        float tMax = 1.0f;
        Vector2 hitNormLocal = { 0.0f, 0.0f };

        if (fabsf(dir.x) < 1e-6f)
        {
            if (localP0.x < -expandedHalfSize.x || localP0.x > expandedHalfSize.x)
            {
                return false;
            }
        }
        else
        {
            float invD = 1.0f / dir.x;
            float t1 = (-expandedHalfSize.x - localP0.x) * invD;
            float t2 = (expandedHalfSize.x - localP0.x) * invD;
            Vector2 n1 = { -1.0f, 0.0f };
            Vector2 n2 = { 1.0f, 0.0f };
            if (t1 > t2)
            {
                std::swap(t1, t2);
                std::swap(n1, n2);
            }
            if (t1 > tMin)
            {
                tMin = t1;
                hitNormLocal = n1;
            }
            if (t2 < tMax)
            {
                tMax = t2;
            }
            if (tMin > tMax)
            {
                return false;
            }
        }

        if (fabsf(dir.y) < 1e-6f)
        {
            if (localP0.y < -expandedHalfSize.y || localP0.y > expandedHalfSize.y)
            {
                return false;
            }
        }
        else
        {
            float invD = 1.0f / dir.y;
            float t1 = (-expandedHalfSize.y - localP0.y) * invD;
            float t2 = (expandedHalfSize.y - localP0.y) * invD;
            Vector2 n1 = { 0.0f, -1.0f };
            Vector2 n2 = { 0.0f, 1.0f };
            if (t1 > t2)
            {
                std::swap(t1, t2);
                std::swap(n1, n2);
            }
            if (t1 > tMin)
            {
                tMin = t1;
                hitNormLocal = n1;
            }
            if (t2 < tMax)
            {
                tMax = t2;
            }
            if (tMin > tMax)
            {
                return false;
            }
        }

        if (tMin >= 0.0f && tMin <= 1.0f)
        {
            Vector2 localHit = Vector2Add(localP0, Vector2Scale(dir, tMin));
            Vector2 clampedLocal = {
                fmaxf(-halfSize.x, fminf(halfSize.x, localHit.x)),
                fmaxf(-halfSize.y, fminf(halfSize.y, localHit.y))
            };
            outHitPoint = Vector2Add(buildingPos, Vector2Rotate(clampedLocal, rotationDeg * DEG2RAD));
            outHitNormal = Vector2Rotate(hitNormLocal, rotationDeg * DEG2RAD);
            return true;
        }

        return false;
    }

    static std::array<ServerBullet, kMaxBullets> BulletPool;
    static uint16_t NextBulletID = 1;
    static std::vector<DestroyedBulletInfo> DestroyedBulletsThisTick;

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
                bullet.Radius = 0.5f;
                bullet.Lifetime = 3.0f;

                float rad = angleDeg * DEG2RAD;
                bullet.Velocity = Vector2{ cosf(rad) * speed, sinf(rad) * speed };
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

                    if (player.Health <= uint8_t(bullet.Damage))
                    {
                        player.Health = 0;
                        player.IsDead = true;
                        player.RespawnTimer = 5.0f;
                        player.Deaths++;

                        auto* attacker = ServerPlayerList::GetPlayer(bullet.OwnerID);
                        if (attacker)
                        {
                            attacker->Kills++;
                        }
                    }
                    else
                    {
                        player.Health -= uint8_t(bullet.Damage);
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
