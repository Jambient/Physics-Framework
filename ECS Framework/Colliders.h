#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix3.h"
#include "Plane.h"
#include <vector>

enum class ColliderType
{
	Plane,
	Sphere,
	AlignedBox,
	OrientedBox
};

const int ColliderTypeCount = 4;

struct ColliderBase
{
	ColliderType type;

	ColliderBase(ColliderType type) : type(type) {}
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

class Face
{
public:
	std::vector<Vector3> vertices;

	Face(const std::vector<Vector3>& vertices) : vertices(vertices) {}

	// Compute the normal of the polygon assuming counterclockwise vertices
	Vector3 GetNormal() const {
		Vector3 edge1 = vertices[1] - vertices[0];
		Vector3 edge2 = vertices[2] - vertices[0];
		return Vector3::Cross(edge1, edge2).normalized();
	}

	// Create planes from the edges of the polygon
	std::vector<Plane> GetEdgePlanes() const {
		std::vector<Plane> edgePlanes;
		Vector3 normal = GetNormal();
		size_t vertexCount = vertices.size();

		for (size_t i = 0; i < vertexCount; ++i) {
			Vector3 v1 = vertices[i];
			Vector3 v2 = vertices[(i + 1) % vertexCount];
			Vector3 edge = v2 - v1;
			Vector3 edgeNormal = Vector3::Cross(edge, normal).normalized();
			edgePlanes.push_back(Plane(edgeNormal, v1));
		}

		return edgePlanes;
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

	Matrix3 GetTransform() const
	{
		return Matrix3::FromRotationVectors(axes[0], axes[1], axes[2]);
	}

	std::vector<Vector3> GetVertices() const {
		std::vector<Vector3> vertices;
		for (int x = -1; x <= 1; x += 2) {
			for (int y = -1; y <= 1; y += 2) {
				for (int z = -1; z <= 1; z += 2) {
					Vector3 offset = axes[0] * (x * halfExtents.x) +
						axes[1] * (y * halfExtents.y) +
						axes[2] * (z * halfExtents.z);
					vertices.push_back(center + offset);
				}
			}
		}
		return vertices;
	}

	// Generate the 6 faces of the OBB as polygons
	std::vector<Face> GetFaces() const {
		std::vector<Face> faces;
		Vector3 faceCenters[6] = {
			center + axes[0] * halfExtents.x, center - axes[0] * halfExtents.x,
			center + axes[1] * halfExtents.y, center - axes[1] * halfExtents.y,
			center + axes[2] * halfExtents.z, center - axes[2] * halfExtents.z
		};

		Vector3 faceNormals[6] = {
			axes[0], -axes[0],
			axes[1], -axes[1],
			axes[2], -axes[2]
		};

		for (int i = 0; i < 6; ++i) {
			Vector3 normal = faceNormals[i];
			Vector3 tangent1 = (i < 2) ? axes[1] : (i < 4) ? axes[2] : axes[0];
			Vector3 tangent2 = Vector3::Cross(normal, tangent1).normalized();

			std::vector<Vector3> vertices = {
				faceCenters[i] + tangent1 * halfExtents.x + tangent2 * halfExtents.y,
				faceCenters[i] - tangent1 * halfExtents.x + tangent2 * halfExtents.y,
				faceCenters[i] - tangent1 * halfExtents.x - tangent2 * halfExtents.y,
				faceCenters[i] + tangent1 * halfExtents.x - tangent2 * halfExtents.y
			};

			faces.push_back(Face(vertices));
		}

		return faces;
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