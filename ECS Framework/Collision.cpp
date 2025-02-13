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

//----------------------------------------------------------------------
// Helper: Clip a polygon against a plane.
// The plane is defined by a point (pPlane) and a normal (nPlane)
// Returns the clipped polygon.
std::vector<Vector3> clipPolygonAgainstPlane(const std::vector<Vector3>& polygon,
    const Vector3& pPlane, const Vector3& nPlane)
{
    std::vector<Vector3> clipped;

    size_t count = polygon.size();
    for (size_t i = 0; i < count; ++i) {
        const Vector3& curr = polygon[i];
        const Vector3& prev = polygon[(i + count - 1) % count];

        float currDist = Vector3::Dot(curr - pPlane, nPlane);
        float prevDist = Vector3::Dot(prev - pPlane, nPlane);

        // If current point is inside (or on) the plane, add it.
        if (currDist >= 0)
        {
            // If previous point was outside, add the intersection first.
            if (prevDist < 0) {
                float t = prevDist / (prevDist - currDist);
                Vector3 intersect = prev + (curr - prev) * t;
                clipped.push_back(intersect);
            }
            clipped.push_back(curr);
        }
        // Else, if current is outside but previous was inside, add intersection.
        else if (prevDist >= 0) {
            float t = prevDist / (prevDist - currDist);
            Vector3 intersect = prev + (curr - prev) * t;
            clipped.push_back(intersect);
        }
    }

    return clipped;
}

