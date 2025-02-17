#include "Collision.h"
#include "Plane.h"
#include "Definitions.h"

CollisionHandler Collision::m_dispatchTable[ColliderTypeCount][ColliderTypeCount] = {};

bool NoOpCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    throw std::logic_error("Collision between these shapes is not implemented");
}

bool CalculateOverlapOnAxis(const OBB& obb1, const OBB& obb2, const Vector3& axis, float& overlap, Vector3& collisionNormal) 
{
    Vector3 normalizedAxis = axis.normalized();
    float projection1 = obb1.ProjectOntoAxis(normalizedAxis);
    float projection2 = obb2.ProjectOntoAxis(normalizedAxis);

    float d = Vector3::Dot(obb2.GetCenter() - obb1.GetCenter(), normalizedAxis);
    float distance = fabsf(d);

    overlap = projection1 + projection2 - distance;
    if (overlap > 0) {
        // Ensure the collision normal points from obb1 to obb2
        collisionNormal = (d < 0) ? -normalizedAxis : normalizedAxis;
        return true;
    }
    return false;
}

void CreateCollisionManifold(const OBB& obbA, const OBB& obbB, const Vector3& collisionNormal, float penetration, CollisionManifold& manifold)
{
    // choose the faces on either box that align most with the collision normal
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

    // then the reference is the face that is more aligned with the normal
    bool isBoxAReference = (maxA >= maxB);

    const OBB& refBox = isBoxAReference ? obbA : obbB;
    const OBB& incBox = isBoxAReference ? obbB : obbA;

    Vector3 refNormal = refBox.GetAxis(isBoxAReference ? refFaceA : refFaceB);
    if (Vector3::Dot(refNormal, incBox.GetCenter() - refBox.GetCenter()) < 0)
    {
        refNormal = -refNormal;
    }

    // start building up the reference face clip
    Vector3 refBoxHalfExtents = refBox.GetHalfExtents();
    float refExtent = (isBoxAReference ?
        (refFaceA == 0 ? refBoxHalfExtents.x :
            refFaceA == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z) :
        (refFaceB == 0 ? refBoxHalfExtents.x :
            refFaceB == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z));

    Vector3 refFaceCenter = refBox.GetCenter() + refNormal * refExtent;

    int refAxis0 = (isBoxAReference ? refFaceA : refFaceB);
    int sideAxis1 = (refAxis0 + 1) % 3;
    int sideAxis2 = (refAxis0 + 2) % 3;

    Vector3 sideNormal1 = refBox.GetAxis(sideAxis1);
    Vector3 sideNormal2 = refBox.GetAxis(sideAxis2);

    float extent1 = (sideAxis1 == 0 ? refBoxHalfExtents.x : (sideAxis1 == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z));
    float extent2 = (sideAxis2 == 0 ? refBoxHalfExtents.x : (sideAxis2 == 1 ? refBoxHalfExtents.y : refBoxHalfExtents.z));

    // define the four clipping planes that represent the four edges of the reference face.
    // the normals of these planes point inwards (towards the center of the reference face)
    std::vector<Plane> clipPlanes(4);

    clipPlanes[0] = Plane(refFaceCenter + sideNormal1 * extent1,  -sideNormal1);
    clipPlanes[1] = Plane(refFaceCenter - sideNormal1 * extent1,  sideNormal1);
    clipPlanes[2] = Plane(refFaceCenter + sideNormal2 * extent2,  -sideNormal2);
    clipPlanes[3] = Plane(refFaceCenter - sideNormal2 * extent2,  sideNormal2);

    // identify the incident face
    // this is the face whose normal is most opposite to the collision normal
    double minDot = 1e9;
    int incFaceIndex = 0;
    bool isNegativeFace = false;

    for (int i = 0; i < 3; ++i) {
        Vector3 faceNormal = incBox.GetAxis(i);

        // check both positive and negative face normal
        double dotPos = Vector3::Dot(faceNormal, isBoxAReference ? collisionNormal : -collisionNormal);
        double dotNeg = Vector3::Dot(-faceNormal, isBoxAReference ? collisionNormal : -collisionNormal);

        if (dotPos < minDot) {
            minDot = dotPos;
            incFaceIndex = i;
            isNegativeFace = false;
        }
        if (dotNeg < minDot) {
            minDot = dotNeg;
            incFaceIndex = i;
            isNegativeFace = true;
        }
    }

    // get the incident face normal
    Vector3 incFaceNormal = incBox.GetAxis(incFaceIndex);
    if (Vector3::Dot(incFaceNormal, refBox.GetCenter() - incBox.GetCenter()) < 0)
    {
        incFaceNormal = -incFaceNormal;
    }

    std::vector<Vector3> incidentFace = incBox.GetFaceVertices(incFaceIndex, !isNegativeFace);

    // clip the incident face against the four planes of the reference face
    std::vector<Vector3> clippedPolygon = incidentFace;
    for (const Plane& plane : clipPlanes)
        clippedPolygon = plane.ClipPolygon(clippedPolygon);

    // for any points in the clipped polygon that are left, compute their penetration into the reference plane
    manifold.normal = collisionNormal;
    manifold.penetration = penetration;
    manifold.contactPoints.clear();

    Plane refPlane = Plane(refFaceCenter, refNormal);

    for (const Vector3& p : clippedPolygon) {
        float separation = refPlane.DistanceToPoint(p);

        if (separation <= EPSILON) {
            Vector3 contactPoint = p - refPlane.GetNormal() * separation;
            manifold.contactPoints.push_back(contactPoint);
        }
    }

    if (manifold.contactPoints.size() == 0)
    {
        std::cout << "NO CONTACT POINTS" << std::endl;
    }

    // limit contact points to 4
    if (manifold.contactPoints.size() > 4)
        manifold.contactPoints.resize(4);
}

