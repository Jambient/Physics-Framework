#pragma once
#include "Structures.h"
#include "Colliders.h"
#include <functional>
#include "Plane.h"

struct CollisionManifold
{
	Vector3 normal;
	float penetration;
	std::vector<Vector3> contactPoints;
};


struct CollisionInfo
{
	Entity entityA;
	Entity entityB;
	CollisionManifold manifold;
};

using CollisionHandler = std::function<bool(const ColliderBase&, const ColliderBase&, CollisionManifold&)>;

class Collision
{
public:
	static bool Collide(const ColliderBase& c1, const ColliderBase& c2, CollisionManifold& manifoldOut);

	static void Init();

private:
	static void RegisterCollisionHandler(ColliderType typeA, ColliderType typeB, CollisionHandler handler);

	static CollisionHandler m_dispatchTable[ColliderTypeCount][ColliderTypeCount];
};
