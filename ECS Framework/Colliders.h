#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "Plane.h"
#include <vector>

enum class ColliderType
{
	POINT,
	HALF_SPACE_TRIANGLE,
	SPHERE,
	ALIGNED_BOX,
	ORIENTED_BOX
};

const int ColliderTypeCount = 5;

class ColliderBase
{
public:
	ColliderBase(ColliderType type) : m_type(type) {}
	
	ColliderType GetType() const { return m_type; }

private:
	ColliderType m_type;
};

class Point : public ColliderBase
{
public:
	Point(const Vector3& position) : m_position(position), ColliderBase(ColliderType::POINT) {};

	void SetPosition(const Vector3& newPosition) { m_position = newPosition; }
	Vector3 GetPosition() const { return m_position; }

private:
	Vector3 m_position;
};

class AABB : public ColliderBase
{
public:
	AABB() : m_lowerBound(Vector3::Zero), m_upperBound(Vector3::Zero), ColliderBase(ColliderType::ALIGNED_BOX) {};
	AABB(const Vector3& lowerBound, const Vector3& upperBound)
		: m_lowerBound(lowerBound), m_upperBound(upperBound), ColliderBase(ColliderType::ALIGNED_BOX) {};

	Vector3 GetLowerBound() const { return m_lowerBound; }
	Vector3 GetUpperBound() const { return m_upperBound; }

	Vector3 GetPosition() const
	{
		return (m_lowerBound + m_upperBound) * 0.5f;
	}

	Vector3 GetSize() const
	{
		return m_upperBound - m_lowerBound;
	}

	float GetArea() const
	{
		Vector3 d = m_upperBound - m_lowerBound;
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	AABB GetEnlarged(float factor) const
	{
		Vector3 margin = (m_upperBound - m_lowerBound) * factor;
		return { m_lowerBound - margin, m_upperBound + margin };
	}

	void UpdatePosition(const Vector3& position)
	{
		Vector3 delta = position - GetPosition();
		m_lowerBound += delta;
		m_upperBound += delta;
	}

	void UpdateScale(const Vector3& scale)
	{
		Vector3 position = GetPosition();
		m_lowerBound = position - scale * 0.5f;
		m_upperBound = position + scale * 0.5f;
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
		return { Vector3::Min(a.GetLowerBound(), b.GetLowerBound()), Vector3::Max(a.GetUpperBound(), b.GetUpperBound()) };
	}

	static bool Overlap(const AABB& a, const AABB b)
	{
		Vector3 lowerBoundA = a.GetLowerBound();
		Vector3 upperBoundA = a.GetUpperBound();
		Vector3 lowerBoundB = b.GetLowerBound();
		Vector3 upperBoundB = b.GetUpperBound();
		return (lowerBoundA.x <= upperBoundB.x && upperBoundA.x >= lowerBoundB.x) &&
			(lowerBoundA.y <= upperBoundB.y && upperBoundA.y >= lowerBoundB.y) &&
			(lowerBoundA.z <= upperBoundB.z && upperBoundA.z >= lowerBoundB.z);
	}

private:
	Vector3 m_lowerBound;
	Vector3 m_upperBound;
};

class OBB : public ColliderBase
{
public:
	OBB(Vector3 position, Vector3 size, Quaternion rotation) : ColliderBase(ColliderType::ORIENTED_BOX)
	{
		Update(position, size, rotation);
	}

	Vector3 GetCenter() const { return m_center; }
	Vector3 GetHalfExtents() const { return m_halfExtents; }
	Vector3 GetAxis(int index) const { return m_axes[index]; }

	inline void Update(Vector3 position, Vector3 size, Quaternion rotation)
	{
		m_center = position;
		m_halfExtents = size * 0.5f;

		m_axes[0] = rotation * Vector3::Right;
		m_axes[1] = rotation * Vector3::Up;
		m_axes[2] = rotation * Vector3::Forward;
	}

