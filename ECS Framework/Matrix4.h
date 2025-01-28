#pragma once
#include "Vector3.h"
#include "Matrix3.h"
#include <array>

class Matrix4
{
public:
	std::array<float, 16> values;

	Matrix4()
	{
		values = {
			1.0f, 0, 0, 0,
			0, 1.0f, 0, 0,
			0, 0, 1.0f, 0,
			0, 0, 0, 1.0f
		};
	}

	// this in an optimised inverse that only works on affine (transform) matrices
	Matrix4 inverseAffine()
	{
		Matrix4 result;

		Vector3 uVec = Vector3(values[0], values[1], values[2]);
		Vector3 vVec = Vector3(values[4], values[5], values[6]);
		Vector3 wVec = Vector3(values[8], values[9], values[10]);
		Vector3 tVec = Vector3(values[12], values[13], values[14]);

		// Transpose the rotation part (invert rotation matrix)
		result.values[0] = uVec.x;
		result.values[1] = vVec.x;
		result.values[2] = wVec.x;
		result.values[3] = 0;

		result.values[4] = uVec.y;
		result.values[5] = vVec.y;
		result.values[6] = wVec.y;
		result.values[7] = 0;

		result.values[8] = uVec.z;
		result.values[9] = vVec.z;
		result.values[10] = wVec.z;
		result.values[11] = 0;

		// Compute -dot(u, t), -dot(v, t), -dot(w, t)
		result.values[12] = -Vector3::Dot(uVec, tVec);
		result.values[13] = -Vector3::Dot(vVec, tVec);
		result.values[14] = -Vector3::Dot(wVec, tVec);
		result.values[15] = 1;

		return result;
	}

	Matrix4 transpose() const
	{
		Matrix4 result;

		result.values[0] = values[0];
		result.values[1] = values[4];
		result.values[2] = values[8];
		result.values[3] = values[12];

		result.values[4] = values[1];
		result.values[5] = values[5];
		result.values[6] = values[9];
		result.values[7] = values[13];

		result.values[8] = values[2];
		result.values[9] = values[6];
		result.values[10] = values[10];
		result.values[11] = values[14];

		result.values[12] = values[3];
		result.values[13] = values[7];
		result.values[14] = values[11];
		result.values[15] = values[15];

		return result;
	}

	Vector3 operator*(const Vector3& vec) const
	{
		return Vector3(
			values[0] * vec.x + values[1] * vec.y + values[2] * vec.z + values[12],
			values[4] * vec.x + values[5] * vec.y + values[6] * vec.z + values[13],
			values[8] * vec.x + values[9] * vec.y + values[10] * vec.z + values[14]
		);
	}

	static Matrix4 FromRotationPosition(const Matrix3& rotation, const Vector3& position)
	{
		Matrix4 mat;
		mat.values[0] = rotation.values[0];
		mat.values[1] = rotation.values[1];
		mat.values[2] = rotation.values[2];
		mat.values[3] = 0;

		mat.values[4] = rotation.values[3];
		mat.values[5] = rotation.values[4];
		mat.values[6] = rotation.values[5];
		mat.values[7] = 0;

		mat.values[8] = rotation.values[6];
		mat.values[9] = rotation.values[7];
		mat.values[10] = rotation.values[8];
		mat.values[11] = 0;

		mat.values[12] = position.x;
		mat.values[13] = position.y;
		mat.values[14] = position.z;
		mat.values[15] = 1;

		return mat;
	}
};