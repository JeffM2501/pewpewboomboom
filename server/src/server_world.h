#pragma once

#include "world_data.h"
#include <vector>
#include <memory>

class ServerWorldObject : public WorldObject
{
protected:
	BoundingCircle Bounds;

public:
	S2C_SetWorldObject Packet;

	BoundingCircle& GetBoundingCircle() override { return Bounds; }
};

class ServerWorldWalls : ServerWorldObject
{
public:
	ServerWorldWalls(float size);

	bool Intersects(const WorldObject& other) const override;
};

class ServerWorldBuilding : ServerWorldObject
{
public:
	ServerWorldBuilding(Vector2 position, float rotation, float size = 10, Color tint = BEIGE);
	bool Intersects(const WorldObject& other) const override;
};

class ServerWorldBox : ServerWorldObject
{
public:
	ServerWorldBox(Vector2 position, float rotation, float size = 2, Color tint = BROWN);
	bool Intersects(const WorldObject& other) const override;
};

class ServerWorldBarrel : ServerWorldObject
{
public:
	ServerWorldBarrel(Vector2 position, float size = 1, Color tint = GREEN);
	bool Intersects(const WorldObject& other) const override;
};

class ServerWorld
{
public:
	S2C_SetWorldInfo InfoPacket;

	std::vector<std::unique_ptr<ServerWorldObject>> Objects;

	bool CanPlaceObject(ServerWorldObject* object) const;
};