#include "server_world.h"

#include "raylib.h"
#include "raymath.h"

// ServerWorldWalls
ServerWorldWalls::ServerWorldWalls(float size)
{
	Packet.objType = S2C_SetWorldObject::ObjectType::Walls;
	Packet.scale = size;
	Packet.rotation = 0.0f;
	Packet.position[0] = 0.0f;
	Packet.position[1] = 0.0f;

	Packet.color[0] = 255;
	Packet.color[1] = 255;
	Packet.color[2] = 255;
	Packet.color[3] = 255;

	Bounds.Center.x = 0.0f;
	Bounds.Center.y = 0.0f;
	Bounds.Radius = size;

	Collider.Size = Vector2{ size,size };
}

// ServerWorldBuilding
ServerWorldBuilding::ServerWorldBuilding(Vector2 position, float rotation, float size, Color tint)
{
	Packet.objType = S2C_SetWorldObject::ObjectType::Building;
	Packet.position[0] = position.x;
	Packet.position[1] = position.y;

	Packet.rotation = rotation;

	Packet.scale = size;

	Packet.color[0] = tint.r;
	Packet.color[1] = tint.g;
	Packet.color[2] = tint.b;
	Packet.color[3] = tint.a;

    Collider.Size = Vector2{ size, size };
    Collider.Rotation = rotation;
	Collider.Bounds.Center.x = position.x;
	Collider.Bounds.Center.y = position.y;
	Collider.Bounds.Radius = Vector2Length(Vector2{ size, size });

	BoundingRect.x = -size;
	BoundingRect.y = -size ;
	BoundingRect.width = size * 2;
	BoundingRect.height = size * 2;
}

// ServerWorldBox
ServerWorldBox::ServerWorldBox(Vector2 position, float rotation, float size, Color tint)
{
	Packet.objType = S2C_SetWorldObject::ObjectType::Box;
	Packet.position[0] = position.x;
	Packet.position[1] = position.y;

	Packet.rotation = rotation;

	Packet.scale = size;

	Packet.color[0] = tint.r;
	Packet.color[1] = tint.g;
	Packet.color[2] = tint.b;
	Packet.color[3] = tint.a;

	Collider.Size = Vector2{ size, size };
	Collider.Rotation = rotation;
	Collider.Bounds.Center.x = position.x;
	Collider.Bounds.Center.y = position.y;
	Collider.Bounds.Radius = Vector2Length(Vector2{ size, size });

	BoundingRect.x = -size;
	BoundingRect.y = -size;
	BoundingRect.width = size * 2;
	BoundingRect.height = size * 2;
}

// ServerWorldBarrel
ServerWorldBarrel::ServerWorldBarrel(Vector2 position, float size, Color tint)
{
	Packet.objType = S2C_SetWorldObject::ObjectType::Barrel;
	Packet.position[0] = position.x;
	Packet.position[1] = position.y;
	Packet.rotation = 0.0f;
	Packet.scale = size;
	Packet.color[0] = tint.r;
	Packet.color[1] = tint.g;
	Packet.color[2] = tint.b;
	Packet.color[3] = tint.a;
	Collider.Bounds.Center.x = position.x;
	Collider.Bounds.Center.y = position.y;
	Collider.Bounds.Radius = size;
}

// ServerWorld
bool ServerWorld::CanPlaceObject(BoundingCircle& bounds) const
{
	if (Walls.GetCollider().Intersects(bounds))
	{
		return false;
	}

	for (const auto& obj : Objects)
	{
		if (obj->GetCollider().Intersects(bounds))
		{
			return false;
		}
	}
	return true;
}

void ServerWorld::DoForEachObject(BoundingCircle& area, std::function<void(WorldObject& object)> func)
{
    for (auto& object : Objects)
    {
        if (CheckCollisionCircles(area.Center, area.Radius, object->GetBoundingCircle().Center, object->GetBoundingCircle().Radius))
        {
            func(*object.get());
        }
    }

	if (Walls.GetCollider().Intersects(area))
	{
		func(Walls);
	}
}