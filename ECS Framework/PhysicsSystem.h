#pragma once
#include "Components.h"
#include "Scene.h"

class PhysicsSystem : public System
{
public:
	void Update(float dt)
	{
		std::vector<Transform>& transformComponents = m_scene->GetAllComponents<Transform>();
		std::vector<RigidBody>& rigidBodyComponents = m_scene->GetAllComponents<RigidBody>();
		const std::vector<Gravity>& gravityComponents = m_scene->GetAllComponents<Gravity>();

		for (size_t i = 0; i < m_entities.size(); i++)
		{
			transformComponents[i].Position += rigidBodyComponents[i].Velocity * dt;
			rigidBodyComponents[i].Velocity += gravityComponents[i].Force * dt;

			//// update position with velocity
			//XMStoreFloat3(&transformComponents[i].Position, XMVectorAdd(XMVectorScale(XMLoadFloat3(&rigidBodyComponents[i].Velocity), dt), XMLoadFloat3(&transformComponents[i].Position)));

			//// update rigidbody velocity using gravity
			//XMStoreFloat3(&rigidBodyComponents[i].Velocity, XMVectorAdd(XMVectorScale(XMLoadFloat3(&gravityComponents[i].Force), dt), XMLoadFloat3(&rigidBodyComponents[i].Velocity)));
		}
	}
};