bool HandlePointPointCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{    
    return false;
}

bool HandleSpherePointCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    // cast colliders into correct type
    const Sphere& sphereA = static_cast<const Sphere&>(a);
    const Point& pointB = static_cast<const Point&>(b);

    Vector3 delta = pointB.GetPosition() - sphereA.GetCenter();

    if (delta.sqrMagnitude() < sphereA.GetRadius() * sphereA.GetRadius())
    {
        float distance = delta.magnitude();

        manifold.penetration = sphereA.GetRadius() - distance;
        manifold.normal = delta.normalized();

        Vector3 contactPoint = sphereA.GetCenter() + manifold.normal * sphereA.GetRadius();
        manifold.contactPoints.push_back(contactPoint);

        return true;
    }

    return false;
}

bool HandleOBBPointCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    // cast colliders into correct type
    const OBB& boxA = static_cast<const OBB&>(a);
    const Point& pointB = static_cast<const Point&>(b);

    // Transform the point to OBB local space
    Vector3 localPoint = boxA.GetRotationMatrix().transpose() * (pointB.GetPosition() - boxA.GetCenter());
    Vector3 boxHalfExtents = boxA.GetHalfExtents();

    // Check if the point is inside the OBB
    if (std::abs(localPoint.x) <= boxHalfExtents.x &&
        std::abs(localPoint.y) <= boxHalfExtents.y &&
        std::abs(localPoint.z) <= boxHalfExtents.z) {

        manifold.contactPoints.push_back(pointB.GetPosition());

        // Compute penetration depths along each axis
        Vector3 penetration(
            boxHalfExtents.x - std::abs(localPoint.x),
            boxHalfExtents.y - std::abs(localPoint.y),
            boxHalfExtents.z - std::abs(localPoint.z)
        );

        // Find the minimum penetration axis (which determines the normal)
        if (penetration.x <= penetration.y && penetration.x <= penetration.z) {
            manifold.normal = boxA.GetAxis(0) * (localPoint.x < 0 ? -1.0f : 1.0f);
            manifold.penetration = penetration.x;
        }
        else if (penetration.y <= penetration.x && penetration.y <= penetration.z) {
            manifold.normal = boxA.GetAxis(1) * (localPoint.y < 0 ? -1.0f : 1.0f);
            manifold.penetration = penetration.y;
        }
        else {
            manifold.normal = boxA.GetAxis(2) * (localPoint.z < 0 ? -1.0f : 1.0f);
            manifold.penetration = penetration.z;
        }

        return true;
    }

    return false;
}

