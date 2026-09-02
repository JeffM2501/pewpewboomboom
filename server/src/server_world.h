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

	const BoundingCircle& GetBoundingCircle() const override { return Bounds; }
};

class ServerWorldWalls : public ServerWorldObject
{
public:
	ServerWorldWalls(float size);

	bool Intersects(const WorldObject& other) const override;
};

class ServerWorldBuilding : public ServerWorldObject
{
private:
	Rectangle BoundingRect = { 0 };

public:
	ServerWorldBuilding(Vector2 position, float rotation, float size = 10, Color tint = BEIGE);
	bool Intersects(const WorldObject& other) const override;
};

class ServerWorldBox : public ServerWorldObject
{
private:
	Rectangle BoundingRect = { 0 };

public:
	ServerWorldBox(Vector2 position, float rotation, float size = 2, Color tint = BROWN);
	bool Intersects(const WorldObject& other) const override;
};

class ServerWorldBarrel : public ServerWorldObject
{
public:
	ServerWorldBarrel(Vector2 position, float size = 1, Color tint = GREEN);
	bool Intersects(const WorldObject& other) const override;
};

class ServerWorld
{
public:
	S2C_SetWorldInfo InfoPacket;

	ServerWorldWalls Walls;

	ServerWorld(float wallSize = 500) : Walls(wallSize) {}

	std::vector<std::unique_ptr<ServerWorldObject>> Objects;

	template<typename T, typename... Args>
	T& AddObject(Args&&... args)
	{
		auto object = std::make_unique<T>(std::forward<Args>(args)...);
		Objects.push_back(std::move(object));
		return *(static_cast<T*>(Objects.back().get()));
	}

	bool CanPlaceObject(ServerWorldObject* object) const;

};