#include "Collision.h"

CollisionHandler Collision::dispatchTable[ColliderTypeCount][ColliderTypeCount] = {};

//CollisionInfo Collision::Intersects(const AABB& c1, const AABB& c2)
//{
//    CollisionInfo info;
//
//    if (AABB::Overlap(c1, c2))
//    {
//        info.isColliding = true;
//
//        // Calculate the overlap on each axis
//        float xOverlap = min(c1.upperBound.x, c2.upperBound.x) - max(c1.lowerBound.x, c2.lowerBound.x);
//        float yOverlap = min(c1.upperBound.y, c2.upperBound.y) - max(c1.lowerBound.y, c2.lowerBound.y);
//        float zOverlap = min(c1.upperBound.z, c2.upperBound.z) - max(c1.lowerBound.z, c2.lowerBound.z);
//
//        // Find the axis of least penetration
//        if (xOverlap < yOverlap && xOverlap < zOverlap)
//        {
//            info.normal = Vector3((c1.getPosition().x < c2.getPosition().x) ? -1.0f : 1.0f, 0.0f, 0.0f);
//            info.penetrationDepth = xOverlap;
//        }
//        else if (yOverlap < zOverlap)
//        {
//            info.normal = Vector3(0.0f, (c1.getPosition().y < c2.getPosition().y) ? -1.0f : 1.0f, 0.0f);
//            info.penetrationDepth = yOverlap;
//        }
//        else
//        {
//            info.normal = Vector3(0.0f, 0.0f, (c1.getPosition().z < c2.getPosition().z) ? -1.0f : 1.0f);
//            info.penetrationDepth = zOverlap;
//        }
//
//        // Calculate an approximate contact point (center of overlap area)
//        info.contactPoint = (Vector3(
//            max(c1.lowerBound.x, c2.lowerBound.x) + xOverlap * 0.5f,
//            max(c1.lowerBound.y, c2.lowerBound.y) + yOverlap * 0.5f,
//            max(c1.lowerBound.z, c2.lowerBound.z) + zOverlap * 0.5f
//        ));
//    }
//
//    return info;
//}
//
//bool Collision::Intersects(const SphereCollider& c1, const SphereCollider& c2)
//{
//    float combinedRadii = c1.radius + c2.radius;
//    return (c1.center - c2.center).sqrMagnitude() < combinedRadii * combinedRadii;
//}
//
//// Helper functions
//float projectOntoAxis(const Vector3& axis, const OBB& obb) {
//    return obb.halfExtents.x * fabsf(Vector3::Dot(axis, obb.axes[0])) +
//        obb.halfExtents.y * fabsf(Vector3::Dot(axis, obb.axes[1])) +
//        obb.halfExtents.z * fabsf(Vector3::Dot(axis, obb.axes[2]));
//}
//
//bool overlapOnAxis(const OBB& obb1, const OBB& obb2, const Vector3& axis, float& overlap, Vector3& collisionNormal) {
//    Vector3 normalizedAxis = axis.normalized();
//    float projection1 = projectOntoAxis(normalizedAxis, obb1);
//    float projection2 = projectOntoAxis(normalizedAxis, obb2);
//    float distance = fabsf(Vector3::Dot(obb2.center - obb1.center, normalizedAxis));
//
//    overlap = projection1 + projection2 - distance;
//    if (overlap > 0) {
//        // Ensure the collision normal points from obb1 to obb2
//        collisionNormal = (distance < 0) ? -normalizedAxis : normalizedAxis;
//        return true;
//    }
//    return false;
//}
//
//void generateContactPoints(CollisionManifold& manifold)
//{
//    if (!manifold) return;
//    if (manifold.penetrationDepth >= 0.0f) return;
//}
//
//bool checkCollisionAxis(const Vector3& axis, CollisionManifold& manifoldOut)
//{
//
//}
//
//bool satIntersection(const std::vector<Vector3>& axes, CollisionManifold& manifoldOut)
//{
//    CollisionManifold currentManifold;
//    CollisionManifold bestManifold;
//
//    bestManifold.penetrationDepth = -FLT_MAX;
//    for (const Vector3& axis : axes)
//    {
//        if (!checkCollisionAxis(axis, currentManifold))
//            return false;
//
//        if (currentManifold.penetrationDepth >= bestManifold.penetrationDepth)
//        {
//            bestManifold = currentManifold;
//        }
//    }
//
//    currentManifold = bestManifold;
//    currentManifold.isColliding = true;
//
//    return true;
//}
//
//CollisionManifold Collision::Intersects(const OBB& c1, const OBB& c2)
//{
//    CollisionManifold manifold;
//    manifold.isColliding = false;
//    manifold.penetrationDepth = FLT_MAX;
//
//    Vector3 axes[15];
//    int axisCount = 0;
//
//    // Add the face normals of both OBBs
//    for (int i = 0; i < 3; ++i) {
//        axes[axisCount++] = c1.axes[i];
//        axes[axisCount++] = c2.axes[i];
//    }
//
//    // Add the cross products of each axis pair
//    for (int i = 0; i < 3; ++i) {
//        for (int j = 0; j < 3; ++j) {
//            axes[axisCount++] = Vector3::Cross(c1.axes[i], c2.axes[j]).normalized();
//        }
//    }
//
//    Vector3 bestAxis;
//    float smallestOverlap = FLT_MAX;
//
//    // Test each axis for overlap
//    for (int i = 0; i < axisCount; ++i) {
//        if (axes[i].magnitude() < 1e-6) continue; // Skip near-zero axes
//
//        float overlap;
//        Vector3 collisionNormal;
//        if (!overlapOnAxis(c1, c2, axes[i], overlap, collisionNormal)) {
//            // Separating axis found, no collision
//            return manifold;
//        }
//
//        if (overlap < smallestOverlap) {
//            smallestOverlap = overlap;
//            bestAxis = collisionNormal;
//        }
//    }
//
//    // If we reach here, there's a collision
//    manifold.isColliding = true;
//    manifold.collisionNormal = bestAxis.normalized();
//    manifold.penetrationDepth = smallestOverlap;
//
//    // Generate contact points (simplified as the center of the overlap region)
//    Vector3 displacement = c2.center - c1.center;
//    if (Vector3::Dot(displacement, manifold.collisionNormal) < 0) {
//        manifold.collisionNormal = -manifold.collisionNormal; // Ensure normal points from obb1 to obb2
//    }
//
//    // TODO:THIS SHOULD NOT BE DONE IN SAT AS IT IS NOT RELIABLE OR ACCURATE AT ALL.
//    // A CLIPPING METHOD SHOULD BE BUILT TO PROPERLY CALCULATE THE CONTACT POINT
//    // 
//    // Generate contact points (refined)
//    Vector3 direction = manifold.collisionNormal;
//    float penetration = manifold.penetrationDepth;
//
//    // Project both centers onto the collision axis
//    float c1Projection = Vector3::Dot(c1.center, direction);
//    float c2Projection = Vector3::Dot(c2.center, direction);
//
//    // Midpoint of the overlap region
//    float midpointProjection = (c1Projection + c2Projection) * 0.5f;
//    Vector3 contactPoint = direction * midpointProjection;
//
//    // Adjust contact point by moving along the collision axis based on penetration depth
//    contactPoint = c1.center + direction * (penetration * 0.5f);
//
//    manifold.contactPoints.push_back(contactPoint);
//
//    return manifold;
//}

