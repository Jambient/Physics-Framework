#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix3.h"
#include "Colliders.h"
#include "Definitions.h"
#include <variant>

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

	// Holds the last frames acceleration
	Vector3 LastLinearAcceleration = Vector3::Zero;

	// Holds the acceleration of the particle.
	Vector3 Force = Vector3::Zero;

	// Holds the inverse of the mass of the particle.
	float InverseMass;

	// Coefficient of Restitution.
	float Restitution;

	void ApplyLinearImpulse(const Vector3& force)
	{
		LinearVelocity += force * InverseMass;
	}
};

struct RigidBody
{
	Vector3 AngularVelocity = Vector3::Zero;

	Vector3 Torque = Vector3::Zero;

	Vector3 InverseInertia = Vector3::Zero;

	Matrix3 InverseInertiaTensor;

	void ApplyAngularImpuse(const Vector3& force)
	{
		AngularVelocity += InverseInertiaTensor * force;
	}
};

struct PhysicsMaterial
{
	float StaticFriction = 0.6f;
	float DynamicFriction = 0.4f;
	// possible coefficient of restitution in here as well
};

struct Mesh
{
	int MeshId;
};

struct Collider
{
	std::variant<OBB, Sphere, AABB, Point, HalfSpaceTriangle> Collider;

	ColliderBase& GetColliderBase()
	{
		return std::visit([](auto& concreteCollider) -> ColliderBase& {
			return static_cast<ColliderBase&>(concreteCollider); // Cast to base class
			}, Collider);
	}
};

struct Spring
{
	Entity Entity1;
	Entity Entity2;
	float RestLength;
	float Stiffness;
};