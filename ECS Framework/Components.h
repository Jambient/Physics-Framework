#pragma once
#include "Vector3.h"
#include "Quaternion.h"

struct Particle
{
	// Holds the linear positon of the particle in world space.
	Vector3 Position = Vector3::Zero;

	// Holds the linear velocity of the particle in world space.
	Vector3 Velocity = Vector3::Zero;

	// Holds the acceleration of the particle.
	Vector3 Acceleration = Vector3::Zero;

	// Holds the amount of damping applied to linear motion.
	float damping;

	// Holds the inverse of the mass of the particle.
	float inverseMass;
};

struct TestRotation
{
	Quaternion rotation;
};

struct SphereCollider
{
	Vector3 center;
	float radius;
};

struct Transform
{
	Vector3 Position = Vector3::Zero;
	Vector3 Rotation = Vector3::Zero;
	Vector3 Scale = Vector3::One;
};

struct Gravity
{
	Vector3 Force = Vector3::Zero;
};

struct RigidBody
{
	Vector3 Velocity = Vector3::Zero;
	Vector3 Acceleration = Vector3::Zero;
};