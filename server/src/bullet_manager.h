#pragma once

#include "raylib.h"
#include "raymath.h"
#include <array>
#include <cstdint>
#include <cstddef>
#include "event_source.h"

#include <vector>

class ServerWorld;
class ServerWorldBuilding;

struct ServerBullet
{
    uint16_t ID = 0;
    uint64_t OwnerID = 0;
    uint8_t BulletType = 0;
    Vector2 Position = { 0.0f, 0.0f };
    Vector2 Velocity = { 0.0f, 0.0f };
    float Damage = 25.0f;
    float Radius = 0.5f;
    float Lifetime = 3.0f;
    bool Active = false;
};

struct DestroyedBulletInfo
{
    uint16_t ID = 0;
    Vector2 Position = { 0.0f, 0.0f };
};

struct BulletBuildingCollisionEvent
{
    ServerBullet* Bullet = nullptr;
    ServerWorldBuilding* Building = nullptr;
    uint64_t BuildingID = 0;
    Vector2 HitPoint = { 0.0f, 0.0f };
    Vector2 HitNormal = { 0.0f, 0.0f };
    mutable bool DestroyBullet = true;

    void KeepAlive() const
    {
        DestroyBullet = false;
    }

    void SetDestroyBullet(bool destroy) const
    {
        DestroyBullet = destroy;
    }

    bool ShouldDestroyBullet() const
    {
        return DestroyBullet;
    }
};

namespace BulletManager
{
    static constexpr size_t kMaxBullets = 256;

    extern EventSource<BulletBuildingCollisionEvent> OnBulletHitBuilding;

    void Init();
    ServerBullet* SpawnBullet(uint64_t ownerId, Vector2 muzzlePos, float angleDeg, float speed = 50.0f, float damage = 25.0f, uint8_t type = 0);
    void Update(float deltaTime, ServerWorld& world);
    void DestroyBullet(ServerBullet& bullet, Vector2 impactPos);
    size_t GetActiveBulletCount();
    const std::array<ServerBullet, kMaxBullets>& GetBullets();
    const std::vector<DestroyedBulletInfo>& GetDestroyedBullets();
    void ClearDestroyedBullets();
    void Clear();

    EventSource<BulletBuildingCollisionEvent>& GetOnBulletHitBuilding();
    bool CheckBulletBuildingCollision(Vector2 startPos, Vector2 endPos, float radius, Vector2 buildingPos, Vector2 buildingSize, float rotationDeg, Vector2& outHitPoint, Vector2& outHitNormal);
}

