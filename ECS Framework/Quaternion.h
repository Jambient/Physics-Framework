#pragma once
#define _USE_MATH_DEFINES

#include <math.h>
#include <array>
#include "Vector3.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "Definitions.h"

// this quaternion class was made with great help from https://www.3dgep.com/understanding-quaternions/ 
// which goes through a large amount of the required equations.

// https://allenchou.net/2014/02/game-physics-broadphase-dynamic-aabb-tree/

class Quaternion
{
public:
	float r;
	float i;
	float j;
	float k;

	Quaternion() : r(1.0f), i(0.0f), j(0.0f), k(0.0f) {};
	Quaternion(float r, float i, float j, float k) : r(r), i(i), j(j), k(k) {};
	Quaternion(float r, const Vector3& vector) : r(r), i(vector.x), j(vector.y), k(vector.z) {};

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
		Quaternion normQ = normalized();
		Vector3 angles;

		// roll (x-axis rotation)
		float sinr_cosp = 2 * (normQ.r * normQ.i + normQ.j * normQ.k);
		float cosr_cosp = 1 - 2 * (normQ.i * normQ.i + normQ.j * normQ.j);
		angles.x = atan2f(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float sinp = 2 * (normQ.r * normQ.j - normQ.k * normQ.i);
		if (fabs(sinp) >= 1)
			angles.y = copysignf(M_PI / 2.0f, sinp); // Use 90 degrees if out of range
		else
			angles.y = asinf(sinp);

		// yaw (z-axis rotation)
		float siny_cosp = 2 * (normQ.r * normQ.k + normQ.i * normQ.j);
		float cosy_cosp = 1 - 2 * (normQ.j * normQ.j + normQ.k * normQ.k);
		angles.z = atan2f(siny_cosp, cosy_cosp);

		return angles;
	}

	float* toRotationMatrix() const
	{
		//std::array<float, 16> rotMat;

		//rotMat[0] = 2 * (r * r + i * i) - 1;
		//rotMat[1] = 2 * (i * j - r * k);
		//rotMat[2] = 2 * (i * k + r * j);
		//rotMat[3] = 0;

		//// Second row of the rotation matrix
		//rotMat[4] = 2 * (i * j + r * k);
		//rotMat[5] = 2 * (r * r + j * j) - 1;
		//rotMat[6] = 2 * (j * k - r * i);
		//rotMat[7] = 0;

		//// Third row of the rotation matrix
		//rotMat[8] = 2 * (i * k - r * j);
		//rotMat[9] = 2 * (j * k + r * i);
		//rotMat[10] = 2 * (r * r + k * k) - 1;
		//rotMat[11] = 0;

		// Fourth row (homogeneous coordinates)
		/*rotMat[12] = 0;
		rotMat[13] = 0;
		rotMat[14] = 0;
		rotMat[15] = 1;*/

		Vector3 right = (*this) * Vector3::Right;
		Vector3 up = (*this) * Vector3::Up;
		Vector3 forward = (*this) * Vector3::Forward;

		Matrix4 mat = Matrix4::FromRotationPosition(Matrix3::FromRotationVectors(right, up, forward), Vector3::Zero);

		return mat.values.data();
	}

	// TODO: I NEED TO MERGE THIS METHOD WITH THE ABOVE ONE
	Matrix3 toMatrix3() const 
	{
		Matrix3 mat;

		// x = i, y = j, z = k, w = r

		float yy = j * j;
		float zz = k * k;
		float xy = i * j;
		float zw = k * r;
		float xz = i * k;
		float yw = j * r;
		float xx = i * i;
		float yz = j * k;
		float xw = i * r;

		mat.values[0] = 1 - 2 * yy - 2 * zz;
		mat.values[1] = 2 * xy + 2 * zw;
		mat.values[2] = 2 * xz - 2 * yw;

		mat.values[3] = 2 * xy - 2 * zw;
		mat.values[4] = 1 - 2 * xx - 2 * zz;
		mat.values[5] = 2 * yz + 2 * xw;

		mat.values[6] = 2 * xz + 2 * yw;
		mat.values[7] = 2 * yz - 2 * xw;
		mat.values[8] = 1 - 2 * xx - 2 * yy;

		return mat;
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
		/*return Quaternion(
            r * other.r - i * other.i - j * other.j - k * other.k,
            r * other.i + other.r * i + j * other.k - other.j * k,
            r * other.j + other.r * j + k * other.i - other.k * i,
            r * other.k + other.r * k + i * other.j - other.i * j
        );*/

		return Quaternion(r * other.r - i * other.i
			- j * other.j - k * other.k,
			r * other.i + i * other.r
			+ j * other.k - k * other.j,
			r * other.j + j * other.r
			+ k * other.i - i * other.k,
			r * other.k + k * other.r
			+ i * other.j - j * other.i);
	}

	Vector3 operator*(const Vector3& v) const
	{
		Quaternion vectorQuat(0, v);
		Quaternion result = (*this) * vectorQuat * this->conjugate();
		return Vector3(result.i, result.j, result.k);
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
		a.normalize();
		b.normalize();

		float angle = Angle(a, b);
		float sinAngle = sin(angle);

		// this stops divide by zero errors for small angles
		if (fabsf(sinAngle) < EPSILON) { return Lerp(a, b, t); }

		// this stops the interpolation travelling the "long-way" around the 4D sphere in some cases
		float dot = Dot(a, b);
		Quaternion adjustedA = dot < 0 ? -a : a;

		float factorA = sin((1.0f - t) * angle) / sinAngle;
		float factorB = sin(t * angle) / sinAngle;

		// Perform the spherical linear interpolation
		return adjustedA * factorA + b * factorB;
	}

	// code from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles.
	// angles are in radians -> roll, pitch, yaw
	static Quaternion FromEulerAngles(const Vector3& angles)
	{
		Quaternion q;
		float roll = angles.x;
		float pitch = angles.y;
		float yaw = angles.z;

		float cyaw, cpitch, croll, syaw, spitch, sroll;
		float cyawcpitch, syawspitch, cyawspitch, syawcpitch;

		cyaw = cosf(0.5f * yaw);
		cpitch = cosf(0.5f * pitch);
		croll = cosf(0.5f * roll);
		syaw = sinf(0.5f * yaw);
		spitch = sinf(0.5f * pitch);
		sroll = sinf(0.5f * roll);

		cyawcpitch = cyaw * cpitch;
		syawspitch = syaw * spitch;
		cyawspitch = cyaw * spitch;
		syawcpitch = syaw * cpitch;

		q.r = cyawcpitch * croll + syawspitch * sroll;
		q.i = cyawcpitch * sroll - syawspitch * croll;
		q.j = cyawspitch * croll + syawcpitch * sroll;
		q.k = syawcpitch * croll - cyawspitch * sroll;

		return q;
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
};

