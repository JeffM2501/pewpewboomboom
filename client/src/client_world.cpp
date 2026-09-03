#include "client_world.h"
#include "raymath.h"
#include "rlgl.h"

// ClientWorldWalls
ClientWorldWalls::ClientWorldWalls(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Bounds.Radius = message.scale;
    Bounds.Center = Vector2{ 0, 0 };
}

bool ClientWorldWalls::Intersects(const BoundingCircle& other) const
{
    Vector2 otherMin = { other.Center.x - other.Radius, other.Center.y - other.Radius };
    Vector2 otherMax = { other.Center.x + other.Radius, other.Center.y + other.Radius };

    if (otherMin.x < -Bounds.Radius / 2.0f || otherMax.x > Bounds.Radius / 2.0f || otherMin.y < -Bounds.Radius / 2.0f || otherMax.y > Bounds.Radius / 2.0f)
    {
        return true;
    }

    return false;
}

void ClientWorldWalls::Draw()
{
    Rectangle rect{
        -Bounds.Radius,
        -Bounds.Radius,
        Bounds.Radius * 2.0f,
        Bounds.Radius * 2.0f
    };
    DrawRectangleLinesEx(rect, 10, Tint);
}

// ClientWorldBuilding
ClientWorldBuilding::ClientWorldBuilding(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Size.x = message.scale;
    Size.y = message.scale;
}

bool ClientWorldBuilding::Intersects(const BoundingCircle& other) const
{
    Vector2 delta = other.Center - Bounds.Center;

    Rectangle rect{
       -Size.x,
       -Size.y,
       Size.x * 2.0f,
       Size.y * 2.0f
    };

    Vector2 rotatedDelta = Vector2Rotate(delta, -Rotation);
    return CheckCollisionCircleRec(rotatedDelta, other.Radius, rect);
}

void ClientWorldBuilding::Draw()
{
    Rectangle rect{
      -Size.x,
      -Size.y,
      Size.x * 2.0f,
      Size.y * 2.0f
    };
    DrawRectangleRec(rect, Tint);
}

// ClientWorldBox
ClientWorldBox::ClientWorldBox(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Size.x = message.scale;
    Size.y = message.scale;
}

bool ClientWorldBox::Intersects(const BoundingCircle& other) const
{
    Vector2 delta = other.Center - Bounds.Center;

    Rectangle rect{
       -Size.x,
       -Size.y,
       Size.x * 2.0f,
       Size.y * 2.0f
    };

    Vector2 rotatedDelta = Vector2Rotate(delta, -Rotation);
    return CheckCollisionCircleRec(rotatedDelta, other.Radius, rect);
}

void ClientWorldBox::Draw()
{
    Rectangle rect{
     -Size.x,
     -Size.y,
     Size.x * 2.0f,
     Size.y * 2.0f
    };
    DrawRectangleRec(rect, Tint);
}

// ClientWorldBarrel
ClientWorldBarrel::ClientWorldBarrel(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
}

bool ClientWorldBarrel::Intersects(const BoundingCircle& other) const
{
    return CheckCollisionCircles(Bounds.Center, Bounds.Radius, other.Center, other.Radius);
}

void ClientWorldBarrel::Draw()
{
    DrawCircleV(Bounds.Center, Bounds.Radius, Tint);
}

// ClientWorld
void ClientWorld::Init(const S2C_SetWorldInfo& message)
{
    Name = message.name;
    Count = message.objectCount;
    Objects.clear();

    WorldStarted.Invoke(*this);
}

void ClientWorld::Receive(const S2C_SetWorldObject& message)
{
    switch (message.objType)
    {
    case S2C_SetWorldObject::ObjectType::Walls:
        Walls = std::make_unique<ClientWorldWalls>(message);
		WallSize.x = message.scale;
        WallSize.y = message.scale;
        break;
    case S2C_SetWorldObject::ObjectType::Building:
        Objects.push_back(std::make_unique<ClientWorldBuilding>(message));
        break;
    case S2C_SetWorldObject::ObjectType::Box:
        Objects.push_back(std::make_unique<ClientWorldBox>(message));
        break;
    case S2C_SetWorldObject::ObjectType::Barrel:
        Objects.push_back(std::make_unique<ClientWorldBarrel>(message));
        break;
    default:
        break;
    }

    if (Objects.size() + (Walls ? 1 : 0) == Count)
    {
        WorldFinalized.Invoke(*this);
    }
}

void ClientWorld::Draw()
{
	for (auto& object : Objects)
	{
		rlPushMatrix();
		rlTranslatef(object->Bounds.Center.x, object->Bounds.Center.y, 0);
		rlRotatef(object->Rotation, 0, 0, 1);

		object->Draw();
		rlPopMatrix();
	}
}