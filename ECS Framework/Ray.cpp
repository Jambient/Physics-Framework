#include "Ray.h"
#include "Colliders.h"
#include <algorithm>
using namespace std;

Ray::Ray(Vector3 origin, Vector3 direction)
{
	m_origin = origin;
	m_direction = direction;
	m_inverseDirection = direction.reciprocal();
}

bool Ray::Intersect(const AABB& aabb, float& distance) const
{
	Vector3 t1 = Vector3::Scale(aabb.GetLowerBound() - m_origin, m_inverseDirection);
	Vector3 t2 = Vector3::Scale(aabb.GetUpperBound() - m_origin, m_inverseDirection);

	float tmin = max(min(t1.x, t2.x), max(min(t1.y, t2.y), min(t1.z, t2.z)));
	float tmax = min(max(t1.x, t2.x), min(max(t1.y, t2.y), max(t1.z, t2.z)));

	// no intersection occured
	if (tmax < 0 || tmin > tmax)
		return false;

	// Ensure distance is non-negative and valid
	distance = tmin > 0 ? tmin : tmax;
	return distance >= 0;
}
