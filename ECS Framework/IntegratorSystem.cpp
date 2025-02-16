#include "IntegratorSystem.h"

const float DAMPING_FACTOR = 0.99f;
void IntegratorSystem::Update(ECSScene& scene, float dt)
{
	assert(dt > 0.0);

	float frameDamping = std::powf(DAMPING_FACTOR, dt);

	// integrate linear movement
	scene.ForEach<Particle, Transform>([dt, frameDamping](Entity entity, Particle* particle, Transform* transform)
		{
			// dont integrate things with infinite mass
			if (particle->inverseMass <= 0.0f) return;

			Vector3 newAcceleration = particle->force * particle->inverseMass;
			newAcceleration += Vector3::Down * 9.8f;

			// velocity verlet integration
			transform->position += particle->linearVelocity * dt + particle->lastLinearAcceleration * 0.5f * dt * dt;

			particle->linearVelocity += (particle->lastLinearAcceleration + newAcceleration) * 0.5f * dt;
			particle->linearVelocity *= frameDamping;

			particle->lastLinearAcceleration = newAcceleration;
			particle->force = Vector3::Zero;
		});

	// integrate angular movement
	scene.ForEach<Particle, RigidBody, Transform>([dt, frameDamping](Entity entity, Particle* particle, RigidBody* rigidBody, Transform* transform)
		{
			// dont integrate things with infinite mass
			if (particle->inverseMass <= 0.0f) return;

			// update inertia tensor
			Quaternion q = transform->rotation;
			Matrix3 invOrientation = q.conjugate().toMatrix3();
			Matrix3 orientation = q.toMatrix3();

			rigidBody->inverseInertiaTensor = orientation * Matrix3(rigidBody->inverseInertia) * invOrientation;

			Vector3 angAccel = rigidBody->inverseInertiaTensor * rigidBody->torque;
			rigidBody->angularVelocity += angAccel * dt;

			transform->rotation += Quaternion(0.0f, rigidBody->angularVelocity * dt * 0.5f) * q;
			transform->rotation.normalize();

			rigidBody->angularVelocity *= frameDamping;
		});
}
