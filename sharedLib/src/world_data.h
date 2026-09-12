#pragma once

#include "protocol.h"
#include "raylib.h"
#include "raymath.h"

#include <vector>
#include <functional>

struct BoundingCircle
{
	Vector2 Center = Vector2{ 0, 0 };
	float Radius = 0;
};

class WorldObject;

class WorldObjectCollider
{
public:
	virtual bool Intersects(const WorldObject& other) const;

    virtual bool Intersects(const BoundingCircle& other) const = 0;

    virtual bool IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const = 0;
};

class WorldObject
{
public:
	virtual float GetRotation() const { return 0; }

	virtual const BoundingCircle& GetBoundingCircle() const = 0;
	virtual const WorldObjectCollider& GetCollider() const = 0;
};

class RectangleColliderObject : public WorldObjectCollider
{
public:
	BoundingCircle Bounds;
	Vector2 Size = Vector2Zeros;
	float Rotation = 0;
public:

	bool Intersects(const BoundingCircle& other) const override;
	bool IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const override;
};

class CircleColliderObject : public WorldObjectCollider
{
public:
    BoundingCircle Bounds;

public:
    bool Intersects(const BoundingCircle& other) const override;
    bool IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const override;
};

class WallColliderObject : public WorldObjectCollider
{
public:
	Vector2 Size = Vector2Zeros;
public:
    bool Intersects(const BoundingCircle& other) const override;
    bool IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const override;
};

class WorldObjectItterator
{
public:
	virtual void DoForEachObject(BoundingCircle& area, std::function<void(WorldObject& object)> func) = 0;

	Vector2 Collide(Vector2 startPos, Vector2 desiredPos, BoundingCircle& bounds);
};
