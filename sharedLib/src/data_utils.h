#pragma once

#include "raylib.h"
#include "raymath.h"

namespace DataUtils
{
	void PackVector2(const Vector2& vec, float array[2])
	{
		array[0] = vec.x;
		array[1] = vec.y;
	}

	Vector2 UnpackVector2(const float array[2])
	{
		return { array[0], array[1] };
	}
}