void CreateCollisionManifold(const OBB& obbA, const OBB& obbB,
    const Vector3& collisionNormal, float penetration,
    CollisionManifold& manifold)
{
    // Step 1. Choose a reference and an incident box.
    // Here we choose the box whose face is most aligned with the collision normal.
    // (For simplicity, we assume collisionNormal points from A to B.)

    double maxA = 0.0;
    int refFaceA = 0;
    for (int i = 0; i < 3; ++i) {
        double dotA = std::fabs(Vector3::Dot(collisionNormal, obbA.axes[i]));
        if (dotA > maxA) {
            maxA = dotA;
            refFaceA = i;
        }
    }

    double maxB = 0.0;
    int refFaceB = 0;
    for (int i = 0; i < 3; ++i) {
        double dotB = std::fabs(Vector3::Dot(collisionNormal, obbB.axes[i]));
        if (dotB > maxB) {
            maxB = dotB;
            refFaceB = i;
        }
    }

    // We choose the reference box to be the one with the larger face alignment.
    bool useAAsReference = (maxA >= maxB);

    const OBB& refBox = useAAsReference ? obbA : obbB;
    const OBB& incBox = useAAsReference ? obbB : obbA;

    // Ensure the reference face normal points in the proper direction.
    Vector3 refNormal = refBox.axes[useAAsReference ? refFaceA : refFaceB];
    if (Vector3::Dot(refNormal, collisionNormal) < 0)
        refNormal = refNormal * -1.0;

    // Compute the reference face center.
    // (For the chosen face, move from the box center along the face normal by the corresponding half extent.)
    float refExtent = (useAAsReference ?
        (refFaceA == 0 ? refBox.halfExtents.x :
            refFaceA == 1 ? refBox.halfExtents.y : refBox.halfExtents.z) :
        (refFaceB == 0 ? refBox.halfExtents.x :
            refFaceB == 1 ? refBox.halfExtents.y : refBox.halfExtents.z));
    Vector3 refFaceCenter = refBox.center + refNormal * refExtent;

    // Step 2. Determine the side (edge) axes for the reference face.
    // These come from the two remaining axes of the reference box.
    int refAxis0 = (useAAsReference ? refFaceA : refFaceB);
    int sideAxis1 = (refAxis0 + 1) % 3;
    int sideAxis2 = (refAxis0 + 2) % 3;
    Vector3 sideNormal1 = refBox.axes[sideAxis1];
    Vector3 sideNormal2 = refBox.axes[sideAxis2];

    // Get the half extents for the reference face in these directions.
    float extent1 = (sideAxis1 == 0 ? refBox.halfExtents.x : (sideAxis1 == 1 ? refBox.halfExtents.y : refBox.halfExtents.z));
    float extent2 = (sideAxis2 == 0 ? refBox.halfExtents.x : (sideAxis2 == 1 ? refBox.halfExtents.y : refBox.halfExtents.z));

    // Define four clipping planes for the reference face.
    // Each plane is defined by a point and an outward normal (pointing inside the reference face region).
    struct Plane { Vector3 point; Vector3 normal; };
    std::vector<Plane> clipPlanes(4);

    // For each side, the plane passes through (refFaceCenter ± sideNormal * extent)
    clipPlanes[0] = { refFaceCenter + sideNormal1 * extent1,  sideNormal1 * -1.0f };
    clipPlanes[1] = { refFaceCenter - sideNormal1 * extent1,  sideNormal1 * 1.0f };
    clipPlanes[2] = { refFaceCenter + sideNormal2 * extent2,  sideNormal2 * -1.0f };
    clipPlanes[3] = { refFaceCenter - sideNormal2 * extent2,  sideNormal2 * 1.0f };

    // Also add the reference face plane itself for later depth testing:
    Plane refPlane = { refFaceCenter, refNormal };

    // Step 3. Identify the incident face from the incident box.
    // We choose the face whose normal (in world space) is most anti-parallel to the collision normal.
    double minDot = 1e9;
    int incFaceIndex = 0;
    bool isNegativeFace = false;

    for (int i = 0; i < 3; ++i) {
        Vector3 faceNormal = incBox.axes[i];

        double dotPos = Vector3::Dot(faceNormal, collisionNormal);     // Positive face normal
        double dotNeg = Vector3::Dot(faceNormal * -1.0, collisionNormal); // Negative face normal

        if (dotPos < minDot) {
            minDot = dotPos;
            incFaceIndex = i;
            isNegativeFace = false;  // Positive normal is best so far
        }
        if (dotNeg < minDot) {
            minDot = dotNeg;
            incFaceIndex = i;
            isNegativeFace = true;   // Negative normal is best so far
        }
    }

    // Set the incident face normal
    Vector3 incFaceNormal = incBox.axes[incFaceIndex];
    if (isNegativeFace) {
        incFaceNormal = incFaceNormal * -1.0;  // Flip if selecting negative face
    }

    std::vector<Vector3> incidentFace = incBox.GetFaceVertices(incFaceIndex, !isNegativeFace);

    //// Compute the incident face center.
    //double incExtent = (incFaceIndex == 0 ? incBox.halfExtents.x :
    //    incFaceIndex == 1 ? incBox.halfExtents.y : incBox.halfExtents.z);
    //Vector3 incFaceCenter = incBox.center + incFaceNormal * incExtent;

    //// Get the two remaining axes for the incident face.
    //int incAxis1 = (incFaceIndex + 1) % 3;
    //int incAxis2 = (incFaceIndex + 2) % 3;
    //Vector3 incAxisVec1 = incBox.axes[incAxis1];
    //Vector3 incAxisVec2 = incBox.axes[incAxis2];

    //double incExtent1 = (incAxis1 == 0 ? incBox.halfExtents.x :
    //    incAxis1 == 1 ? incBox.halfExtents.y : incBox.halfExtents.z);
    //double incExtent2 = (incAxis2 == 0 ? incBox.halfExtents.x :
    //    incAxis2 == 1 ? incBox.halfExtents.y : incBox.halfExtents.z);

    //// Build the incident face polygon (a rectangle with 4 vertices).
    //std::vector<Vector3> incidentFace(4);
    //incidentFace[0] = incFaceCenter + incAxisVec1 * incExtent1 + incAxisVec2 * incExtent2;
    //incidentFace[1] = incFaceCenter - incAxisVec1 * incExtent1 + incAxisVec2 * incExtent2;
    //incidentFace[2] = incFaceCenter - incAxisVec1 * incExtent1 - incAxisVec2 * incExtent2;
    //incidentFace[3] = incFaceCenter + incAxisVec1 * incExtent1 - incAxisVec2 * incExtent2;

    // Step 4. Clip the incident face polygon against the side planes of the reference face.
    std::vector<Vector3> clippedPolygon = incidentFace;
    for (const auto& plane : clipPlanes)
        clippedPolygon = clipPolygonAgainstPlane(clippedPolygon, plane.point, plane.normal);

    // Step 5. For each vertex in the clipped polygon, compute the penetration relative to the reference face plane.
    manifold.collisionNormal = collisionNormal;   // Use the collision normal provided.
    manifold.penetrationDepth = penetration;    // Use the penetration provided.
    manifold.contactPoints.clear();
    const float kEpsilon = 1e-6;
    for (const Vector3& p : clippedPolygon) {
        float separation = Vector3::Dot(p - refPlane.point, refPlane.normal);
        // Only add contact points that are within the penetration tolerance.
        if (separation <= kEpsilon) {
            // Optionally, you can project the contact point onto the reference plane.
            Vector3 contactPoint = p - refPlane.normal * separation;
            manifold.contactPoints.push_back(contactPoint);
        }
    }

    // Limit to a maximum of 4 contact points.
    if (manifold.contactPoints.size() > 4)
        manifold.contactPoints.resize(4);
}

