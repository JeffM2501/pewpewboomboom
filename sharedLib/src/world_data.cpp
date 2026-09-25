#include "world_data.h"
#include "collisions.h"

bool WorldObjectCollider::Intersects(const WorldObject& other) const
{
    const BoundingCircle& circle = other.GetBoundingCircle();
    return Intersects(circle);
}

Vector2 WorldObjectItterator::Collide(Vector2 startPos, Vector2 desiredPos, float colliderRadius, BoundingCircle& bounds)
{
    Vector2 endPos = desiredPos;
    DoForEachObject(bounds, [&endPos, startPos, colliderRadius](WorldObject& object)
        {
            Vector2 intersction;
            Vector2 normal;
            object.GetCollider().IntersectPath(endPos, startPos, colliderRadius, intersction, normal);
        });

    return endPos;
}

bool RectangleColliderObject::Intersects(const BoundingCircle& other) const
{
    Vector2 delta = other.Center - Bounds.Center;

    Rectangle rect{
       -Size.x,
       -Size.y,
       Size.x * 2.0f,
       Size.y * 2.0f
    };

    Vector2 rotatedDelta = Vector2Rotate(delta, -Rotation * DEG2RAD);
    return CheckCollisionCircleRec(rotatedDelta, other.Radius, rect);
}

bool RectangleColliderObject::IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const
{
    Vector2 delta = currentPosition - Bounds.Center;

    Rectangle rect{
       -Size.x,
       -Size.y,
       Size.x * 2.0f,
       Size.y * 2.0f
    };

    Vector2 rotatedDelta = Vector2Rotate(delta, -Rotation * DEG2RAD);

    bool hit = IntersectBBoxCircle(rect, rotatedDelta, initalPosition, radius, intersectionPoint, hitNormal);
    if (hit)
    {
        currentPosition = Bounds.Center + Vector2Rotate(rotatedDelta, Rotation * DEG2RAD);
        intersectionPoint = Bounds.Center + Vector2Rotate(intersectionPoint, Rotation * DEG2RAD);
        hitNormal = Vector2Rotate(hitNormal, Rotation * DEG2RAD);
    }

    return hit;
}

bool CircleColliderObject::Intersects(const BoundingCircle& other) const
{
    return CheckCollisionCircles(Bounds.Center, Bounds.Radius, other.Center, other.Radius);
}

bool CircleColliderObject::IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const
{
    return IntersectCircleCylinder(Bounds.Center, Bounds.Radius, currentPosition, initalPosition, radius, intersectionPoint, hitNormal);
}

bool WallColliderObject::Intersects(const BoundingCircle& other) const
{
    Vector2 otherMin = { other.Center.x - other.Radius, other.Center.y - other.Radius };
    Vector2 otherMax = { other.Center.x + other.Radius, other.Center.y + other.Radius };

    if (otherMin.x < -Size.x || otherMax.x > Size.x || otherMin.y < -Size.y || otherMax.y > Size.y)
    {
        return true;
    }

    return false;
}

bool WallColliderObject::IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const
{
    bool hit = false;
    intersectionPoint = currentPosition;
    hitNormal = Vector2Zeros;

    if (currentPosition.x > Size.x - radius)
    {
        currentPosition.x = Size.x - radius;
        intersectionPoint.x = Size.x;
        hitNormal.x = -1.0f;
        hit = true;
    }
    else if (currentPosition.x < -Size.x + radius)
    {
        currentPosition.x = -Size.x + radius;
        intersectionPoint.x = -Size.x;
        hitNormal.x = 1.0f;
        hit = true;
    }

    if (currentPosition.y > Size.y - radius)
    {
        currentPosition.y = Size.y - radius;
        intersectionPoint.y = Size.y;
        hitNormal.y = -1.0f;
        hit = true;
    }
    else if (currentPosition.y < -Size.y + radius)
    {
        currentPosition.y = -Size.y + radius;
        intersectionPoint.y = -Size.y;
        hitNormal.y = 1.0f;
        hit = true;
    }

    if (hit && Vector2LengthSqr(hitNormal) > 1.0f)
    {
        hitNormal = Vector2Normalize(hitNormal);
    }

    return hit;
}

bool RectangleColliderObject::IntersectRay(Vector2 rayOrigin, Vector2 rayDir, float& outDist, Vector2& outHitPoint) const
{
	return IntersectRayOBB(rayOrigin, rayDir, Bounds.Center, Size, Rotation, outDist, outHitPoint);
}

bool CircleColliderObject::IntersectRay(Vector2 rayOrigin, Vector2 rayDir, float& outDist, Vector2& outHitPoint) const
{
	return IntersectRayCircle(rayOrigin, rayDir, Bounds.Center, Bounds.Radius, outDist, outHitPoint);
}

bool WallColliderObject::IntersectRay(Vector2 rayOrigin, Vector2 rayDir, float& outDist, Vector2& outHitPoint) const
{
	Vector2 boxMin = { -Size.x, -Size.y };
	Vector2 boxMax = { Size.x, Size.y };
	return IntersectRayBoxExit(rayOrigin, rayDir, boxMin, boxMax, outDist, outHitPoint);
}
