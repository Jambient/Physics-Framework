#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix3.h"
#include "Colliders.h"
#include "Definitions.h"
#include <variant>

struct Transform
{
	Transform(Vector3 position, Quaternion rotation, Vector3 scale) : position(position),
		rotation(rotation), scale(scale) {};

	Vector3 position = Vector3::Zero;
	Quaternion rotation;
	Vector3 scale = Vector3::Zero;
};

struct Particle
{
	Particle(float mass) : inverseMass(mass > 0.0f ? 1.0f / mass : 0.0f) {};

	// Holds the linear velocity of the particle in world space.
	Vector3 linearVelocity = Vector3::Zero;

	// Holds the last frames acceleration
	Vector3 lastLinearAcceleration = Vector3::Zero;

	// Holds the acceleration of the particle.
	Vector3 force = Vector3::Zero;

	// Holds the inverse of the mass of the particle.
	float inverseMass;

	void ApplyLinearImpulse(const Vector3& force)
	{
		linearVelocity += force * inverseMass;
	}
};

struct RigidBody
{
	RigidBody(Vector3 inverseInertia) : inverseInertia(inverseInertia), inverseInertiaTensor(inverseInertia) {};

	Vector3 angularVelocity = Vector3::Zero;

	Vector3 torque = Vector3::Zero;

	Vector3 inverseInertia = Vector3::Zero;

	Matrix3 inverseInertiaTensor;

	void ApplyAngularImpuse(const Vector3& force)
	{
		angularVelocity += inverseInertiaTensor * force;
	}
};

struct RenderMaterial
{
	int materialID;
};

struct PhysicsMaterial
{
	float dynamicFriction = 0.5f;
	float restitution = 0.5f;
};

struct Mesh
{
	int meshID;
};

struct Collider
{
	std::variant<OBB, Sphere, AABB, Point, HalfSpaceTriangle> collider;

	ColliderBase& GetColliderBase()
	{
		return std::visit([](auto& concreteCollider) -> ColliderBase& {
			return static_cast<ColliderBase&>(concreteCollider); // Cast to base class
			}, collider);
	}
};

struct Spring
{
	Entity entityA;
	Entity entityB;
	float restLength;
	float stiffness;
};