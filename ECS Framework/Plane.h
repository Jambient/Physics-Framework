#pragma once
#include "Vector3.h"
#include <vector>

class Plane
{
public:
    Plane() {};
	Plane(const Vector3& point, const Vector3& normal) : m_point(point), m_normal(normal.normalized()) {};

	Vector3 GetPoint() const { return m_point; }
	Vector3 GetNormal() const { return m_normal; }

	float DistanceToPoint(const Vector3& p) const
	{
		return Vector3::Dot(p - m_point, m_normal);
	}

	std::vector<Vector3> ClipPolygon(const std::vector<Vector3>& polygon) const
	{
        std::vector<Vector3> clipped;

        size_t count = polygon.size();
        for (size_t i = 0; i < count; ++i) {
            const Vector3& curr = polygon[i];
            const Vector3& prev = polygon[(i + count - 1) % count];

            float currDist = DistanceToPoint(curr);
            float prevDist = DistanceToPoint(prev);

            // If current point is inside (or on) the plane, add it.
            if (currDist >= 0)
            {
                // If previous point was outside, add the intersection first.
                if (prevDist < 0) {
                    float t = prevDist / (prevDist - currDist);
                    Vector3 intersect = prev + (curr - prev) * t;
                    clipped.push_back(intersect);
                }
                clipped.push_back(curr);
            }
            // Else, if current is outside but previous was inside, add intersection.
            else if (prevDist >= 0) {
                float t = prevDist / (prevDist - currDist);
                Vector3 intersect = prev + (curr - prev) * t;
                clipped.push_back(intersect);
            }
        }

        return clipped;
	}

private:
	Vector3 m_point;
	Vector3 m_normal;
};