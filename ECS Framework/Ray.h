#pragma once
#include "Vector3.h"
#include "Ray.h"

class AABB;

class Ray
{
public:
	Ray(Vector3 origin, Vector3 direction);

	bool Intersect(const AABB& aabb, float& distance) const;
	Vector3 GetOrigin() const { return m_origin; }
	Vector3 GetDirection() const { return m_direction; }

private:
	Vector3 m_origin;
	Vector3 m_direction;
	Vector3 m_inverseDirection;
};

