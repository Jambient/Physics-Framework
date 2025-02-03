#pragma once
#include "Components.h"
#include "Scene.h"

class PhysicsSystem : public System
{
public:
	void Update(float dt)
	{
		float dampingFactor = 0.99f;
		float frameDamping = std::powf(dampingFactor, dt);

		// integrate linear movement
		m_scene->ForEach<Particle, Transform>([dt, frameDamping](Entity entity, Particle* particle, Transform* transform)
			{
				// dont integrate things with infinite mass
				if (particle->InverseMass <= 0.0f) return;

				assert(dt > 0.0);

				Vector3 newAcceleration = particle->Force * particle->InverseMass;
				newAcceleration += Vector3::Down * 9.8f;

				// velocity verlet integration
				transform->Position += particle->LinearVelocity * dt + particle->LastLinearAcceleration * 0.5f * dt * dt;

				particle->LinearVelocity += (particle->LastLinearAcceleration + newAcceleration) * 0.5f * dt;
				particle->LinearVelocity *= frameDamping;

				particle->LastLinearAcceleration = newAcceleration;
				particle->Force = Vector3::Zero;
			});

		// integrate angular movement
		m_scene->ForEach<Particle, RigidBody, Transform>([dt, frameDamping](Entity entity, Particle* particle, RigidBody* rigidBody, Transform* transform)
			{
				// dont integrate things with infinite mass
				if (particle->InverseMass <= 0.0f) return;

				assert(dt > 0.0);

				// update inertia tensor
				Quaternion q = transform->Rotation;
				Matrix3 invOrientation = q.conjugate().toMatrix3();
				Matrix3 orientation = q.toMatrix3();

				rigidBody->InverseInertiaTensor = orientation * Matrix3(rigidBody->InverseInertia) * invOrientation;

				Vector3 angAccel = rigidBody->InverseInertiaTensor * rigidBody->Torque;
				rigidBody->AngularVelocity += angAccel * dt;

				transform->Rotation += Quaternion(0.0f, rigidBody->AngularVelocity * dt * 0.5f) * q;
				transform->Rotation.normalize();

				rigidBody->AngularVelocity *= frameDamping;
			});
	}
};