#include "Collision.h"

CollisionInfo Collision::Intersects(const AABB& c1, const AABB& c2)
{
    CollisionInfo info;

    if (AABB::Overlap(c1, c2))
    {
        info.isColliding = true;

        // Calculate the overlap on each axis
        float xOverlap = min(c1.upperBound.x, c2.upperBound.x) - max(c1.lowerBound.x, c2.lowerBound.x);
        float yOverlap = min(c1.upperBound.y, c2.upperBound.y) - max(c1.lowerBound.y, c2.lowerBound.y);
        float zOverlap = min(c1.upperBound.z, c2.upperBound.z) - max(c1.lowerBound.z, c2.lowerBound.z);

        // Find the axis of least penetration
        if (xOverlap < yOverlap && xOverlap < zOverlap)
        {
            info.normal = Vector3((c1.getPosition().x < c2.getPosition().x) ? -1.0f : 1.0f, 0.0f, 0.0f);
            info.penetrationDepth = xOverlap;
        }
        else if (yOverlap < zOverlap)
        {
            info.normal = Vector3(0.0f, (c1.getPosition().y < c2.getPosition().y) ? -1.0f : 1.0f, 0.0f);
            info.penetrationDepth = yOverlap;
        }
        else
        {
            info.normal = Vector3(0.0f, 0.0f, (c1.getPosition().z < c2.getPosition().z) ? -1.0f : 1.0f);
            info.penetrationDepth = zOverlap;
        }

        // Calculate an approximate contact point (center of overlap area)
        info.contactPoint = (Vector3(
            max(c1.lowerBound.x, c2.lowerBound.x) + xOverlap * 0.5f,
            max(c1.lowerBound.y, c2.lowerBound.y) + yOverlap * 0.5f,
            max(c1.lowerBound.z, c2.lowerBound.z) + zOverlap * 0.5f
        ));
    }

    return info;
}

bool Collision::Intersects(const SphereCollider& c1, const SphereCollider& c2)
{
    float combinedRadii = c1.radius + c2.radius;
    return (c1.center - c2.center).sqrMagnitude() < combinedRadii * combinedRadii;
}

bool Collision::Intersects(const OBB& c1, const OBB& c2)
{
    if (OBBSeparated(c1, c2, c1.right))
        return false;
    if (OBBSeparated(c1, c2, c1.up))
        return false;
    if (OBBSeparated(c1, c2, c1.forward))
        return false;

    if (OBBSeparated(c1, c2, c2.right))
        return false;
    if (OBBSeparated(c1, c2, c2.up))
        return false;
    if (OBBSeparated(c1, c2, c2.forward))
        return false;

    if (OBBSeparated(c1, c2, Vector3::Cross(c1.right, c2.right)))
        return false;
    if (OBBSeparated(c1, c2, Vector3::Cross(c1.right, c2.up)))
        return false;
    if (OBBSeparated(c1, c2, Vector3::Cross(c1.right, c2.forward)))
        return false;

    if (OBBSeparated(c1, c2, Vector3::Cross(c1.up, c2.right)))
        return false;
    if (OBBSeparated(c1, c2, Vector3::Cross(c1.up, c2.up)))
        return false;
    if (OBBSeparated(c1, c2, Vector3::Cross(c1.up, c2.forward)))
        return false;

    if (OBBSeparated(c1, c2, Vector3::Cross(c1.forward, c2.right)))
        return false;
    if (OBBSeparated(c1, c2, Vector3::Cross(c1.forward, c2.up)))
        return false;
    if (OBBSeparated(c1, c2, Vector3::Cross(c1.forward, c2.forward)))
        return false;

    return true;
}

bool OBBSeparated(const OBB& a, const OBB& b, Vector3 axis)
{
    // Handles the cross product = {0,0,0} case
    if (axis == Vector3::Zero)
        return false;

    float aMin = FLT_MAX;
    float aMax = FLT_MIN;
    float bMin = FLT_MAX;
    float bMax = FLT_MIN;

    // Define two intervals, a and b. Calculate their min and max values
    for (int i = 0; i < 8; i++)
    {
        float aDist = Vector3::Dot(a.vertices[i], axis);
        aMin = aDist < aMin ? aDist : aMin;
        aMax = aDist > aMax ? aDist : aMax;
        float bDist = Vector3::Dot(a.vertices[i], axis);
        bMin = bDist < bMin ? bDist : bMin;
        bMax = bDist > bMax ? bDist : bMax;
    }

    // One-dimensional intersection test between a and b
    float longSpan = max(aMax, bMax) - min(aMin, bMin);
    float sumSpan = aMax - aMin + bMax - bMin;
    return longSpan >= sumSpan; // > to treat touching as intersection
}
