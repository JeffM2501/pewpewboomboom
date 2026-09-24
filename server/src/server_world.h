#pragma once

#include "world_data.h"
#include "text_utils.h"
#include <vector>
#include <memory>

class ServerWorldObject : public WorldObject
{
public:
	S2C_SetWorldObject Packet;

	virtual const BoundingCircle& GetBoundingCircle() const = 0;
	virtual const WorldObjectCollider& GetCollider() const = 0;
};

class ServerWorldWalls : public ServerWorldObject
{
public:
	WallColliderObject Collider;
	BoundingCircle Bounds;

	ServerWorldWalls(float size);

	const WorldObjectCollider& GetCollider() const override { return Collider; }

	const BoundingCircle& GetBoundingCircle() const override { return Bounds; }
};

class ServerWorldBuilding : public ServerWorldObject
{
private:
	Rectangle BoundingRect = { 0 };
	RectangleColliderObject Collider;
public:
	ServerWorldBuilding(Vector2 position, float rotation, float size = 10, Color tint = WHITE);
	const WorldObjectCollider& GetCollider() const override { return Collider; }
	const BoundingCircle& GetBoundingCircle() const override { return Collider.Bounds; }
	float GetRotation() const override { return Collider.Rotation; }
};

class ServerWorldBox : public ServerWorldObject
{
private:
	Rectangle BoundingRect = { 0 };
	RectangleColliderObject Collider;
public:
	ServerWorldBox(Vector2 position, float rotation, float size = 2, Color tint = WHITE);
	const WorldObjectCollider& GetCollider() const override { return Collider; }
	const BoundingCircle& GetBoundingCircle() const override { return Collider.Bounds; }
	float GetRotation() const override { return Collider.Rotation; }
};

class ServerWorldBarrel : public ServerWorldObject
{
	CircleColliderObject Collider;
public:
	ServerWorldBarrel(Vector2 position, float size = 1, Color tint = WHITE);
	const WorldObjectCollider& GetCollider() const override { return Collider; }
	const BoundingCircle& GetBoundingCircle() const override { return Collider.Bounds; }
};

class ServerWorld : public WorldObjectItterator
{
public:
	S2C_SetWorldInfo InfoPacket;

	ServerWorldWalls Walls;

	ServerWorld(float wallSize = 500) : Walls(wallSize)
	{
		CopyFixedSizeString(InfoPacket.name, TextFormat("Default World %d", (int)wallSize), kMaxChatLineSize);
		InfoPacket.objectCount = 1; // always the walls
	}

	std::vector<std::unique_ptr<ServerWorldObject>> Objects;

	template<typename T, typename... Args>
	T& AddObject(Args&&... args)
	{
		auto object = std::make_unique<T>(std::forward<Args>(args)...);
		Objects.push_back(std::move(object));

		InfoPacket.objectCount++;
		return *(static_cast<T*>(Objects.back().get()));
	}

	bool CanPlaceObject(BoundingCircle& bounds) const;

    void DoForEachObject(BoundingCircle& area, std::function<void(WorldObject& object)> func) override;
};