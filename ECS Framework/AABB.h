#pragma once
#define NOMINMAX
#include "Vector3.h"

struct AABB
{
	AABB() : lowerBound(Vector3::Zero), upperBound(Vector3::Zero) {};
	AABB(const Vector3& lowerBound, const Vector3& upperBound)
		: lowerBound(lowerBound), upperBound(upperBound) {};

	Vector3 lowerBound;
	Vector3 upperBound;

	Vector3 getPosition() const
	{
		return (lowerBound + upperBound) * 0.5f;
	}

	Vector3 getSize() const
	{
		return upperBound - lowerBound;
	}

	float area() const
	{
		Vector3 d = upperBound - lowerBound;
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	AABB enlarged(float factor) const
	{
		Vector3 margin = (upperBound - lowerBound) * factor;
		return { lowerBound - margin, upperBound + margin };
	}

	static AABB FromPositionScale(const Vector3& position, const Vector3& size)
	{
		Vector3 halfExtents = size * 0.5f;
		return { position - halfExtents, position + halfExtents };
	}

	static AABB Union(const AABB& a, const AABB& b)
	{
		return { Vector3::Min(a.lowerBound, b.lowerBound), Vector3::Max(a.upperBound, b.upperBound) };
	}
};