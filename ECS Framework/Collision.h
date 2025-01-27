#pragma once
#include "Structures.h"
#include "Colliders.h"
#include <functional>
#include "Plane.h"

// temporary structs
struct ContactPoint
{
	Vector3 localA;
	Vector3 localB;
	Vector3 normal;
	float penetration;
};

struct CollisionInfo
{
	Entity entityA;
	Entity entityB;

	ContactPoint point;
};

// full manifold derived from collision data
struct CollisionManifold
{
	bool isColliding;
	Vector3 collisionNormal;
	float penetrationDepth;
	std::vector<Vector3> contactPoints;

	// this makes sure this struct can be used in conditionals
	explicit operator bool() const
	{
		return isColliding;
	}
};

using CollisionHandler = std::function<CollisionManifold(const ColliderBase&, const ColliderBase&)>;

class Collision
{
public:
	static CollisionManifold Collide(const ColliderBase& c1, const ColliderBase& c2);

	static void RegisterCollisionHandler(ColliderType typeA, ColliderType typeB, CollisionHandler handler);

	static void Init();

private:
	static CollisionHandler dispatchTable[ColliderTypeCount][ColliderTypeCount];
};
