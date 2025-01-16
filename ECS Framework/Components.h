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
	Vector3 LinearVelocity = Vector3::Zero;

	// Holds the acceleration of the particle.
	Vector3 LinearAcceleration = Vector3::Zero;

	// Holds the amount of damping applied to linear motion.
	float damping;

	// Holds the inverse of the mass of the particle.
	float inverseMass;

	float restitution;
};

struct RigidBody
{
	Vector3 AngularVelocity = Vector3::Zero;

	Vector3 AngularAcceleration = Vector3::Zero;

	Vector3 inverseInertiaTensor = Vector3::Zero;
};

struct SphereCollider
{
	Vector3 center;
	float radius;
};