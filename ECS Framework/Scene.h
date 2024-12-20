#pragma once
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"
#include <cmath>

struct WorldView
{
	Signature signature;
	std::vector<std::shared_ptr<Archetype>> archetypes;
};

class Scene
{
public:
	void Init()
	{
		m_componentManager = std::make_shared<ComponentManager>();
		m_entityManager = std::make_shared<EntityManager>();
		m_systemManager = std::make_shared<SystemManager>();
	}

	///////////// ENTITY METHODS
	
	Entity CreateEntity()
	{
		return m_entityManager->CreateEntity();
	}

	void DestroyEntity(Entity entity)
	{
		m_entityManager->DestroyEntity(entity);
		m_componentManager->EntityDestroyed(entity);
		m_systemManager->EntityDestroyed(entity);
	}

	///////////// VIEW METHODS
	template <typename... Components>
	WorldView CreateWorldView()
	{
		WorldView worldView;
		Signature viewSignature;
		(SetComponentBit<Components>(viewSignature), ...);

		worldView.signature = viewSignature;
		worldView.archetypes = m_componentManager->GetArchetypes(viewSignature);

		return worldView;
	}

	template<typename... Components, typename Callback>
	void ForEach(Callback&& callback)
	{
		Signature signature = BuildSignature<Components...>();

		for (auto& archetype : m_componentManager->GetArchetypes(signature))
		{
			archetype->ForEach<Components...>(callback);
		}
	}

	///////////// COMPONENT METHODS

	template <typename T>
	void RegisterComponent()
	{
		m_componentManager->RegisterComponent<T>();
	}

	template <typename T>
	void AddComponent(Entity entity, T component)
	{
		Signature newSignature = m_componentManager->AddComponent<T>(entity, m_entityManager->GetSignature(entity), component);
		m_entityManager->SetSignature(entity, newSignature);

		m_systemManager->EntitySignatureChanged(entity, newSignature);
	}

	template <typename T>
	void RemoveComponent(Entity entity)
	{
		m_componentManager->RemoveComponent<T>(entity);

		Signature signature = m_entityManager->GetSignature(entity);
		signature.set(m_componentManager->GetComponentType<T>(), false);
		m_entityManager->SetSignature(entity, signature);

		m_systemManager->EntitySignatureChanged(entity, signature);
	}

	template <typename T>
	T& GetComponent(Entity entity)
	{
		return m_componentManager->GetComponent<T>(entity);
	}

	template <typename T>
	std::vector<T>& GetAllComponents()
	{
		return m_componentManager->GetAllComponents<T>();
	}

	template <typename T>
	ComponentType GetComponentType()
	{
		return m_componentManager->GetComponentType<T>();
	}

	///////////// SYSTEM METHODS

	template <typename T>
	std::shared_ptr<T> RegisterSystem()
	{
		return m_systemManager->RegisterSystem<T>();
	}

	template <typename T>
	void SetSystemSignature(Signature signature)
	{
		m_systemManager->SetSignature<T>(signature);
	}

private:
	template <typename... Components>
	Signature BuildSignature()
	{
		static const Signature signature( sum( (std::powf(2, (float)m_componentManager->GetComponentType<Components>()))... ) );
		return signature;
	}

	template<typename... Args>
	static int sum(Args... args) {
		return (args + ...);
	}

	template <typename T>
	void SetComponentBit(Signature& signature) {
		signature.set(m_componentManager->GetComponentType<T>());
	}

	std::shared_ptr<ComponentManager> m_componentManager;
	std::shared_ptr<EntityManager> m_entityManager;
	std::shared_ptr<SystemManager> m_systemManager;
};