#pragma once

#include "world_data.h"
#include "protocol.h"

#include "event_source.h"
#include "raylib.h"
#include <vector>
#include <memory>
#include <string>

class ClientWorldObject : public WorldObject
{
public:
    Color Tint = WHITE;

    ClientWorldObject(const S2C_SetWorldObject& message)
    {
        Tint = Color{ message.color[0], message.color[1], message.color[2], message.color[3] };
    }

    virtual const BoundingCircle& GetBoundingCircle() const = 0;

    virtual const WorldObjectCollider& GetCollider() const = 0;

    virtual void Draw() = 0;
};

class ClientWorldWalls : public ClientWorldObject
{
protected:
    WallColliderObject Collider;
    BoundingCircle Bounds;
public:
    ClientWorldWalls(const S2C_SetWorldObject& message);

    const WorldObjectCollider& GetCollider() const override {  return Collider; }

    const BoundingCircle& GetBoundingCircle() const override { return Bounds; }

    void Draw() override;
};

class ClientWorldBuilding : public ClientWorldObject
{
    RectangleColliderObject Collider;

public:
    ClientWorldBuilding(const S2C_SetWorldObject& message);

    const WorldObjectCollider& GetCollider() const override { return Collider; }
    const BoundingCircle& GetBoundingCircle() const override { return Collider.Bounds; }

    float GetRotation() const override { return Collider.Rotation; }

    void Draw() override;
};

class ClientWorldBox : public ClientWorldObject
{
    RectangleColliderObject Collider;

public:
    ClientWorldBox(const S2C_SetWorldObject& message);

    const WorldObjectCollider& GetCollider() const override { return Collider; }
    const BoundingCircle& GetBoundingCircle() const override { return Collider.Bounds; }

    float GetRotation() const override { return Collider.Rotation; }

    void Draw() override;
};

class ClientWorldBarrel : public ClientWorldObject
{
    CircleColliderObject Collider;
public:
    ClientWorldBarrel(const S2C_SetWorldObject& message);

    const WorldObjectCollider& GetCollider() const override { return Collider; }
    const BoundingCircle& GetBoundingCircle() const override { return Collider.Bounds; }

    void Draw() override;
};

class ClientWorld : public WorldObjectItterator
{
public:
    std::string Name;
    uint64_t Count;
    std::unique_ptr<ClientWorldWalls> Walls;
    std::vector<std::unique_ptr<ClientWorldObject>> Objects;

    Vector2 WallSize = { 0,0 };
    
    void Init(const S2C_SetWorldInfo& message);
    void Receive(const S2C_SetWorldObject& message);

    void Draw();

	bool IsValid() const { return Walls != nullptr && Count-1 == Objects.size(); }

    bool Loading = false;

    EventSource<ClientWorld> WorldStarted;
    EventSource<ClientWorld> WorldFinalized;

    void DoForEachObject(BoundingCircle& area, std::function<void(WorldObject& object)> func) override;
};