bool HandleObbObbCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    // cast colliders into correct type
    const OBB& boxA = static_cast<const OBB&>(a);
    const OBB& boxB = static_cast<const OBB&>(b);

    manifold.penetration = FLT_MAX;
    
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
        if (axes[i].magnitude() < EPSILON) continue; // Skip near-zero axes
    
        float overlap;
        Vector3 collisionNormal;
        if (!CalculateOverlapOnAxis(boxA, boxB, axes[i], overlap, collisionNormal)) {
            // Separating axis found, no collision
            return false;
        }
    
        if (overlap < smallestOverlap) {
            smallestOverlap = overlap;
            bestAxis = collisionNormal;
        }
    }

    CreateCollisionManifold(boxA, boxB, bestAxis.normalized(), smallestOverlap, manifold);
    return true;
}

bool HandleSphereSphereCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    // cast colliders into correct type
    const Sphere& sphereA = static_cast<const Sphere&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

    float combinedRadii = sphereA.GetRadius() + sphereB.GetRadius();
    Vector3 delta = sphereB.GetCenter() - sphereA.GetCenter();

    if (delta.sqrMagnitude() >= combinedRadii * combinedRadii)
        return false;

    float distance = delta.magnitude();
    manifold.penetration = combinedRadii - distance;

    manifold.normal = delta.normalized();

    Vector3 contactPoint = sphereA.GetCenter() + manifold.normal * sphereA.GetRadius();
    manifold.contactPoints.push_back(contactPoint);
    return true;
}

bool HandleOBBSphereCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    // Cast colliders into correct type
    const OBB& boxA = static_cast<const OBB&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

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
        manifold.normal = displacement.normalized();
        manifold.penetration = sphereB.GetRadius() - distance;

        Vector3 contactPoint = sphereB.GetCenter() - manifold.normal * sphereB.GetRadius();
        manifold.contactPoints.push_back(contactPoint);

        return true;
    }

    return false;
}

bool HandleAABBSphereCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    // cast colliders into correct type
    const AABB& boxA = static_cast<const AABB&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

    Vector3 boxSize = boxA.GetSize() * 0.5f;
    Vector3 delta = sphereB.GetCenter() - boxA.GetPosition();

    Vector3 closestPointOnBox = Vector3::Clamp(delta, -boxSize, boxSize);

    Vector3 localPoint = delta - closestPointOnBox;
    float distance = localPoint.magnitude();

    if (distance < sphereB.GetRadius())
    {
        manifold.normal = localPoint.normalized();
        manifold.penetration = sphereB.GetRadius() - distance;

        Vector3 contactPoint = sphereB.GetCenter() + -manifold.normal * sphereB.GetRadius();
        manifold.contactPoints.push_back(contactPoint);

        return true;
    }

    return false;
}

bool HandleAABBAABBCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    const AABB& boxA = static_cast<const AABB&>(a);
    const AABB& boxB = static_cast<const AABB&>(b);

    if (AABB::Overlap(boxA, boxB))
    {
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

        manifold.normal = bestAxis;
        manifold.penetration = penetration;

        return true;
    }

    return false;
}

bool HandleHSTriHSTriCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    return false;
}

