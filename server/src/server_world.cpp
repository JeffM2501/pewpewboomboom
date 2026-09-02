#include "server_world.h"

#include "server_world.h"

// ServerWorldWalls
ServerWorldWalls::ServerWorldWalls(float size)
{
}

bool ServerWorldWalls::Intersects(const WorldObject& other) const
{
	return false;
}

// ServerWorldBuilding
ServerWorldBuilding::ServerWorldBuilding(Vector2 position, float rotation, float size, Color tint)
{
}

bool ServerWorldBuilding::Intersects(const WorldObject& other) const
{
	return false;
}

// ServerWorldBox
ServerWorldBox::ServerWorldBox(Vector2 position, float rotation, float size, Color tint)
{
}

bool ServerWorldBox::Intersects(const WorldObject& other) const
{
	return false;
}

// ServerWorldBarrel
ServerWorldBarrel::ServerWorldBarrel(Vector2 position, float size, Color tint)
{
}

bool ServerWorldBarrel::Intersects(const WorldObject& other) const
{
	return false;
}

// ServerWorld
bool ServerWorld::CanPlaceObject(ServerWorldObject* object) const
{
	return true;
}