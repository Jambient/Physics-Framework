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

	void updatePosition(const Vector3& position)
	{
		Vector3 delta = position - getPosition();
		lowerBound += delta;
		upperBound += delta;
	}

	void updateScale(const Vector3& scale)
	{
		Vector3 position = getPosition();
		lowerBound = position - scale * 0.5f;
		upperBound = position + scale * 0.5f;
	}

	static AABB FromPositionScale(const Vector3& position, const Vector3& scale)
	{
		Vector3 halfExtents = scale * 0.5f;
		return { position - halfExtents, position + halfExtents };
	}

	static AABB Union(const AABB& a, const AABB& b)
	{
		return { Vector3::Min(a.lowerBound, b.lowerBound), Vector3::Max(a.upperBound, b.upperBound) };
	}

	static bool Overlap(const AABB& a, const AABB b)
	{
		return (a.lowerBound.x <= b.upperBound.x && a.upperBound.x >= b.lowerBound.x) &&
			(a.lowerBound.y <= b.upperBound.y && a.upperBound.y >= b.lowerBound.y) &&
			(a.lowerBound.z <= b.upperBound.z && a.upperBound.z >= b.lowerBound.z);
	}
};