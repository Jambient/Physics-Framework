#pragma once
#include "Components.h"
#include "Structures.h"
#include "AABB.h"

struct CollisionInfo
{
	bool isColliding = false;
	Vector3 normal;
	float penetrationDepth = 0.0f;
	Vector3 contactPoint;

	// this makes sure this struct can be used in conditionals
	explicit operator bool() const
	{
		return isColliding;
	}
};

class Collision
{
public:
	static CollisionInfo Intersects(const AABB& c1, const AABB& c2);

	static bool Intersects(const SphereCollider& c1, const SphereCollider& c2);

    static bool Intersects(const OBB& c1, const OBB& c2);
};
