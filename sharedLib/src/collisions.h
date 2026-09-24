
#pragma once

#include "raylib.h"

void PointNearestRectanglePoint(Rectangle rect, Vector2 point, Vector2& nearest, Vector2& normal);
bool IntersectBBoxCircle(Rectangle bounds, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal);
bool IntersectBBoxCylinder(Rectangle bounds, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal);

void PointNearestCirclePoint(Vector2 circleCenter, float circleRadius, Vector2 point, Vector2& nearest, Vector2& normal);
bool IntersectCircleCylinder(Vector2 circleCenter, float circleRadius, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal);
bool ResolveCircleCircleCollision(Vector2& posA, float radiusA, Vector2& posB, float radiusB, Vector2& hitNormal, float& penetrationDepth);
