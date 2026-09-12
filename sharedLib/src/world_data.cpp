#include "world_data.h"
#include "collisions.h"

bool WorldObjectCollider::Intersects(const WorldObject& other) const
{
    const BoundingCircle& circle = other.GetBoundingCircle();
    return Intersects(circle);
}

Vector2 WorldObjectItterator::Collide(Vector2 startPos, Vector2 desiredPos, BoundingCircle& bounds)
{
    Vector2 endPos = desiredPos;
    DoForEachObject(bounds, [&endPos, startPos, bounds](WorldObject& object)
        {
            Vector2 intersction;
            Vector2 normal;
            object.GetCollider().IntersectPath(endPos, startPos, bounds.Radius, intersction, normal);
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

    Vector2 rotatedDelta = Vector2Rotate(delta, -Rotation);
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

    Vector2 rotatedDelta = Vector2Rotate(delta, -Rotation);

    bool hit = IntersectBBoxCylinder(rect, delta, initalPosition, radius, intersectionPoint, hitNormal);
    if (hit)
    {
        currentPosition = Bounds.Center + delta;
    }

    return hit;
}

bool CircleColliderObject::Intersects(const BoundingCircle& other) const
{
    return CheckCollisionCircles(Bounds.Center, Bounds.Radius, other.Center, other.Radius);
}

bool CircleColliderObject::IntersectPath(Vector2& currentPosition, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal) const
{
    Vector2 delta = currentPosition - Bounds.Center;

    bool hit = IntersectCircleCylinder(Vector2Zeros, Bounds.Radius, delta, initalPosition, radius, intersectionPoint, hitNormal);
    if (hit)
    {
        currentPosition = Bounds.Center + delta;
    }

    return hit;
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
    Vector2 delta = currentPosition - initalPosition;

    bool hit = false;
    intersectionPoint = currentPosition;
    hitNormal = Vector2Zeros;

    if (delta.x > 0)
    {
        if (currentPosition.x > Size.x - radius)
        {
            currentPosition.x = Size.x - radius;
            intersectionPoint.x = Size.x;
            hitNormal.x = -1;
            hit = true;
        }
    }
    else
    {
        if (currentPosition.x < -Size.x + radius)
        {
            currentPosition.x = -Size.x + radius;
            intersectionPoint.x = -Size.x;
            hitNormal.x = 1;
            hit = true;
        }
    }

    if (delta.y > 0)
    {
        if (currentPosition.y > Size.y - radius)
        {
            currentPosition.y = Size.y - radius;
            intersectionPoint.y = Size.y;
            hitNormal.y = -1;
            hit = true;
        }
    }
    else
    {
        if (currentPosition.y < -Size.y + radius)
        {
            currentPosition.y = -Size.y + radius;
            intersectionPoint.y = -Size.y;
            hitNormal.y = 1;
            hit = true;
        }
    }

    return hit;
}
