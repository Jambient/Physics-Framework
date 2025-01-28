#include "Collision.h"

CollisionHandler Collision::dispatchTable[ColliderTypeCount][ColliderTypeCount] = {};

CollisionManifold NoOpCollision(const ColliderBase& a, const ColliderBase& b)
{
    throw std::logic_error("Collision between these shapes is not implemented");
}

float projectOntoAxis(const Vector3& axis, const OBB& obb) 
{
    return obb.halfExtents.x * fabsf(Vector3::Dot(axis, obb.axes[0])) +
        obb.halfExtents.y * fabsf(Vector3::Dot(axis, obb.axes[1])) +
        obb.halfExtents.z * fabsf(Vector3::Dot(axis, obb.axes[2]));
}

bool overlapOnAxis(const OBB& obb1, const OBB& obb2, const Vector3& axis, float& overlap, Vector3& collisionNormal) 
{
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
    //Vector3 displacement = boxB.center - boxA.center;
    //if (Vector3::Dot(displacement, manifold.collisionNormal) < 0) {
    //    manifold.collisionNormal = -manifold.collisionNormal; // Ensure normal points from obb1 to obb2
    //}
    //
    //// TODO:THIS SHOULD NOT BE DONE IN SAT AS IT IS NOT RELIABLE OR ACCURATE AT ALL.
    //// A CLIPPING METHOD SHOULD BE BUILT TO PROPERLY CALCULATE THE CONTACT POINT

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

CollisionManifold HandleOBBSphereCollision(const ColliderBase& a, const ColliderBase& b)
{
    // Cast colliders into correct type
    const OBB& boxA = static_cast<const OBB&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    // Transform sphere center to OBB local space
    Vector3 localSphereCenter = boxA.GetRotationMatrix().transpose() * (sphereB.center - boxA.center);

    Vector3 boxSize = boxA.halfExtents;

    // Clamp point to OBB extents in local space
    Vector3 closestPointLocal = Vector3::Clamp(localSphereCenter, -boxSize, boxSize);

    // Back to world space
    Vector3 closestPointOnBox = boxA.GetRotationMatrix() * closestPointLocal + boxA.center;

    // Compute local displacement
    Vector3 displacement = sphereB.center - closestPointOnBox;
    float distance = displacement.magnitude();

    if (distance < sphereB.radius)
    {
        manifold.isColliding = true;

        manifold.collisionNormal = displacement.normalized();
        manifold.penetrationDepth = sphereB.radius - distance;

        Vector3 contactPoint = sphereB.center - manifold.collisionNormal * sphereB.radius;
        manifold.contactPoints.push_back(contactPoint);
    }

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

CollisionManifold HandleAABBAABBCollision(const ColliderBase& a, const ColliderBase& b)
{
    const AABB& boxA = static_cast<const AABB&>(a);
    const AABB& boxB = static_cast<const AABB&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    if (AABB::Overlap(boxA, boxB))
    {
        manifold.isColliding = true;

        static const Vector3 faces[6] =
        {
            Vector3::Left, Vector3::Right,
            Vector3::Down, Vector3::Up,
            Vector3::Back, Vector3::Forward
        };

        float distances[6] =
        {
            boxB.upperBound.x - boxA.lowerBound.x,
            boxA.upperBound.x - boxB.lowerBound.x,
            boxB.upperBound.y - boxA.lowerBound.y,
            boxA.upperBound.y - boxB.lowerBound.y,
            boxB.upperBound.z - boxA.lowerBound.z,
            boxA.upperBound.z - boxB.lowerBound.z,
        };
        float penetration = FLT_MAX;
        Vector3 bestAxis;

        for (int i = 0; i < 6; i++)
        {
            if (distances[i] < penetration)
            {
                penetration = distances[i];
                bestAxis = faces[i];
            }
        }

        manifold.collisionNormal = bestAxis;
        manifold.penetrationDepth = penetration;
        //manifold.contactPoints.push_back()
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
            CollisionManifold manifold = handler(b, a);
            //manifold.collisionNormal = -manifold.collisionNormal;
            return manifold;
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
    Collision::RegisterCollisionHandler(ColliderType::OrientedBox, ColliderType::Sphere, HandleOBBSphereCollision);

    // sphere vs ...
    Collision::RegisterCollisionHandler(ColliderType::Sphere, ColliderType::Sphere, HandleSphereSphereCollision);

    // aligned box vs ...
    Collision::RegisterCollisionHandler(ColliderType::AlignedBox, ColliderType::Sphere, HandleAABBSphereCollision);
}