CollisionManifold NoOpCollision(const ColliderBase& a, const ColliderBase& b)
{
    throw std::logic_error("Collision between these shapes is not implemented");
}

float projectOntoAxis(const Vector3& axis, const OBB& obb) {
    return obb.halfExtents.x * fabsf(Vector3::Dot(axis, obb.axes[0])) +
        obb.halfExtents.y * fabsf(Vector3::Dot(axis, obb.axes[1])) +
        obb.halfExtents.z * fabsf(Vector3::Dot(axis, obb.axes[2]));
}

bool overlapOnAxis(const OBB& obb1, const OBB& obb2, const Vector3& axis, float& overlap, Vector3& collisionNormal) {
    Vector3 normalizedAxis = axis.normalized();
    float projection1 = projectOntoAxis(normalizedAxis, obb1);
    float projection2 = projectOntoAxis(normalizedAxis, obb2);
    float distance = fabsf(Vector3::Dot(obb2.center - obb1.center, normalizedAxis));

    overlap = projection1 + projection2 - distance;
    if (overlap > 0) {
        // Ensure the collision normal points from obb1 to obb2
        collisionNormal = (distance < 0) ? -normalizedAxis : normalizedAxis;
        return true;
    }
    return false;
}

CollisionManifold HandleObbObbCollision(const ColliderBase& a, const ColliderBase& b)
{
    // cast colliders into correct type
    const OBB& boxA = static_cast<const OBB&>(a);
    const OBB& boxB = static_cast<const OBB&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;
    manifold.penetrationDepth = FLT_MAX;
    
    Vector3 axes[15];
    int axisCount = 0;
    
    // Add the face normals of both OBBs
    for (int i = 0; i < 3; ++i) {
        axes[axisCount++] = boxA.axes[i];
        axes[axisCount++] = boxB.axes[i];
    }
    
    // Add the cross products of each axis pair
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            axes[axisCount++] = Vector3::Cross(boxA.axes[i], boxB.axes[j]).normalized();
        }
    }
    
    Vector3 bestAxis;
    float smallestOverlap = FLT_MAX;
    
    // Test each axis for overlap
    for (int i = 0; i < axisCount; ++i) {
        if (axes[i].magnitude() < 1e-6) continue; // Skip near-zero axes
    
        float overlap;
        Vector3 collisionNormal;
        if (!overlapOnAxis(boxA, boxB, axes[i], overlap, collisionNormal)) {
            // Separating axis found, no collision
            return manifold;
        }
    
        if (overlap < smallestOverlap) {
            smallestOverlap = overlap;
            bestAxis = collisionNormal;
        }
    }
    
    // If we reach here, there's a collision
    manifold.isColliding = true;
    manifold.collisionNormal = bestAxis.normalized();
    manifold.penetrationDepth = smallestOverlap;
    
    // Generate contact points (simplified as the center of the overlap region)
    Vector3 displacement = boxB.center - boxA.center;
    if (Vector3::Dot(displacement, manifold.collisionNormal) < 0) {
        manifold.collisionNormal = -manifold.collisionNormal; // Ensure normal points from obb1 to obb2
    }
    
    // TODO:THIS SHOULD NOT BE DONE IN SAT AS IT IS NOT RELIABLE OR ACCURATE AT ALL.
    // A CLIPPING METHOD SHOULD BE BUILT TO PROPERLY CALCULATE THE CONTACT POINT
    // 
    // Generate contact points (refined)
    //Vector3 direction = manifold.collisionNormal;
    //float penetration = manifold.penetrationDepth;
    //
    //// Project both centers onto the collision axis
    //float c1Projection = Vector3::Dot(boxA.center, direction);
    //float c2Projection = Vector3::Dot(boxB.center, direction);
    //
    //// Midpoint of the overlap region
    //float midpointProjection = (c1Projection + c2Projection) * 0.5f;
    //Vector3 contactPoint = direction * midpointProjection;
    //
    //// Adjust contact point by moving along the collision axis based on penetration depth
    //contactPoint = boxB.center + direction * (penetration * 0.5f);
    //
    //manifold.contactPoints.push_back(contactPoint);

    Vector3 contactPoint = boxA.center + manifold.collisionNormal * (boxA.halfExtents.x / 2.0f);
    manifold.contactPoints.push_back(contactPoint);
    
    return manifold;
}

