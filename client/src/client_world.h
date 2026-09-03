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
    BoundingCircle Bounds;
    Color Tint = WHITE;
    float Rotation = 0.0f;


    ClientWorldObject(const S2C_SetWorldObject& message)
    {
        Bounds.Center.x = message.position[0];
        Bounds.Center.y = message.position[1];
        Bounds.Radius = sqrtf(message.scale*message.scale + message.scale*message.scale);
        Rotation = message.rotation;

        Tint = Color{ message.color[0], message.color[1], message.color[2], message.color[3] };
    }

    const BoundingCircle& GetBoundingCircle() const override { return Bounds; }

    virtual void Draw() = 0;

    bool Intersects(const WorldObject& other) const override
    {
        const BoundingCircle& circle = other.GetBoundingCircle();
        return Intersects(circle);
    }

    virtual bool Intersects(const BoundingCircle& other) const = 0;
};

class ClientWorldWalls : public ClientWorldObject
{
public:
    ClientWorldWalls(const S2C_SetWorldObject& message);

    bool Intersects(const BoundingCircle& other) const override;

    void Draw() override;
};

class ClientWorldBuilding : public ClientWorldObject
{
    Vector2 Size = { 0, 0 };

public:
    ClientWorldBuilding(const S2C_SetWorldObject& message);

    bool Intersects(const BoundingCircle& other) const override;

    void Draw() override;
};

class ClientWorldBox : public ClientWorldObject
{
    Vector2 Size = { 0, 0 };
    float Rotation = 0.0f;

public:
    ClientWorldBox(const S2C_SetWorldObject& message);

    bool Intersects(const BoundingCircle& other) const override;

    void Draw() override;
};

class ClientWorldBarrel : public ClientWorldObject
{
public:
    ClientWorldBarrel(const S2C_SetWorldObject& message);

    bool Intersects(const BoundingCircle& other) const override;

    void Draw() override;
};

class ClientWorld
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
};