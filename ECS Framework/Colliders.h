#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "Plane.h"
#include <vector>

enum class ColliderType
{
	Point,
	Plane,
	HalfSpaceTriangle,
	Sphere,
	AlignedBox,
	OrientedBox
};

const int ColliderTypeCount = 6;

struct ColliderBase
{
	ColliderType type;

	ColliderBase(ColliderType type) : type(type) {}
};

struct Point : public ColliderBase
{
	Point(const Vector3& position) : position(position), ColliderBase(ColliderType::Point) {};

	Vector3 position;
};

struct AABB : public ColliderBase
{
	AABB() : lowerBound(Vector3::Zero), upperBound(Vector3::Zero), ColliderBase(ColliderType::AlignedBox) {};
	AABB(const Vector3& lowerBound, const Vector3& upperBound)
		: lowerBound(lowerBound), upperBound(upperBound), ColliderBase(ColliderType::AlignedBox) {};

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

	static AABB FromTriangle(const Vector3& p1, const Vector3& p2, const Vector3& p3)
	{
		Vector3 min = Vector3::Min(p1, Vector3::Min(p2, p3));
		Vector3 max = Vector3::Max(p1, Vector3::Max(p2, p3));
		return { min, max };
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

	inline AABB toAABB()
	{
		// Compute the absolute values of the axes scaled by the half extents.
		Vector3 absExtent = Vector3(
			fabsf(axes[0].x) * halfExtents.x + fabsf(axes[0].y) * halfExtents.y + fabsf(axes[0].z) * halfExtents.z,
			fabsf(axes[1].x) * halfExtents.x + fabsf(axes[1].y) * halfExtents.y + fabsf(axes[1].z) * halfExtents.z,
			fabsf(axes[2].x) * halfExtents.x + fabsf(axes[2].y) * halfExtents.y + fabsf(axes[2].z) * halfExtents.z
		);

		// Compute the min and max of the AABB.
		Vector3 aabbMin = center - absExtent;
		Vector3 aabbMax = center + absExtent;

		return AABB(aabbMin, aabbMax);
	}

	Matrix4 GetRotationMatrix() const
	{
		return Matrix4::FromRotationPosition(Matrix3::FromRotationVectors(axes[0], axes[1], axes[2]), Vector3::Zero);
	}

	Matrix4 GetTransformMatrix() const
	{
		return Matrix4::FromRotationPosition(Matrix3::FromRotationVectors(axes[0], axes[1], axes[2]), center);
	}

	std::vector<Vector3> GetVertices() const {
		std::vector<Vector3> vertices(8);

		// Compute the corner offsets using the half extents and axes
		Vector3 right = axes[0] * halfExtents.x;
		Vector3 up = axes[1] * halfExtents.y;
		Vector3 forward = axes[2] * halfExtents.z;

		// Compute the 8 vertices
		vertices[0] = center + right + up + forward;  // +x, +y, +z
		vertices[1] = center + right + up - forward;  // +x, +y, -z
		vertices[2] = center + right - up + forward;  // +x, -y, +z
		vertices[3] = center + right - up - forward;  // +x, -y, -z
		vertices[4] = center - right + up + forward;  // -x, +y, +z
		vertices[5] = center - right + up - forward;  // -x, +y, -z
		vertices[6] = center - right - up + forward;  // -x, -y, +z
		vertices[7] = center - right - up - forward;  // -x, -y, -z

		return vertices;
	}

	// Given an index 0,1,2, returns the four vertices of the face whose normal is axes[index] (positive face)
	std::vector<Vector3> GetFaceVertices(int axisIndex, bool positiveFace = true) const 
	{
		std::vector<Vector3> boxVertices = GetVertices();

		Vector3 normal = axes[axisIndex] * (positiveFace ? 1.0f : -1.0f);
		Vector3 faceCenter = center + normal * halfExtents[axisIndex];

		int i = (axisIndex + 1) % 3;
		int j = (axisIndex + 2) % 3;

		Vector3 edge1 = axes[i] * halfExtents[i];
		Vector3 edge2 = axes[j] * halfExtents[j];

		return { faceCenter - edge1 - edge2,
				 faceCenter + edge1 - edge2,
				 faceCenter + edge1 + edge2,
				 faceCenter - edge1 + edge2 };
	}

	/*void GetMinMaxVertexOnAxis(const Vector3& axis, float& minOut, float& maxOut) const
	{
		float centerProjection = Vector3::Dot(center, axis);

		float extent = 0.0f;
		for (int i = 0; i < 3; i++)
		{
			extent += fabsf(Vector3::Dot(axes[i], axis)) * halfExtents[i];
		}

		minOut = centerProjection - extent;
		maxOut = centerProjection + extent;
	}*/
};

struct Sphere : public ColliderBase
{
	Sphere(Vector3 center, float radius) : ColliderBase(ColliderType::Sphere), center(center), radius(radius) {}

	Vector3 center;
	float radius;
};

struct HalfSpaceTriangle : public ColliderBase
{
	HalfSpaceTriangle(const Vector3& point1, const Vector3& point2, const Vector3& point3) : ColliderBase(ColliderType::HalfSpaceTriangle), 
		point1(point1), point2(point2), point3(point3) {}

	Vector3 point1;
	Vector3 point2;
	Vector3 point3;
};