CollisionManifold HandlePointPointCollision(const ColliderBase& a, const ColliderBase& b)
{
    CollisionManifold manifold;
    manifold.isColliding = false;
    
    return manifold;
}

CollisionManifold HandleSpherePointCollision(const ColliderBase& a, const ColliderBase& b)
{
    // cast colliders into correct type
    const Sphere& sphereA = static_cast<const Sphere&>(a);
    const Point& pointB = static_cast<const Point&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    Vector3 delta = pointB.position - sphereA.center;

    if (delta.sqrMagnitude() < sphereA.radius * sphereA.radius)
    {
        manifold.isColliding = true;
        float distance = delta.magnitude();
        manifold.penetrationDepth = sphereA.radius - distance;

        manifold.collisionNormal = delta.normalized();

        Vector3 contactPoint = sphereA.center + manifold.collisionNormal * sphereA.radius;
        manifold.contactPoints.push_back(contactPoint);
    }

    return manifold;
}

CollisionManifold HandleOBBPointCollision(const ColliderBase& a, const ColliderBase& b)
{
    // cast colliders into correct type
    const OBB& boxA = static_cast<const OBB&>(a);
    const Point& pointB = static_cast<const Point&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    // Transform the point to OBB local space
    Vector3 localPoint = boxA.GetRotationMatrix().transpose() * (pointB.position - boxA.center);

    // Check if the point is inside the OBB
    if (std::abs(localPoint.x) <= boxA.halfExtents.x &&
        std::abs(localPoint.y) <= boxA.halfExtents.y &&
        std::abs(localPoint.z) <= boxA.halfExtents.z) {

        manifold.isColliding = true;
        manifold.contactPoints.push_back(pointB.position);

        // Compute penetration depths along each axis
        Vector3 penetration(
            boxA.halfExtents.x - std::abs(localPoint.x),
            boxA.halfExtents.y - std::abs(localPoint.y),
            boxA.halfExtents.z - std::abs(localPoint.z)
        );

        // Find the minimum penetration axis (which determines the normal)
        if (penetration.x <= penetration.y && penetration.x <= penetration.z) {
            manifold.collisionNormal = boxA.axes[0] * (localPoint.x < 0 ? -1.0f : 1.0f);
            manifold.penetrationDepth = penetration.x;
        }
        else if (penetration.y <= penetration.x && penetration.y <= penetration.z) {
            manifold.collisionNormal = boxA.axes[1] * (localPoint.y < 0 ? -1.0f : 1.0f);
            manifold.penetrationDepth = penetration.y;
        }
        else {
            manifold.collisionNormal = boxA.axes[2] * (localPoint.z < 0 ? -1.0f : 1.0f);
            manifold.penetrationDepth = penetration.z;
        }
    }

    return manifold;
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
    
    //// If we reach here, there's a collision
    //manifold.isColliding = true;
    //manifold.collisionNormal = bestAxis.normalized();
    //manifold.penetrationDepth = smallestOverlap;
    //
    //// Generate contact points (simplified as the center of the overlap region)
    ////Vector3 displacement = boxB.center - boxA.center;
    ////if (Vector3::Dot(displacement, manifold.collisionNormal) < 0) {
    ////    manifold.collisionNormal = -manifold.collisionNormal; // Ensure normal points from obb1 to obb2
    ////}
    ////
    ////// TODO:THIS SHOULD NOT BE DONE IN SAT AS IT IS NOT RELIABLE OR ACCURATE AT ALL.
    ////// A CLIPPING METHOD SHOULD BE BUILT TO PROPERLY CALCULATE THE CONTACT POINT

    //Vector3 contactPoint = boxA.center + manifold.collisionNormal * (boxA.halfExtents.x / 2.0f);
    //manifold.contactPoints.push_back(contactPoint);

    manifold.isColliding = true;
    CreateCollisionManifold(boxA, boxB, bestAxis.normalized(), smallestOverlap, manifold);

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
    Vector3 localSphereCenter = boxA.GetRotationMatrix() * (sphereB.center - boxA.center);

    Vector3 boxSize = boxA.halfExtents;

    // Clamp point to OBB extents in local space
    Vector3 closestPointLocal = Vector3::Clamp(localSphereCenter, -boxSize, boxSize);

    // Back to world space
    Vector3 closestPointOnBox = boxA.GetRotationMatrix().transpose() * closestPointLocal + boxA.center;

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

    return manifold;

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
    }

    return manifold;
}