	inline AABB ToAABB()
	{
		Vector3 absExtent = Vector3(
			fabsf(m_axes[0].x) * m_halfExtents.x + fabsf(m_axes[1].x) * m_halfExtents.y + fabsf(m_axes[2].x) * m_halfExtents.z,
			fabsf(m_axes[0].y) * m_halfExtents.x + fabsf(m_axes[1].y) * m_halfExtents.y + fabsf(m_axes[2].y) * m_halfExtents.z,
			fabsf(m_axes[0].z) * m_halfExtents.x + fabsf(m_axes[1].z) * m_halfExtents.y + fabsf(m_axes[2].z) * m_halfExtents.z
		);

		Vector3 aabbMin = m_center - absExtent;
		Vector3 aabbMax = m_center + absExtent;

		return AABB(aabbMin, aabbMax);
	}

	Matrix4 GetRotationMatrix() const
	{
		return Matrix4::FromRotationPosition(Matrix3::FromRotationVectors(m_axes[0], m_axes[1], m_axes[2]), Vector3::Zero);
	}

	Matrix4 GetTransformMatrix() const
	{
		return Matrix4::FromRotationPosition(Matrix3::FromRotationVectors(m_axes[0], m_axes[1], m_axes[2]), m_center);
	}

	std::vector<Vector3> GetVertices() const {
		std::vector<Vector3> vertices(8);

		// Compute the corner offsets using the half extents and axes
		Vector3 right = m_axes[0] * m_halfExtents.x;
		Vector3 up = m_axes[1] * m_halfExtents.y;
		Vector3 forward = m_axes[2] * m_halfExtents.z;

		// Compute the 8 vertices
		vertices[0] = m_center + right + up + forward;  // +x, +y, +z
		vertices[1] = m_center + right + up - forward;  // +x, +y, -z
		vertices[2] = m_center + right - up + forward;  // +x, -y, +z
		vertices[3] = m_center + right - up - forward;  // +x, -y, -z
		vertices[4] = m_center - right + up + forward;  // -x, +y, +z
		vertices[5] = m_center - right + up - forward;  // -x, +y, -z
		vertices[6] = m_center - right - up + forward;  // -x, -y, +z
		vertices[7] = m_center - right - up - forward;  // -x, -y, -z

		return vertices;
	}

	// Given an index 0,1,2, returns the four vertices of the face whose normal is axes[index] (positive face)
	std::vector<Vector3> GetFaceVertices(int axisIndex, bool positiveFace = true) const 
	{
		std::vector<Vector3> boxVertices = GetVertices();

		Vector3 normal = m_axes[axisIndex] * (positiveFace ? 1.0f : -1.0f);
		Vector3 faceCenter = m_center + normal * m_halfExtents[axisIndex];

		int i = (axisIndex + 1) % 3;
		int j = (axisIndex + 2) % 3;

		Vector3 edge1 = m_axes[i] * m_halfExtents[i];
		Vector3 edge2 = m_axes[j] * m_halfExtents[j];

		return { faceCenter - edge1 - edge2,
				 faceCenter + edge1 - edge2,
				 faceCenter + edge1 + edge2,
				 faceCenter - edge1 + edge2 };
	}

	float ProjectOntoAxis(const Vector3& axis) const
	{
		return m_halfExtents.x * fabsf(Vector3::Dot(axis, m_axes[0])) +
			m_halfExtents.y * fabsf(Vector3::Dot(axis, m_axes[1])) +
			m_halfExtents.z * fabsf(Vector3::Dot(axis, m_axes[2]));
	}

private:
	Vector3 m_center;
	Vector3 m_halfExtents;
	std::array<Vector3, 3> m_axes;
};

class Sphere : public ColliderBase
{
public:
	Sphere(Vector3 center, float radius) : ColliderBase(ColliderType::SPHERE), m_center(center), m_radius(radius) {}

	Vector3 GetCenter() const { return m_center; }
	void SetCenter(Vector3 newCenter) { m_center = newCenter; }

	float GetRadius() const { return m_radius; }
	void SetRadius(float newRadius) { m_radius = newRadius; }

private:
	Vector3 m_center;
	float m_radius;
};

class HalfSpaceTriangle : public ColliderBase
{
public:
	HalfSpaceTriangle(const Vector3& point1, const Vector3& point2, const Vector3& point3, const Vector3& normal) : ColliderBase(ColliderType::HALF_SPACE_TRIANGLE)
	{
		m_points = { point1, point2, point3 };
		m_normal = normal;
	}

	Vector3 GetPoint(int index) const { return m_points[index]; }
	Vector3 GetNormal() const { return m_normal; }

private:
	std::array<Vector3, 3> m_points;
	Vector3 m_normal;
};