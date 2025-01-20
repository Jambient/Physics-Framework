#pragma once
#include "Vector3.h"
#include "Quaternion.h"

enum class ColliderType
{
	Plane,
	Sphere,
	OrientedBox
};

const int ColliderTypeCount = 3;

struct ColliderBase
{
	ColliderType type;

	ColliderBase(ColliderType type) : type(type) {}
};

struct OBB : public ColliderBase
{
	Vector3 center;
	Vector3 halfExtents;
	std::array<Vector3, 3> axes;

	OBB(Vector3 position, Vector3 size, Quaternion rotation) : ColliderBase(ColliderType::OrientedBox)
	{
		Update(position, size, rotation);
	}

	inline void Update(Vector3 position, Vector3 size, Quaternion rotation)
	{
		center = position;
		halfExtents = size * 0.5f;

		axes[0] = rotation * Vector3::Right;
		axes[1] = rotation * Vector3::Up;
		axes[2] = rotation * Vector3::Forward;
	}

	void GetMinMaxVertexOnAxis(const Vector3& axis, float& minOut, float& maxOut) const
	{
		float centerProjection = Vector3::Dot(center, axis);

		float extent = 0.0f;
		for (int i = 0; i < 3; i++)
		{
			extent += fabsf(Vector3::Dot(axes[i], axis)) * halfExtents[i];
		}

		minOut = centerProjection - extent;
		maxOut = centerProjection + extent;
	}
};

struct Sphere : public ColliderBase
{
	Sphere(Vector3 center, float radius) : ColliderBase(ColliderType::Sphere), center(center), radius(radius) {}

	Vector3 center;
	float radius;
};