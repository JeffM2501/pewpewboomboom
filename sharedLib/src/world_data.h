#pragma once

#include "protocol.h"
#include "raylib.h"

#include <vector>

struct BoundingCircle
{
	Vector2 Center = Vector2{ 0, 0 };
	float Radius = 0;
};

class WorldObject
{
public:
	virtual const BoundingCircle& GetBoundingCircle() const = 0;

	virtual bool Intersects(const WorldObject& other) const = 0;
};
