#include "client_world.h"
#include "raymath.h"
#include "rlgl.h"

// ClientWorldWalls
ClientWorldWalls::ClientWorldWalls(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Size.x = message.scale;
    Collider.Size.y = message.scale;

    Bounds.Radius = message.scale;
}

void ClientWorldWalls::Draw()
{
    Rectangle rect{
        -(Collider.Size.x),
        -(Collider.Size.y),
        Collider.Size.x * 2.0f,
        Collider.Size.y * 2.0f
    };
    DrawRectangleLinesEx(rect, 10, Tint);
}

// ClientWorldBuilding
ClientWorldBuilding::ClientWorldBuilding(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Size.x = message.scale;
    Collider.Size.y = message.scale;

    Collider.Bounds.Center.x = message.position[0];
    Collider.Bounds.Center.y = message.position[1];
    Collider.Bounds.Radius = Vector2Length(Vector2(message.scale, message.scale));

    Collider.Rotation = message.rotation;
}

void ClientWorldBuilding::Draw()
{
    Rectangle rect{
      -Collider.Size.x,
      -Collider.Size.y,
      Collider.Size.x * 2.0f,
      Collider.Size.y * 2.0f
    };
    DrawRectangleRec(rect, Tint);
}

// ClientWorldBox
ClientWorldBox::ClientWorldBox(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Size.x = message.scale;
    Collider.Size.y = message.scale;

    Collider.Bounds.Center.x = message.position[0];
    Collider.Bounds.Center.y = message.position[1];
    Collider.Bounds.Radius = Vector2Length(Vector2(message.scale, message.scale));

    Collider.Rotation = message.rotation;
}

void ClientWorldBox::Draw()
{
    Rectangle rect{
     -Collider.Size.x,
     -Collider.Size.y,
     Collider.Size.x * 2.0f,
     Collider.Size.y * 2.0f
    };
    DrawRectangleRec(rect, Tint);
}

// ClientWorldBarrel
ClientWorldBarrel::ClientWorldBarrel(const S2C_SetWorldObject& message) : ClientWorldObject(message)
{
    Collider.Bounds.Center.x = message.position[0];
    Collider.Bounds.Center.y = message.position[1];
    Collider.Bounds.Radius = Vector2Length(Vector2(message.scale, message.scale));
}

void ClientWorldBarrel::Draw()
{
    DrawCircleV(Collider.Bounds.Center, Collider.Bounds.Radius, Tint);
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
    if (Walls)
    {
        Walls->Draw();
    }

    for (auto& object : Objects)
	{
		rlPushMatrix();
		rlTranslatef(object->GetBoundingCircle().Center.x, object->GetBoundingCircle().Center.y, 0);
		rlRotatef(object->GetRotation(), 0, 0, 1);

		object->Draw();
		rlPopMatrix();
	}
}

void ClientWorld::DoForEachObject(BoundingCircle& area, std::function<void(WorldObject& object)> func)
{
    for (auto& object : Objects)
    {
        if (CheckCollisionCircles(area.Center, area.Radius, object->GetBoundingCircle().Center, object->GetBoundingCircle().Radius))
            func(*object.get());
    }
}