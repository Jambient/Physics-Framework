#pragma once
#include "Vector3.h"

struct AABB
{
	AABB() : lowerBound(Vector3::Zero), upperBound(Vector3::Zero) {};
	AABB(const Vector3& position, const Vector3& size)
	{
		Vector3 halfExtents = size * 0.5f;
		lowerBound = position - halfExtents;
		upperBound = position + halfExtents;
	};

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
		AABB enlarged;
		enlarged.lowerBound = lowerBound - margin;
		enlarged.upperBound = upperBound + margin;

		return enlarged;
	}

	static AABB Union(const AABB& a, const AABB& b)
	{
		AABB unioned;
		unioned.lowerBound = Vector3{
			a.lowerBound.x < b.lowerBound.x ? a.lowerBound.x : b.lowerBound.x,
			a.lowerBound.y < b.lowerBound.y ? a.lowerBound.y : b.lowerBound.y,
			a.lowerBound.z < b.lowerBound.z ? a.lowerBound.z : b.lowerBound.z
		};
		unioned.upperBound = Vector3{
			a.upperBound.x > b.upperBound.x ? a.upperBound.x : b.upperBound.x,
			a.upperBound.y > b.upperBound.y ? a.upperBound.y : b.upperBound.y,
			a.upperBound.z > b.upperBound.z ? a.upperBound.z : b.upperBound.z
		};

		return unioned;
	}
};

