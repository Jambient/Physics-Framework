#pragma once
#include "Components.h"
#include "Scene.h"

class PhysicsSystem : public System
{
public:
	void Update(float dt)
	{
		//m_scene->ForEach<Gravity, Transform, RigidBody>([dt](Entity entity, Gravity* gravity, Transform* transform, RigidBody* rigidbody) {
		//	//transform->Position += rigidbody->Velocity * dt;
		//	//rigidbody->Velocity += gravity->Force * dt;
		//	transform->Position += Vector3::Down * 10 * dt;
		//});

		// integrate particles
		m_scene->ForEach<Particle, Transform>([dt](Entity entity, Particle* particle, Transform* transform)
			{
				// dont integrate things with infinite mass
				if (particle->inverseMass <= 0.0f) return;

				assert(dt > 0.0);

				// update linear position
				transform->Position += particle->Velocity * dt;

				// work out the accelerate from the force.
				Vector3 resultingAcc = particle->Acceleration;

				// update linear velocity from the acceleration
				particle->Velocity += resultingAcc * dt;

				// impose drag
				particle->Velocity *= std::powf(particle->damping, dt);
			});

		//std::vector<Transform>& transformComponents = m_scene->GetAllComponents<Transform>();
		//std::vector<RigidBody>& rigidBodyComponents = m_scene->GetAllComponents<RigidBody>();
		//const std::vector<Gravity>& gravityComponents = m_scene->GetAllComponents<Gravity>();

		//for (size_t i = 0; i < m_entities.size(); i++)
		//{
		//	transformComponents[i].Position += rigidBodyComponents[i].Velocity * dt;
		//	rigidBodyComponents[i].Velocity += gravityComponents[i].Force * dt;

		//	//// update position with velocity
		//	//XMStoreFloat3(&transformComponents[i].Position, XMVectorAdd(XMVectorScale(XMLoadFloat3(&rigidBodyComponents[i].Velocity), dt), XMLoadFloat3(&transformComponents[i].Position)));

		//	//// update rigidbody velocity using gravity
		//	//XMStoreFloat3(&rigidBodyComponents[i].Velocity, XMVectorAdd(XMVectorScale(XMLoadFloat3(&gravityComponents[i].Force), dt), XMLoadFloat3(&rigidBodyComponents[i].Velocity)));
		//}
	}
};