bool HandleHSTriSphereCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    const HalfSpaceTriangle& triangleA = static_cast<const HalfSpaceTriangle&>(a);
    const Sphere& sphereB = static_cast<const Sphere&>(b);

    float distance = Vector3::Dot(sphereB.GetCenter() - triangleA.GetPoint(0), triangleA.GetNormal());

    if (distance >= 0 && distance <= sphereB.GetRadius())
    {
        // barycentric coordinate check to confirm point is inside triangle
        Vector3 contactPoint = sphereB.GetCenter() - triangleA.GetNormal() * distance;

        Vector3 AB = triangleA.GetPoint(1) - triangleA.GetPoint(0);
        Vector3 AC = triangleA.GetPoint(2) - triangleA.GetPoint(0);

        // degenerate triangle check
        float area = Vector3::Cross(AB, AC).magnitude();
        if (area < EPSILON) { return false; }

        Vector3 PA = triangleA.GetPoint(0) - contactPoint;
        Vector3 PB = triangleA.GetPoint(1) - contactPoint;
        Vector3 PC = triangleA.GetPoint(2) - contactPoint;

        float a = Vector3::Cross(PB, PC).magnitude() / area;
        float b = Vector3::Cross(PC, PA).magnitude() / area;
        float c = Vector3::Cross(PA, PB).magnitude() / area;

        if (a >= 0 && b >= 0 && c >= 0)
        {
            manifold.normal = triangleA.GetNormal();
            manifold.penetration = sphereB.GetRadius() - distance;
            manifold.contactPoints.push_back(contactPoint);

            return true;
        }
    }

    return false;
}

bool HandleHSTriPointCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    const HalfSpaceTriangle& triangleA = static_cast<const HalfSpaceTriangle&>(a);
    const Point& pointB = static_cast<const Point&>(b);

    float distance = Vector3::Dot(pointB.GetPosition() - triangleA.GetPoint(0), triangleA.GetNormal());

    if (distance >= 0 && distance <= EPSILON)
    {
        Vector3 contactPoint = pointB.GetPosition() - triangleA.GetNormal() * distance;

        manifold.normal = triangleA.GetNormal();
        manifold.penetration = distance;
        manifold.contactPoints.push_back(contactPoint);

        return true;
    }

    return false;
}

bool HandleHSTriOBBCollision(const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifold)
{
    const HalfSpaceTriangle& triangleA = static_cast<const HalfSpaceTriangle&>(a);
    const OBB& boxB = static_cast<const OBB&>(b);

    for (const Vector3& point : boxB.GetVertices())
    {
        float distance = Vector3::Dot(point - triangleA.GetPoint(0), triangleA.GetNormal());
        if (distance < 0)
        {
            Vector3 contactPoint = point - triangleA.GetNormal() * distance;

            manifold.normal = triangleA.GetNormal();
            manifold.penetration = max(manifold.penetration, fabsf(distance));
            manifold.contactPoints.push_back(contactPoint);
        }
    }

    return manifold.contactPoints.size() > 0;
}

bool Collision::Collide(const ColliderBase& c1, const ColliderBase& c2, CollisionManifold& manifoldOut)
{
    size_t c1Index = static_cast<size_t>(c1.GetType());
    size_t c2Index = static_cast<size_t>(c2.GetType());
    return m_dispatchTable[c1Index][c2Index](c1, c2, manifoldOut);
}

void Collision::RegisterCollisionHandler(ColliderType typeA, ColliderType typeB, CollisionHandler handler)
{
    size_t indexA = static_cast<size_t>(typeA);
    size_t indexB = static_cast<size_t>(typeB);

    m_dispatchTable[indexA][indexB] = handler;
    if (indexA != indexB)
    {
        m_dispatchTable[indexB][indexA] = [handler](const ColliderBase& a, const ColliderBase& b, CollisionManifold& manifoldOut)
        {
            bool result = handler(b, a, manifoldOut);
            manifoldOut.normal = -manifoldOut.normal;
            return result;
        };
    }
}

void Collision::Init()
{
    for (int i = 0; i < ColliderTypeCount; ++i)
    {
        for (int j = 0; j < ColliderTypeCount; ++j)
        {
            m_dispatchTable[i][j] = NoOpCollision;
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
    Collision::RegisterCollisionHandler(ColliderType::HALF_SPACE_TRIANGLE, ColliderType::ORIENTED_BOX, HandleHSTriOBBCollision);
}