#pragma once
#define _USE_MATH_DEFINES

#include <math.h>
#include "Vector3.h"

// this quaternion class was made with great help from https://www.3dgep.com/understanding-quaternions/ 
// which goes through a large amount of the required equations.

// https://allenchou.net/2014/02/game-physics-broadphase-dynamic-aabb-tree/

class Quaternion
{
public:
	Quaternion() : r(1.0f), i(0.0f), j(0.0f), k(0.0f) {};
	Quaternion(float r, float i, float j, float k) : r(r), i(i), j(j), k(k) {};

	float magnitude() const
	{
		return sqrtf(r * r + i * i + j * j + k * k);
	}

	float sqrMagnitude() const
	{
		return r * r + i * i + j * j + k * k;
	}

	Quaternion conjugate() const
	{
		return Quaternion(r, -i, -j, -k);
	}

	void normalize()
	{
		*this = normalized();
	}

	Quaternion normalized() const
	{
		float mag = magnitude();
		if (mag == 0)
		{
			return Quaternion(1, 0, 0, 0);
		}

		return *this / mag;
	}

	// code from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles.
	// assumes normalized quaternion
	Vector3 toEulerAngles() const
	{
		Vector3 angles;

		// roll (x-axis rotation)
		float sinr_cosp = 2 * (r * i + j * k);
		float cosr_cosp = 1 - 2 * (i * i + j * j);
		angles.x = atan2f(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float sinp = sqrtf(1 + 2 * (r * j - i * k));
		float cosp = sqrtf(1 - 2 * (r * j - i * k));
		angles.y = 2 * atan2f(sinp, cosp) - M_PI / 2;

		// yaw (z-axis rotation)
		float siny_cosp = 2 * (r * k + i * j);
		float cosy_cosp = 1 - 2 * (j * j + k * k);
		angles.z = atan2f(siny_cosp, cosy_cosp);

		return angles;
	}

	// operator overrides
	Quaternion operator*(float scalar) const
	{
		return Quaternion(r * scalar, i * scalar, j * scalar, k * scalar);
	}

	Quaternion operator/(float scalar) const
	{
		return Quaternion(r / scalar, i / scalar, j / scalar, k / scalar);
	}

	Quaternion operator+(const Quaternion& other) const
	{
		return Quaternion(r + other.r, i + other.i, j + other.j, k + other.k);
	}

	Quaternion operator-(const Quaternion& other) const
	{
		return Quaternion(r - other.r, i - other.i, j - other.j, k - other.k);
	}

	Quaternion operator*(const Quaternion& other) const
	{
		return Quaternion(
            r * other.r - i * other.i - j * other.j - k * other.k,
            r * other.i + other.r * i + j * other.k - other.j * k,
            r * other.j + other.r * j + k * other.i - other.k * i,
            r * other.k + other.r * k + i * other.j - other.i * j
        );
	}

	void operator*=(float scalar)
	{
		*this = *this * scalar;
	}

	void operator/=(float scalar)
	{
		*this = *this / scalar;
	}

	void operator+=(const Quaternion& other)
	{
		*this = *this + other;
	}

	void operator-=(const Quaternion& other)
	{
		*this = *this - other;
	}

	void operator*=(const Quaternion& other)
	{
		*this = *this * other;
	}

	bool operator==(const Quaternion& other) const
	{
		return r == other.r && i == other.i && j == other.j && k == other.k;
	}

	bool operator!=(const Quaternion& other) const
	{
		return !(*this == other);
	}

	Quaternion operator-()
	{
		// this operation is NOT the same as inversing
		// the quaternion is just negated but still represents the same orientation
		return Quaternion(-r, -i, -j, -k);
	}

	// static methods
	static float Angle(const Quaternion& a, const Quaternion& b)
	{
		return acos(Dot(a, b) / (a.magnitude() * b.magnitude()));
	}

	static Quaternion AngleAxis(float angleRadians, const Vector3& v)
	{
		float halfAngle = angleRadians * 0.5f;
		float sinHalfAngle = sin(halfAngle);
		Vector3 vNorm = v.normalized();

		return Quaternion(cos(halfAngle), sinHalfAngle * vNorm.x, sinHalfAngle * vNorm.y, sinHalfAngle * vNorm.z);
	}

	static float Dot(const Quaternion& a, const Quaternion& b)
	{
		return a.r * b.r + a.i * b.i + a.j * b.j + a.k * b.k;
	}

	static Quaternion Inverse(const Quaternion& q)
	{
		float sqrMag = q.sqrMagnitude();
		if (sqrMag == 0)
		{
			return Quaternion(1, 0, 0, 0);
		}

		return q.conjugate() / sqrMag;
	}

	static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t)
	{
		return (a * (1 - t) + b * t).normalized();
	}

	static Quaternion Slerp(Quaternion& a, Quaternion& b, float t)
	{
		float angle = Angle(a, b);
		float sinAngle = sin(angle);

		// this stops divide by zero errors for small angles
		if (sinAngle == 0.0f) { return Lerp(a, b, t); }

		// this stops the interpolation travelling the "long-way" around the 4D sphere in some cases
		float dot = Dot(a, b);
		Quaternion adjustedA = dot < 0 ? -a : a;

		return adjustedA * ((sin(1 - t) * angle) / sinAngle) + b * (sin(t * angle) / sinAngle);
	}

	// code from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles.
	// angles are in radians -> roll, pitch, yaw
	static Quaternion FromEulerAngles(const Vector3& angles)
	{
		double cr = cos(angles.x * 0.5);
		double sr = sin(angles.x * 0.5);
		double cp = cos(angles.y * 0.5);
		double sp = sin(angles.y * 0.5);
		double cy = cos(angles.z * 0.5);
		double sy = sin(angles.z * 0.5);

		return Quaternion(
			cr * cp * cy + sr * sp * sy,
			sr * cp * cy - cr * sp * sy,
			cr * sp * cy + sr * cp * sy,
			cr * cp * sy - sr * sp * cy
		);
	}

	// code from https://gamedev.stackexchange.com/questions/15070/orienting-a-model-to-face-a-target
	static Quaternion FromToRotation(const Vector3& fromDirection, const Vector3& toDirection, const Vector3& up)
	{
		float dot = Vector3::Dot(fromDirection, toDirection);

		// check if vectors point in exactly opposite directions
		if (fabsf(dot - (-1.0f)) < 0.000001f)
		{
			return Quaternion::AngleAxis(M_PI, up);
		}

		// check if vectors point in exactly the same direction
		if (fabsf(dot - (1.0f)) < 0.000001f)
		{
			return Quaternion();
		}

		float rotAngle = acosf(dot);
		Vector3 rotAxis = Vector3::Cross(fromDirection, toDirection);
		return Quaternion::AngleAxis(rotAngle, rotAxis);
	}

private:
	float r;
	float i;
	float j;
	float k;
};

