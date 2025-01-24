#pragma once
#include <math.h>
#include <iostream>

class Vector3
{
public:
	float x;
	float y;
	float z;

	// shorthand for (0, 0, 1)
	static const Vector3 Forward;

	// shorthand for (0, 0, -1)
	static const Vector3 Back;

	// shorthand for (-1, 0, 0)
	static const Vector3 Left;

	// shorthand for (1, 0, 0)
	static const Vector3 Right;

	// shorthand for (0, 1, 0)
	static const Vector3 Up;

	// shorthand for (0, -1, 0)
	static const Vector3 Down;

	// shorthand for (1, 1, 1)
	static const Vector3 One;

	// shorthand for (0, 0, 0)
	static const Vector3 Zero;

	Vector3() : x(0.0f), y(0.0f), z(0.0f) {};
	Vector3(float s) : x(s), y(s), z(s) {};
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

	float& operator[](int index) {
		switch (index)
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		default:
			throw std::out_of_range("Index out of range for Vector3");
		}
	}

	const float& operator[](int index) const {
		switch (index) 
		{
		case 0: return x;
		case 1: return y;
		case 2: return z;
		default:
			throw std::out_of_range("Index out of range for Vector3");
		}
	}

	friend std::ostream& operator<<(std::ostream& os, const Vector3& vec)
	{
		os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
		return os;
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

	static inline Vector3 Min(const Vector3& a, const Vector3& b)
	{
		return Vector3(
			a.x < b.x ? a.x : b.x,
			a.y < b.y ? a.y : b.y,
			a.z < b.z ? a.z : b.z
		);
	}

	static inline Vector3 Max(const Vector3& a, const Vector3& b)
	{
		return Vector3(
			a.x > b.x ? a.x : b.x,
			a.y > b.y ? a.y : b.y,
			a.z > b.z ? a.z : b.z
		);
	}

	static inline Vector3 Clamp(const Vector3& v, const Vector3& minV, const Vector3& maxV)
	{
		return Min(maxV, Max(minV, v));
	}

private:
	//float pad;
};