CollisionManifold HandleSphereSphereCollision(const ColliderBase& a, const ColliderBase& b)
{
    // cast colliders into correct type
    const Sphere& sphereA = static_cast<const Sphere&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    float combinedRadii = sphereA.radius + sphereB.radius;
    Vector3 delta = sphereB.center - sphereA.center;

    if (delta.sqrMagnitude() >= combinedRadii * combinedRadii)
        return manifold;
    
    manifold.isColliding = true;
    float distance = delta.magnitude();
    manifold.penetrationDepth = combinedRadii - distance;

    manifold.collisionNormal = delta.normalized();

    Vector3 contactPoint = sphereA.center + manifold.collisionNormal * sphereA.radius;
    manifold.contactPoints.push_back(contactPoint);
    return manifold;
}

CollisionManifold HandleAABBSphereCollision(const ColliderBase& a, const ColliderBase& b)
{
    // cast colliders into correct type
    const AABB& boxA = static_cast<const AABB&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    Vector3 boxSize = boxA.getSize() * 0.5f;
    Vector3 delta = sphereB.center - boxA.getPosition();

    Vector3 closestPointOnBox = Vector3::Clamp(delta, -boxSize, boxSize);

    Vector3 localPoint = delta - closestPointOnBox;
    float distance = localPoint.magnitude();

    if (distance < sphereB.radius)
    {
        manifold.isColliding = true;

        manifold.collisionNormal = localPoint.normalized();
        manifold.penetrationDepth = sphereB.radius - distance;

        Vector3 contactPoint = sphereB.center + -manifold.collisionNormal * sphereB.radius;
        manifold.contactPoints.push_back(contactPoint);
    }

    return manifold;
}

CollisionManifold Collision::Collide(const ColliderBase& c1, const ColliderBase& c2)
{
    size_t c1Index = static_cast<size_t>(c1.type);
    size_t c2Index = static_cast<size_t>(c2.type);
    return dispatchTable[c1Index][c2Index](c1, c2);
}

void Collision::RegisterCollisionHandler(ColliderType typeA, ColliderType typeB, CollisionHandler handler)
{
    size_t indexA = static_cast<size_t>(typeA);
    size_t indexB = static_cast<size_t>(typeB);

    dispatchTable[indexA][indexB] = handler;
    if (indexA != indexB)
    {
        dispatchTable[indexB][indexA] = [handler](const ColliderBase& a, const ColliderBase& b)
        {
            return handler(b, a);
        };
    }
}

void Collision::Init()
{
    for (int i = 0; i < ColliderTypeCount; ++i)
    {
        for (int j = 0; j < ColliderTypeCount; ++j)
        {
            dispatchTable[i][j] = NoOpCollision;
        }
    }

    // oriented box vs ...
    Collision::RegisterCollisionHandler(ColliderType::OrientedBox, ColliderType::OrientedBox, HandleObbObbCollision);

    // sphere vs ...
    Collision::RegisterCollisionHandler(ColliderType::Sphere, ColliderType::Sphere, HandleSphereSphereCollision);

    // aligned box vs ...
    Collision::RegisterCollisionHandler(ColliderType::AlignedBox, ColliderType::Sphere, HandleAABBSphereCollision);
}