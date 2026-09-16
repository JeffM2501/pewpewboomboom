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