CollisionManifold HandleHSTriHSTriCollision(const ColliderBase& a, const ColliderBase& b)
{
    CollisionManifold manifold;
    manifold.isColliding = false;

    return manifold;
}

CollisionManifold HandleHSTriSphereCollision(const ColliderBase& a, const ColliderBase& b)
{
    const HalfSpaceTriangle& triangleA = static_cast<const HalfSpaceTriangle&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    float distance = Vector3::Dot(sphereB.center - triangleA.point1, triangleA.normal);

    if (distance >= 0 && distance <= sphereB.radius)
    {
        // barycentric coordinate check to confirm point is inside triangle
        Vector3 contactPoint = sphereB.center - triangleA.normal * distance;

        Vector3 AB = triangleA.point2 - triangleA.point1;
        Vector3 AC = triangleA.point3 - triangleA.point1;

        // degenerate triangle check
        float area = Vector3::Cross(AB, AC).magnitude();
        if (area < 1e-6f) { return manifold; }

        Vector3 PA = triangleA.point1 - contactPoint;
        Vector3 PB = triangleA.point2 - contactPoint;
        Vector3 PC = triangleA.point3 - contactPoint;

        float a = Vector3::Cross(PB, PC).magnitude() / area;
        float b = Vector3::Cross(PC, PA).magnitude() / area;
        float c = Vector3::Cross(PA, PB).magnitude() / area;

        if (a >= 0 && b >= 0 && c >= 0)
        {
            manifold.isColliding = true;
            manifold.collisionNormal = triangleA.normal;
            manifold.penetrationDepth = sphereB.radius - distance;
            manifold.contactPoints.push_back(contactPoint);
        }
    }

    return manifold;
}

CollisionManifold HandleHSTriPointCollision(const ColliderBase& a, const ColliderBase& b)
{
    const HalfSpaceTriangle& triangleA = static_cast<const HalfSpaceTriangle&>(a);
    const Point& pointB = static_cast<const Point&>(b);

    CollisionManifold manifold;
    manifold.isColliding = false;

    float distance = Vector3::Dot(pointB.position - triangleA.point1, triangleA.normal);

    if (distance >= 0 && distance <= 1e-6f)
    {
        // barycentric coordinate check to confirm point is inside triangle
        Vector3 contactPoint = pointB.position - triangleA.normal * distance;

        manifold.isColliding = true;
        manifold.collisionNormal = triangleA.normal;
        manifold.penetrationDepth = distance;
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
            CollisionManifold manifold = handler(b, a);
            manifold.collisionNormal = -manifold.collisionNormal;
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

    // point vs ...
    Collision::RegisterCollisionHandler(ColliderType::Point, ColliderType::Point, HandlePointPointCollision);

    // oriented box vs ...
    Collision::RegisterCollisionHandler(ColliderType::OrientedBox, ColliderType::OrientedBox, HandleObbObbCollision);
    Collision::RegisterCollisionHandler(ColliderType::OrientedBox, ColliderType::Sphere, HandleOBBSphereCollision);
    Collision::RegisterCollisionHandler(ColliderType::OrientedBox, ColliderType::Point, HandleOBBPointCollision);

    // sphere vs ...
    Collision::RegisterCollisionHandler(ColliderType::Sphere, ColliderType::Sphere, HandleSphereSphereCollision);
    Collision::RegisterCollisionHandler(ColliderType::Sphere, ColliderType::Point, HandleSpherePointCollision);

    // aligned box vs ...
    Collision::RegisterCollisionHandler(ColliderType::AlignedBox, ColliderType::AlignedBox, HandleAABBAABBCollision);
    Collision::RegisterCollisionHandler(ColliderType::AlignedBox, ColliderType::Sphere, HandleAABBSphereCollision);

    // half space triangle vs ...
    Collision::RegisterCollisionHandler(ColliderType::HalfSpaceTriangle, ColliderType::HalfSpaceTriangle, HandleHSTriHSTriCollision);
    Collision::RegisterCollisionHandler(ColliderType::HalfSpaceTriangle, ColliderType::Sphere, HandleHSTriSphereCollision);
    Collision::RegisterCollisionHandler(ColliderType::HalfSpaceTriangle, ColliderType::Point, HandleHSTriPointCollision);
}