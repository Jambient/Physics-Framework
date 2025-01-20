#pragma once
#include "Vector3.h"

// row major matrix
class Matrix3
{
public:
	float values[9];

	Matrix3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
	{
		Set(m00, m01, m02, m10, m11, m12, m20, m21, m22);
	}
	Matrix3() { ToIdentity(); }
	Matrix3(const Vector3& vector) { ToScale(vector); }

	void Set(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
	{
		values[0] = m00;
		values[1] = m01;
		values[2] = m02;
		values[3] = m10;
		values[4] = m11;
		values[5] = m12;
		values[6] = m20;
		values[7] = m21;
		values[8] = m22;
	}

	void ToIdentity()
	{
		Set(1, 0, 0, 0, 1, 0, 0, 0, 1);
	}

	void ToScale(const Vector3& vector)
	{
		Set(vector.x, 0, 0, 0, vector.y, 0, 0, 0, vector.z);
	}

	Matrix3 operator*(const Matrix3& other) const
	{
		return Matrix3(
			// Row 0
			values[0] * other.values[0] + values[1] * other.values[3] + values[2] * other.values[6],
			values[0] * other.values[1] + values[1] * other.values[4] + values[2] * other.values[7],
			values[0] * other.values[2] + values[1] * other.values[5] + values[2] * other.values[8],

			// Row 1
			values[3] * other.values[0] + values[4] * other.values[3] + values[5] * other.values[6],
			values[3] * other.values[1] + values[4] * other.values[4] + values[5] * other.values[7],
			values[3] * other.values[2] + values[4] * other.values[5] + values[5] * other.values[8],

			// Row 2
			values[6] * other.values[0] + values[7] * other.values[3] + values[8] * other.values[6],
			values[6] * other.values[1] + values[7] * other.values[4] + values[8] * other.values[7],
			values[6] * other.values[2] + values[7] * other.values[5] + values[8] * other.values[8]
		);
	}

	Matrix3& operator*=(const Matrix3& other)
	{
		*this = *this * other;
		return *this;
	}

	Vector3 operator*(const Vector3& vec) const
	{
		return Vector3(
			values[0] * vec.x + values[1] * vec.y + values[2] * vec.z,
			values[3] * vec.x + values[4] * vec.y + values[5] * vec.z,
			values[6] * vec.x + values[7] * vec.y + values[8] * vec.z
		);
	}

	static Matrix3 FromRotationVectors(const Vector3& right, const Vector3& up, const Vector3& forward)
	{
		Matrix3 mat;
		mat.Set(right.x, right.y, right.z, up.x, up.y, up.z, forward.x, forward.y, forward.z);
		return mat;
	}
};