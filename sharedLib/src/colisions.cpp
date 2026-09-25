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

bool IntersectRayCircle(Vector2 rayOrigin, Vector2 rayDir, Vector2 circleCenter, float circleRadius, float& outDist, Vector2& outHitPoint)
{
	Vector2 v = Vector2Subtract(circleCenter, rayOrigin);
	float tProj = Vector2DotProduct(v, rayDir);
	float vLenSq = Vector2LengthSqr(v);
	float rSq = circleRadius * circleRadius;

	if (vLenSq <= rSq)
	{
		outDist = 0.0f;
		outHitPoint = rayOrigin;
		return true;
	}

	if (tProj < 0.0f)
	{
		return false;
	}

	float dPerpSq = vLenSq - (tProj * tProj);
	if (dPerpSq > rSq)
	{
		return false;
	}

	float tHalf = sqrtf(rSq - dPerpSq);
	outDist = tProj - tHalf;
	outHitPoint = Vector2Add(rayOrigin, Vector2Scale(rayDir, outDist));
	return true;
}

bool IntersectRayOBB(Vector2 rayOrigin, Vector2 rayDir, Vector2 boxCenter, Vector2 boxHalfSize, float rotationDeg, float& outDist, Vector2& outHitPoint)
{
	Vector2 relOrigin = Vector2Subtract(rayOrigin, boxCenter);
	Vector2 localOrigin = Vector2Rotate(relOrigin, -rotationDeg * DEG2RAD);
	Vector2 localDir = Vector2Rotate(rayDir, -rotationDeg * DEG2RAD);

	float tMin = -1e30f;
	float tMax = 1e30f;

	if (fabsf(localDir.x) < 1e-6f)
	{
		if (fabsf(localOrigin.x) > boxHalfSize.x)
		{
			return false;
		}
	}
	else
	{
		float invD = 1.0f / localDir.x;
		float t1 = (-boxHalfSize.x - localOrigin.x) * invD;
		float t2 = (boxHalfSize.x - localOrigin.x) * invD;
		if (t1 > t2)
		{
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		if (t1 > tMin)
		{
			tMin = t1;
		}
		if (t2 < tMax)
		{
			tMax = t2;
		}
		if (tMin > tMax)
		{
			return false;
		}
	}

	if (fabsf(localDir.y) < 1e-6f)
	{
		if (fabsf(localOrigin.y) > boxHalfSize.y)
		{
			return false;
		}
	}
	else
	{
		float invD = 1.0f / localDir.y;
		float t1 = (-boxHalfSize.y - localOrigin.y) * invD;
		float t2 = (boxHalfSize.y - localOrigin.y) * invD;
		if (t1 > t2)
		{
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		if (t1 > tMin)
		{
			tMin = t1;
		}
		if (t2 < tMax)
		{
			tMax = t2;
		}
		if (tMin > tMax)
		{
			return false;
		}
	}

	if (tMax < 0.0f)
	{
		return false;
	}

	outDist = (tMin >= 0.0f) ? tMin : 0.0f;
	Vector2 localHit = Vector2Add(localOrigin, Vector2Scale(localDir, outDist));
	outHitPoint = Vector2Add(boxCenter, Vector2Rotate(localHit, rotationDeg * DEG2RAD));
	return true;
}

bool IntersectRayAABB(Vector2 rayOrigin, Vector2 rayDir, Vector2 boxMin, Vector2 boxMax, float& outDist, Vector2& outHitPoint)
{
	float tMin = -1e30f;
	float tMax = 1e30f;

	if (fabsf(rayDir.x) < 1e-6f)
	{
		if (rayOrigin.x < boxMin.x || rayOrigin.x > boxMax.x)
		{
			return false;
		}
	}
	else
	{
		float invD = 1.0f / rayDir.x;
		float t1 = (boxMin.x - rayOrigin.x) * invD;
		float t2 = (boxMax.x - rayOrigin.x) * invD;
		if (t1 > t2)
		{
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		if (t1 > tMin)
		{
			tMin = t1;
		}
		if (t2 < tMax)
		{
			tMax = t2;
		}
		if (tMin > tMax)
		{
			return false;
		}
	}

	if (fabsf(rayDir.y) < 1e-6f)
	{
		if (rayOrigin.y < boxMin.y || rayOrigin.y > boxMax.y)
		{
			return false;
		}
	}
	else
	{
		float invD = 1.0f / rayDir.y;
		float t1 = (boxMin.y - rayOrigin.y) * invD;
		float t2 = (boxMax.y - rayOrigin.y) * invD;
		if (t1 > t2)
		{
			float tmp = t1;
			t1 = t2;
			t2 = tmp;
		}
		if (t1 > tMin)
		{
			tMin = t1;
		}
		if (t2 < tMax)
		{
			tMax = t2;
		}
		if (tMin > tMax)
		{
			return false;
		}
	}

	if (tMax < 0.0f)
	{
		return false;
	}

	outDist = (tMin >= 0.0f) ? tMin : 0.0f;
	outHitPoint = Vector2Add(rayOrigin, Vector2Scale(rayDir, outDist));
	return true;
}

bool IntersectRayBoxExit(Vector2 rayOrigin, Vector2 rayDir, Vector2 boxMin, Vector2 boxMax, float& outDist, Vector2& outHitPoint)
{
	float tMax = 1e30f;
	if (fabsf(rayDir.x) > 1e-6f)
	{
		float invD = 1.0f / rayDir.x;
		float t1 = (boxMin.x - rayOrigin.x) * invD;
		float t2 = (boxMax.x - rayOrigin.x) * invD;
		float tFar = fmaxf(t1, t2);
		if (tFar < tMax)
		{
			tMax = tFar;
		}
	}
	if (fabsf(rayDir.y) > 1e-6f)
	{
		float invD = 1.0f / rayDir.y;
		float t1 = (boxMin.y - rayOrigin.y) * invD;
		float t2 = (boxMax.y - rayOrigin.y) * invD;
		float tFar = fmaxf(t1, t2);
		if (tFar < tMax)
		{
			tMax = tFar;
		}
	}
	if (tMax > 0.0f && tMax < 1e29f)
	{
		outDist = tMax;
		outHitPoint = Vector2Add(rayOrigin, Vector2Scale(rayDir, outDist));
		return true;
	}
	return false;
}
