#include <limits>

#include "collisions.h"
#include "raylib.h"
#include "raymath.h"

// check if a cylinder hits a bounding box
bool IntersectBBoxCircle(Rectangle bounds, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal)
{
    float minX = bounds.x;
    float maxX = bounds.x + bounds.width;
    if (minX > maxX)
    {
        float temp = minX;
        minX = maxX;
        maxX = temp;
    }

    float minY = bounds.y;
    float maxY = bounds.y + bounds.height;
    if (minY > maxY)
    {
        float temp = minY;
        minY = maxY;
        maxY = temp;
    }

    Vector2 nearest = { 0 };
    Vector2 normal = { 0 };
    PointNearestRectanglePoint(bounds, center, nearest, normal);

    bool inside = (center.x >= minX && center.x <= maxX && center.y >= minY && center.y <= maxY);

    if (!inside)
    {
        Vector2 diff = Vector2Subtract(center, nearest);
        float distSqr = Vector2LengthSqr(diff);

        if (distSqr >= radius * radius)
        {
            return false;
        }
    }

    intersectionPoint = nearest;
    hitNormal = normal;

    // Push the circle center outward along the contact normal so it rests flush against the box edge or corner
    center = Vector2Add(nearest, Vector2Scale(normal, radius));
    return true;
}

bool IntersectBBoxCylinder(Rectangle bounds, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal)
{
    return IntersectBBoxCircle(bounds, center, initalPosition, radius, intersectionPoint, hitNormal);
}

/// <summary>
/// Returns the point on a rectangle that is nearest to a provided point
/// </summary>
/// <param name="rect">The rectangle to test against</param>
/// <param name="point">The point you want to start from</param>
/// <param name="nearest">A reference that will be filled out with the point on the rectangle that is nearest to your passed in point</param>
/// <param name="normal">A reference that will be filled out with the normal of the edge or corner the nearest point is on</param>
void PointNearestRectanglePoint(Rectangle rect, Vector2 point, Vector2& nearest, Vector2& normal)
{
    float minX = rect.x;
    float maxX = rect.x + rect.width;
    if (minX > maxX)
    {
        float temp = minX;
        minX = maxX;
        maxX = temp;
    }

    float minY = rect.y;
    float maxY = rect.y + rect.height;
    if (minY > maxY)
    {
        float temp = minY;
        minY = maxY;
        maxY = temp;
    }

    bool outsideX = (point.x < minX) || (point.x > maxX);
    bool outsideY = (point.y < minY) || (point.y > maxY);

    if (outsideX || outsideY)
    {
        nearest.x = (point.x < minX) ? minX : ((point.x > maxX) ? maxX : point.x);
        nearest.y = (point.y < minY) ? minY : ((point.y > maxY) ? maxY : point.y);

        Vector2 diff = Vector2Subtract(point, nearest);
        float dist = Vector2Length(diff);

        if (dist > 0.00001f)
        {
            normal = Vector2Scale(diff, 1.0f / dist);
        }
        else
        {
            normal = Vector2{ 0.0f, -1.0f };
        }
    }
    else
    {
        // Point is inside the rectangle (or on the boundary)
        // Find the nearest boundary edge and push outward
        float distLeft = point.x - minX;
        float distRight = maxX - point.x;
        float distTop = point.y - minY;
        float distBottom = maxY - point.y;

        float minDist = distLeft;
        normal = Vector2{ -1.0f, 0.0f };
        nearest = Vector2{ minX, point.y };

        if (distRight < minDist)
        {
            minDist = distRight;
            normal = Vector2{ 1.0f, 0.0f };
            nearest = Vector2{ maxX, point.y };
        }

        if (distTop < minDist)
        {
            minDist = distTop;
            normal = Vector2{ 0.0f, -1.0f };
            nearest = Vector2{ point.x, minY };
        }

        if (distBottom < minDist)
        {
            minDist = distBottom;
            normal = Vector2{ 0.0f, 1.0f };
            nearest = Vector2{ point.x, maxY };
        }
    }
}

// check if a cylinder hits a circle
bool IntersectCircleCylinder(Vector2 circleCenter, float circleRadius, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal)
{
    if (!CheckCollisionCircles(center, radius, circleCenter, circleRadius))
    {
        return false;
    }

    Vector2 nearest = { 0 };
    Vector2 normal = { 0 };
    PointNearestCirclePoint(circleCenter, circleRadius, center, nearest, normal);

    intersectionPoint = nearest;
    hitNormal = normal;

    // Push the cylinder center outward along the contact normal so it rests flush against the circle
    center = Vector2Add(nearest, Vector2Scale(normal, radius));
    return true;
}

/// <summary>
/// Returns the point on a circle that is nearest to a provided point
/// </summary>
/// <param name="circleCenter">The center of the circle to test against</param>
/// <param name="circleRadius">The radius of the circle to test against</param>
/// <param name="point">The point you want to start from</param>
/// <param name="nearest">A reference that will be filled out with the point on the circle that is nearest to your passed in point</param>
/// <param name="normal">A reference that will be filled out with the normal of the circle edge the nearest point is on</param>
void PointNearestCirclePoint(Vector2 circleCenter, float circleRadius, Vector2 point, Vector2& nearest, Vector2& normal)
{
    Vector2 diff = Vector2Subtract(point, circleCenter);
    float dist = Vector2Length(diff);

    if (dist > 0.0001f)
    {
        normal = Vector2Scale(diff, 1.0f / dist);
    }
    else
    {
        normal = Vector2{ 1.0f, 0.0f };
    }

    nearest = Vector2Add(circleCenter, Vector2Scale(normal, circleRadius));
}

bool ResolveCircleCircleCollision(Vector2& posA, float radiusA, Vector2& posB, float radiusB, Vector2& hitNormal, float& penetrationDepth)
{
    Vector2 diff = Vector2Subtract(posA, posB);
    float distSqr = Vector2LengthSqr(diff);
    float minDist = radiusA + radiusB;

    if (distSqr >= minDist * minDist)
    {
        return false;
    }

    float dist = sqrtf(distSqr);
    if (dist > 0.0001f)
    {
        hitNormal = Vector2Scale(diff, 1.0f / dist);
        penetrationDepth = minDist - dist;
    }
    else
    {
        hitNormal = Vector2{ 1.0f, 0.0f };
        penetrationDepth = minDist;
    }

    Vector2 separation = Vector2Scale(hitNormal, penetrationDepth * 0.5f);
    posA = Vector2Add(posA, separation);
    posB = Vector2Subtract(posB, separation);

    return true;
}
