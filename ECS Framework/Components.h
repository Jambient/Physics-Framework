#pragma once
#include "Vector3.h"
#include "Quaternion.h"

struct Transform
{
	Vector3 Position = Vector3::Zero;
	Quaternion Rotation;
	Vector3 Scale = Vector3::Zero;
};

struct Particle
{
	// Holds the linear velocity of the particle in world space.
	Vector3 Velocity = Vector3::Zero;

	// Holds the acceleration of the particle.
	Vector3 Acceleration = Vector3::Zero;

	// Holds the amount of damping applied to linear motion.
	float damping;

	// Holds the inverse of the mass of the particle.
	float inverseMass;
};

struct SphereCollider
{
	Vector3 center;
	float radius;
};