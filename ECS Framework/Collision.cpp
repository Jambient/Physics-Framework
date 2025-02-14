#include "Collision.h"

CollisionHandler Collision::dispatchTable[ColliderTypeCount][ColliderTypeCount] = {};

CollisionManifold NoOpCollision(const ColliderBase& a, const ColliderBase& b)
{
    throw std::logic_error("Collision between these shapes is not implemented");
}

bool overlapOnAxis(const OBB& obb1, const OBB& obb2, const Vector3& axis, float& overlap, Vector3& collisionNormal) 
{
    Vector3 normalizedAxis = axis.normalized();
    float projection1 = obb1.ProjectOntoAxis(normalizedAxis);
    float projection2 = obb2.ProjectOntoAxis(normalizedAxis);
    float distance = fabsf(Vector3::Dot(obb2.GetCenter() - obb1.GetCenter(), normalizedAxis));

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
        double dotA = std::fabs(Vector3::Dot(collisionNormal, obbA.GetAxis(i)));
        if (dotA > maxA) {
            maxA = dotA;
            refFaceA = i;
        }
    }

    double maxB = 0.0;
    int refFaceB = 0;
    for (int i = 0; i < 3; ++i) {
        double dotB = std::fabs(Vector3::Dot(collisionNormal, obbB.GetAxis(i)));
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
    Vector3 refNormal = refBox.GetAxis(useAAsReference ? refFaceA : refFaceB);
    if (Vector3::Dot(refNormal, collisionNormal) < 0)
        refNormal = refNormal * -1.0;

    // Compute the reference face center.
    // (For the chosen face, move from the box center along the face normal by the corresponding half extent.)
    Vector3 refBoxHalfExtents = refBox.GetHalfExtents();
    float refExtent = (useAAsReference ?
        (refFaceA == 0 ? refBoxHalfExtents.x :
            refFaceA == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z) :
        (refFaceB == 0 ? refBoxHalfExtents.x :
            refFaceB == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z));
    Vector3 refFaceCenter = refBox.GetCenter() + refNormal * refExtent;

    // Step 2. Determine the side (edge) axes for the reference face.
    // These come from the two remaining axes of the reference box.
    int refAxis0 = (useAAsReference ? refFaceA : refFaceB);
    int sideAxis1 = (refAxis0 + 1) % 3;
    int sideAxis2 = (refAxis0 + 2) % 3;
    Vector3 sideNormal1 = refBox.GetAxis(sideAxis1);
    Vector3 sideNormal2 = refBox.GetAxis(sideAxis2);

    // Get the half extents for the reference face in these directions.
    float extent1 = (sideAxis1 == 0 ? refBoxHalfExtents.x : (sideAxis1 == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z));
    float extent2 = (sideAxis2 == 0 ? refBoxHalfExtents.x : (sideAxis2 == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z));

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
        Vector3 faceNormal = incBox.GetAxis(i);

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
    Vector3 incFaceNormal = incBox.GetAxis(incFaceIndex);
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

    Vector3 delta = pointB.GetPosition() - sphereA.GetCenter();

    if (delta.sqrMagnitude() < sphereA.GetRadius() * sphereA.GetRadius())
    {
        manifold.isColliding = true;
        float distance = delta.magnitude();
        manifold.penetrationDepth = sphereA.GetRadius() - distance;

        manifold.collisionNormal = delta.normalized();

        Vector3 contactPoint = sphereA.GetCenter() + manifold.collisionNormal * sphereA.GetRadius();
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
    Vector3 localPoint = boxA.GetRotationMatrix().transpose() * (pointB.GetPosition() - boxA.GetCenter());
    Vector3 boxHalfExtents = boxA.GetHalfExtents();

    // Check if the point is inside the OBB
    if (std::abs(localPoint.x) <= boxHalfExtents.x &&
        std::abs(localPoint.y) <= boxHalfExtents.y &&
        std::abs(localPoint.z) <= boxHalfExtents.z) {

        manifold.isColliding = true;
        manifold.contactPoints.push_back(pointB.GetPosition());

        // Compute penetration depths along each axis
        Vector3 penetration(
            boxHalfExtents.x - std::abs(localPoint.x),
            boxHalfExtents.y - std::abs(localPoint.y),
            boxHalfExtents.z - std::abs(localPoint.z)
        );

        // Find the minimum penetration axis (which determines the normal)
        if (penetration.x <= penetration.y && penetration.x <= penetration.z) {
            manifold.collisionNormal = boxA.GetAxis(0) * (localPoint.x < 0 ? -1.0f : 1.0f);
            manifold.penetrationDepth = penetration.x;
        }
        else if (penetration.y <= penetration.x && penetration.y <= penetration.z) {
            manifold.collisionNormal = boxA.GetAxis(1) * (localPoint.y < 0 ? -1.0f : 1.0f);
            manifold.penetrationDepth = penetration.y;
        }
        else {
            manifold.collisionNormal = boxA.GetAxis(2) * (localPoint.z < 0 ? -1.0f : 1.0f);
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
        axes[axisCount++] = boxA.GetAxis(i);
        axes[axisCount++] = boxB.GetAxis(i);
    }
    
    // Add the cross products of each axis pair
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            axes[axisCount++] = Vector3::Cross(boxA.GetAxis(i), boxB.GetAxis(j)).normalized();
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

    float combinedRadii = sphereA.GetRadius() + sphereB.GetRadius();
    Vector3 delta = sphereB.GetCenter() - sphereA.GetCenter();

    if (delta.sqrMagnitude() >= combinedRadii * combinedRadii)
        return manifold;
    
    manifold.isColliding = true;
    float distance = delta.magnitude();
    manifold.penetrationDepth = combinedRadii - distance;

    manifold.collisionNormal = delta.normalized();

    Vector3 contactPoint = sphereA.GetCenter() + manifold.collisionNormal * sphereA.GetRadius();
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
    Vector3 localSphereCenter = boxA.GetRotationMatrix() * (sphereB.GetCenter() - boxA.GetCenter());

    Vector3 boxHalfExtents = boxA.GetHalfExtents();

    // Clamp point to OBB extents in local space
    Vector3 closestPointLocal = Vector3::Clamp(localSphereCenter, -boxHalfExtents, boxHalfExtents);

    // Back to world space
    Vector3 closestPointOnBox = boxA.GetRotationMatrix().transpose() * closestPointLocal + boxA.GetCenter();

    // Compute local displacement
    Vector3 displacement = sphereB.GetCenter() - closestPointOnBox;
    float distance = displacement.magnitude();

    if (distance < sphereB.GetRadius())
    {
        manifold.isColliding = true;

        manifold.collisionNormal = displacement.normalized();
        manifold.penetrationDepth = sphereB.GetRadius() - distance;

        Vector3 contactPoint = sphereB.GetCenter() - manifold.collisionNormal * sphereB.GetRadius();
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

    Vector3 boxSize = boxA.GetSize() * 0.5f;
    Vector3 delta = sphereB.GetCenter() - boxA.GetPosition();

    Vector3 closestPointOnBox = Vector3::Clamp(delta, -boxSize, boxSize);

    Vector3 localPoint = delta - closestPointOnBox;
    float distance = localPoint.magnitude();

    if (distance < sphereB.GetRadius())
    {
        manifold.isColliding = true;

        manifold.collisionNormal = localPoint.normalized();
        manifold.penetrationDepth = sphereB.GetRadius() - distance;

        Vector3 contactPoint = sphereB.GetCenter() + -manifold.collisionNormal * sphereB.GetRadius();
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

        Vector3 lowerBoundA = boxA.GetLowerBound();
        Vector3 upperBoundA = boxA.GetUpperBound();
        Vector3 lowerBoundB = boxB.GetLowerBound();
        Vector3 upperBoundB = boxB.GetUpperBound();
        float distances[6] =
        {
            upperBoundB.x - lowerBoundA.x,
            upperBoundA.x - lowerBoundB.x,
            upperBoundB.y - lowerBoundA.y,
            upperBoundA.y - lowerBoundB.y,
            upperBoundB.z - lowerBoundA.z,
            upperBoundA.z - lowerBoundB.z,
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

    float distance = Vector3::Dot(sphereB.GetCenter() - triangleA.GetPoint(0), triangleA.GetNormal());

    if (distance >= 0 && distance <= sphereB.GetRadius())
    {
        // barycentric coordinate check to confirm point is inside triangle
        Vector3 contactPoint = sphereB.GetCenter() - triangleA.GetNormal() * distance;

        Vector3 AB = triangleA.GetPoint(1) - triangleA.GetPoint(0);
        Vector3 AC = triangleA.GetPoint(2) - triangleA.GetPoint(0);

        // degenerate triangle check
        float area = Vector3::Cross(AB, AC).magnitude();
        if (area < 1e-6f) { return manifold; }

        Vector3 PA = triangleA.GetPoint(0) - contactPoint;
        Vector3 PB = triangleA.GetPoint(1) - contactPoint;
        Vector3 PC = triangleA.GetPoint(2) - contactPoint;

        float a = Vector3::Cross(PB, PC).magnitude() / area;
        float b = Vector3::Cross(PC, PA).magnitude() / area;
        float c = Vector3::Cross(PA, PB).magnitude() / area;

        if (a >= 0 && b >= 0 && c >= 0)
        {
            manifold.isColliding = true;
            manifold.collisionNormal = triangleA.GetNormal();
            manifold.penetrationDepth = sphereB.GetRadius() - distance;
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

    float distance = Vector3::Dot(pointB.GetPosition() - triangleA.GetPoint(0), triangleA.GetNormal());

    if (distance >= 0 && distance <= 1e-6f)
    {
        // barycentric coordinate check to confirm point is inside triangle
        Vector3 contactPoint = pointB.GetPosition() - triangleA.GetNormal() * distance;

        manifold.isColliding = true;
        manifold.collisionNormal = triangleA.GetNormal();
        manifold.penetrationDepth = distance;
        manifold.contactPoints.push_back(contactPoint);
    }

    return manifold;
}

CollisionManifold Collision::Collide(const ColliderBase& c1, const ColliderBase& c2)
{
    size_t c1Index = static_cast<size_t>(c1.GetType());
    size_t c2Index = static_cast<size_t>(c2.GetType());
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
    Collision::RegisterCollisionHandler(ColliderType::POINT, ColliderType::POINT, HandlePointPointCollision);

    // oriented box vs ...
    Collision::RegisterCollisionHandler(ColliderType::ORIENTED_BOX, ColliderType::ORIENTED_BOX, HandleObbObbCollision);
    Collision::RegisterCollisionHandler(ColliderType::ORIENTED_BOX, ColliderType::SPHERE, HandleOBBSphereCollision);
    Collision::RegisterCollisionHandler(ColliderType::ORIENTED_BOX, ColliderType::POINT, HandleOBBPointCollision);

    // sphere vs ...
    Collision::RegisterCollisionHandler(ColliderType::SPHERE, ColliderType::SPHERE, HandleSphereSphereCollision);
    Collision::RegisterCollisionHandler(ColliderType::SPHERE, ColliderType::POINT, HandleSpherePointCollision);

    // aligned box vs ...
    Collision::RegisterCollisionHandler(ColliderType::ALIGNED_BOX, ColliderType::ALIGNED_BOX, HandleAABBAABBCollision);
    Collision::RegisterCollisionHandler(ColliderType::ALIGNED_BOX, ColliderType::SPHERE, HandleAABBSphereCollision);

    // half space triangle vs ...
    Collision::RegisterCollisionHandler(ColliderType::HALF_SPACE_TRIANGLE, ColliderType::HALF_SPACE_TRIANGLE, HandleHSTriHSTriCollision);
    Collision::RegisterCollisionHandler(ColliderType::HALF_SPACE_TRIANGLE, ColliderType::SPHERE, HandleHSTriSphereCollision);
    Collision::RegisterCollisionHandler(ColliderType::HALF_SPACE_TRIANGLE, ColliderType::POINT, HandleHSTriPointCollision);
}