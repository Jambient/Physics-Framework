#pragma once
#include "Vector3.h"

class Plane
{
public:
	Vector3 normal;
	float distance;

	Plane(const Vector3& normal, float distance) : normal(normal.normalized()), distance(distance) {};

	static Plane FromTri(const Vector3& p0, const Vector3& p1, const Vector3& p2)
	{
		Vector3 normal = Vector3::Cross(p1 - p0, p2 - p0).normalized();
		float d = -Vector3::Dot(p0, normal);

		return Plane(normal, d);
	}
};