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
	Bounds.Radius = sqrtf((size / 2.0f) * (size / 2.0f));
}

bool ServerWorldWalls::Intersects(const WorldObject& other) const
{
	return Intersects(other.GetBoundingCircle());
}

bool ServerWorldWalls::Intersects(const BoundingCircle& other) const
{
	Vector2 otherMin = { other.Center.x - other.Radius, other.Center.y - other.Radius };
	Vector2 otherMax = { other.Center.x + other.Radius, other.Center.y + other.Radius };

	if (otherMin.x < -Packet.scale / 2.0f || otherMax.x > Packet.scale / 2.0f || otherMin.y < -Packet.scale / 2.0f || otherMax.y > Packet.scale / 2.0f)
	{
		return true;
	}

	return false;
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

	Bounds.Center.x = position.x;
	Bounds.Center.y = position.y;
	Bounds.Radius = sqrtf((size / 2.0f) * (size / 2.0f));

	BoundingRect.x = -size / 2.0f;
	BoundingRect.y = -size / 2.0f;
	BoundingRect.width = size;
	BoundingRect.height = size;
}

bool ServerWorldBuilding::Intersects(const WorldObject& other) const
{
	return Intersects(other.GetBoundingCircle());
}

bool ServerWorldBuilding::Intersects(const BoundingCircle& other) const
{
	Vector2 delta = other.Center - Bounds.Center;

	Vector2 rotatedDelta = Vector2Rotate(delta, -Packet.rotation);
	return CheckCollisionCircleRec(rotatedDelta, other.Radius, BoundingRect);
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

	Bounds.Center.x = position.x;
	Bounds.Center.y = position.y;
	Bounds.Radius = sqrtf((size / 2.0f) * (size / 2.0f));

	BoundingRect.x = -size / 2.0f;
	BoundingRect.y = -size / 2.0f;
	BoundingRect.width = size;
	BoundingRect.height = size;
}

bool ServerWorldBox::Intersects(const WorldObject& other) const
{
	return Intersects(other.GetBoundingCircle());
}

bool ServerWorldBox::Intersects(const BoundingCircle& other) const
{
	Vector2 delta = other.Center - Bounds.Center;

	Vector2 rotatedDelta = Vector2Rotate(delta, -Packet.rotation);
	return CheckCollisionCircleRec(rotatedDelta, other.Radius, BoundingRect);
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
	Bounds.Center.x = position.x;
	Bounds.Center.y = position.y;
	Bounds.Radius = sqrtf((size / 2.0f) * (size / 2.0f));
}

bool ServerWorldBarrel::Intersects(const WorldObject& other) const
{
	return Intersects(other.GetBoundingCircle());
}

bool ServerWorldBarrel::Intersects(const BoundingCircle& other) const
{
	return CheckCollisionCircles(Bounds.Center, Bounds.Radius, other.Center, other.Radius);
}

// ServerWorld
bool ServerWorld::CanPlaceObject(BoundingCircle& bounds) const
{
	if (Walls.Intersects(bounds))
	{
		return false;
	}

	for (const auto& obj : Objects)
	{
		if (obj->Intersects(bounds))
		{
			return false;
		}
	}
	return true;
}