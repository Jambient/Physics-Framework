#pragma once
#include <math.h>

class Vector3
{
public:
	float x;
	float y;
	float z;

	static const Vector3 Forward;
	static const Vector3 Back;
	static const Vector3 Left;
	static const Vector3 Right;
	static const Vector3 Up;
	static const Vector3 Down;
	static const Vector3 One;
	static const Vector3 Zero;

	Vector3() : x(0.0f), y(0.0f), z(0.0f) {};
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {};

	float magnitude() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	float sqrMagnitude() const
	{
		return x * x + y * y + z * z;
	}

	void normalize()
	{
		*this = normalized();
	}

	Vector3 normalized() const
	{
		float mag = magnitude();
		return mag > 0.0f ? *this * (1.0f / mag) : Vector3::Zero;
	}

	Vector3 reciprocal() const
	{
		return Vector3(1 / x, 1 / y, 1 / z);
	}

	Vector3 operator+(const Vector3& other) const
	{
		return Vector3(x + other.x, y + other.y, z + other.z);
	}

	Vector3 operator-(const Vector3& other) const
	{
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	Vector3 operator*(float scalar) const
	{
		return Vector3(x * scalar, y * scalar, z * scalar);
	}

	void operator+=(const Vector3& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
	}

	void operator-=(const Vector3& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
	}

	void operator*=(const Vector3& other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
	}

	void operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
	}

	bool operator==(const Vector3& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator!=(const Vector3& other) const
	{
		return !(*this == other);
	}

	bool operator<(const Vector3& other) const
	{
		return x < other.x && y < other.y && z < other.z;
	}

	bool operator>(const Vector3& other) const
	{
		return x > other.x && y > other.y && z > other.z;
	}

	Vector3 operator-() const
	{
		return Vector3(-x, -y, -z);
	}

	static float Angle(const Vector3& a, const Vector3& b)
	{
		return acosf(Dot(a, b) / (a.magnitude() * b.magnitude()));
	}

	static Vector3 Cross(const Vector3& a, const Vector3& b)
	{
		return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
	}

	static float Distance(const Vector3& a, const Vector3& b)
	{
		return (b - a).magnitude();
	}

	static float Dot(const Vector3& a, const Vector3& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
	{
		// lerping is done like this rather than the standard a + (b - a) * t
		// because this is supposedly more reliable for the bounds t = 0 and t = 1.
		return a * (1 - t) + b * t;
	}

	static Vector3 Scale(const Vector3& a, const Vector3& b)
	{
		return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
	}

	static void OrthoNormalize(Vector3& a, Vector3& b, Vector3& c)
	{
		a.normalize();
		c = Cross(b, a);
		if (c.sqrMagnitude() == 0.0f) return;
		c.normalize();
		b = Cross(a, c);
	}

private:
	//float pad;
};

