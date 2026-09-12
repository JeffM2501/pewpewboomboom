#include <limits>

#include "collisions.h"
#include "raylib.h"
#include "raymath.h"

// check if a cylinder hits a bounding box
bool IntersectBBoxCylinder(Rectangle bounds, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal)
{
    if (!CheckCollisionCircleRec(center, radius, bounds))
        return false;

    Vector2 newPosOrigin = { center.x, center.y };
    Vector2 hitPoint = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
    Vector2 hitNormal2d = { 0 };

    PointNearestRectanglePoint(bounds, newPosOrigin, hitPoint, hitNormal2d);

    Vector2 vectorToHit = Vector2Subtract(hitPoint, newPosOrigin);

    if (Vector2LengthSqr(vectorToHit) >= radius * radius)
        return false;

    intersectionPoint = Vector2{ hitPoint.x, hitPoint.y };
    hitNormal = Vector2{ hitNormal2d.x, hitNormal2d.y };

    // normalize the vector along the point to where we are nearest
    vectorToHit = Vector2Normalize(vectorToHit);

    // project that out to the radius to find the point that should be 'deepest' into the rectangle.
    Vector2 projectedPoint = Vector2Add(newPosOrigin, Vector2Scale(vectorToHit, radius));

    // compute the shift to take the deepest point out to the edge of our nearest hit, based on the vector direction
    Vector2 delta = { 0,0 };

    if (hitNormal.x != 0)
        delta.x = hitPoint.x - projectedPoint.x;
    else
        delta.y = hitPoint.y - projectedPoint.y;

    // shift the new point by the delta to push us outside of the rectangle
    newPosOrigin = Vector2Add(newPosOrigin, delta);

    center = Vector2{ newPosOrigin.x, newPosOrigin.y };
    return true;
}

/// <summary>
/// Returns the point on a rectangle that is nearest to a provided point
/// </summary>
/// <param name="rect">The rectangle to test against</param>
/// <param name="point">The point you want to start from</param>
/// <param name="nearest">A pointer that will be filed out with the point on the rectangle that is nearest to your passed in point</param>
/// <param name="normal">A pointer that will be filed out with the the normal of the edge the nearest point is on</param>
void PointNearestRectanglePoint(Rectangle rect, Vector2 point, Vector2& nearest, Vector2& normal)
{
    // get the closest point on the vertical sides
    float hValue = rect.x;
    float hNormal = -1;
    if (point.x > rect.x + rect.width)
    {
        hValue = rect.x + rect.width;
        hNormal = 1;
    }

    Vector2 vecToPoint = Vector2Subtract(Vector2{ hValue, rect.y }, point);
    // get the dot product between the ray and the vector to the point
    float dotForPoint = Vector2DotProduct(Vector2{ 0, -1 }, vecToPoint);
    Vector2 nearestPoint = { hValue, 0 };

    if (dotForPoint < 0)
        nearestPoint.y = rect.y;
    else if (dotForPoint >= rect.height)
        nearestPoint.y = rect.y + rect.height;
    else
        nearestPoint.y = rect.y + dotForPoint;

    // get the closest point on the horizontal sides
    float vValue = rect.y;
    float vNormal = -1;
    if (point.y > rect.y + rect.height)
    {
        vValue = rect.y + rect.height;
        vNormal = 1;
    }

    vecToPoint = Vector2Subtract(Vector2{ rect.x, vValue }, point);
    // get the dot product between the ray and the vector to the point
    dotForPoint = Vector2DotProduct(Vector2{ -1, 0 }, vecToPoint);
    nearest = Vector2{ 0,vValue };

    if (dotForPoint < 0)
        nearest.x = rect.x;
    else if (dotForPoint >= rect.width)
        nearest.x = rect.x + rect.width;
    else
        nearest.x = rect.x + dotForPoint;

    if (Vector2LengthSqr(Vector2Subtract(point, nearestPoint)) <= Vector2LengthSqr(Vector2Subtract(point, nearest)))
    {
        nearest = nearestPoint;
        normal.x = hNormal;
        normal.y = 0;
    }
    else
    {
        normal.y = vNormal;
        normal.x = 0;
    }
}

// check if a cylinder hits a circle
bool IntersectCircleCylinder(Vector2 circleCenter, float circleRadius, Vector2& center, Vector2 initalPosition, float radius, Vector2& intersectionPoint, Vector2& hitNormal)
{
    if (!CheckCollisionCircles(center, radius, circleCenter, circleRadius))
    {
        return false;
    }

    Vector2 newPosOrigin = { center.x, center.y };
    Vector2 hitPoint = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
    Vector2 hitNormal2d = { 0 };

    PointNearestCirclePoint(circleCenter, circleRadius, newPosOrigin, hitPoint, hitNormal2d);

    Vector2 vectorToHit = Vector2Subtract(hitPoint, newPosOrigin);

    if (Vector2LengthSqr(vectorToHit) >= radius * radius && Vector2DistanceSqr(newPosOrigin, circleCenter) >= circleRadius * circleRadius)
    {
        return false;
    }

    intersectionPoint = Vector2{ hitPoint.x, hitPoint.y };
    hitNormal = Vector2{ hitNormal2d.x, hitNormal2d.y };

    // normalize the vector along the point to where we are nearest
    float hitDist = Vector2Length(vectorToHit);
    if (hitDist > 0.0001f && Vector2DistanceSqr(newPosOrigin, circleCenter) >= circleRadius * circleRadius)
    {
        vectorToHit = Vector2Scale(vectorToHit, 1.0f / hitDist);
    }
    else
    {
        vectorToHit = Vector2Negate(hitNormal);
    }

    // project that out to the radius to find the point that should be 'deepest' into the circle.
    Vector2 projectedPoint = Vector2Add(newPosOrigin, Vector2Scale(vectorToHit, radius));

    // compute the shift to take the deepest point out to the edge of our nearest hit, based on the vector direction
    Vector2 delta = Vector2Subtract(hitPoint, projectedPoint);

    // shift the new point by the delta to push us outside of the circle
    newPosOrigin = Vector2Add(newPosOrigin, delta);

    center = Vector2{ newPosOrigin.x, newPosOrigin.y };
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