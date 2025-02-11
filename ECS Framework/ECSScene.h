// Scene class for ECS that encapsulates the engine and
// provides easier access to the ECS managers.

#pragma once
#ifndef ECS_SCENE_H_
#define ECS_SCENE_H_
#include <cmath>
#include <cmath>

#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"

/**
 * @class ECSScene
 * @brief Scene that encapsulates ECS engine.
 */
class ECSScene
{
public:

	/**
	 * @brief Initialises the scene by creating instances of the engine managers.
	 */
	void Init()
	{
		m_componentManager = std::make_shared<ComponentManager>();
		m_entityManager = std::make_shared<EntityManager>();
		m_systemManager = std::make_shared<SystemManager>();
	}

	// ENTITY METHODS

	/**
	 * @brief Creates a new entity in the scene.
	 * @return The new entities identifier.
	 */
	Entity CreateEntity()
	{
		return m_entityManager->CreateEntity();
	}

	/**
	 * @brief Removes the given entity and all of its components from the scene.
	 * @param entity The identifier of the entity to remove.
	 */
	void DestroyEntity(Entity entity)
	{
		m_entityManager->DestroyEntity(entity);
		m_componentManager->EntityDestroyed(entity);
		m_systemManager->EntityDestroyed(entity);
	}

	/**
	 * @brief Checks if a given entity exists in this scene.
	 * @param entity The identifier of the entity.
	 * @return True if the entity exists, false otherwise.
	 */
	bool DoesEntityExist(Entity entity)
	{
		return m_entityManager->DoesEntityExist(entity);
	}

	/**
	 * @brief Get the number of entities currently in this scene.
	 */
	uint32_t GetEntityCount()
	{
		return m_entityManager->GetEntityCount();
	}

	/**
	 * @brief Check if the maximum amount of entities has been reached defined by MAX_ENTITIES
	 * @return True if the current entity count is at the cap, false otherwise.
	 */
	bool ReachedEntityCap()
	{
		return m_entityManager->GetEntityCount() == MAX_ENTITIES;
	}

	/**
	 * @brief Checks if a given entity has a given component.
	 * @tparam T The component type.
	 * @param entity The identifier of the entity.
	 * @return True if the entity contains the component, false otherwise.
	 */
	template <typename T>
	bool HasComponent(Entity entity)
	{
		return m_entityManager->HasComponent(entity, GetComponentType<T>());
	}

	/**
	 * @brief Iterate over entities in the scene given a component set filter.
	 * @tparam ...Components The components required in an entity to be included in the loop.
	 * @param callback A callback method that contains the parameters: Entity, Components...
	 */
	template<typename... Components, typename Callback>
	void ForEach(Callback&& callback)
	{
		Signature signature = BuildSignature<Components...>();

		for (auto& archetype : m_componentManager->GetArchetypes(signature))
		{
			archetype->ForEach<Components...>(callback);
		}
	}

	// COMPONENT METHODS

	/**
	 * @brief Registers a new component type into the engine.
	 * @tparam T The type of the component.
	 */
	template <typename T>
	void RegisterComponent()
	{
		m_componentManager->RegisterComponent<T>();
	}

	/**
	 * @brief Adds a new component to an entity.
	 * @param entity The identifier of the entity to add the component to.
	 * @param component An instance of the component to add to the entity. The component must be of a type previously registered in the engine.
	 */
	template <typename T>
	void AddComponent(Entity entity, T component)
	{
		Signature oldSignature = m_entityManager->GetSignature(entity);

		assert(!oldSignature.test(m_componentManager->GetComponentType<T>()) && "Component already added to entity");

		Signature newSignature = m_componentManager->AddComponent<T>(entity, oldSignature, component);
		m_entityManager->SetSignature(entity, newSignature);

		m_systemManager->EntitySignatureChanged(entity, newSignature);
	}

	/**
	 * @brief Removes a component from an entity.
	 * @tparam T The type of the component.
	 * @param entity The identifier of the entity to remove the component from.
	 */
	template <typename T>
	void RemoveComponent(Entity entity)
	{
		Signature oldSignature = m_entityManager->GetSignature(entity);
		assert(oldSignature.test(m_componentManager->GetComponentType<T>()) && "Component does not exist on this entity");

		Signature newSignature = m_componentManager->RemoveComponent<T>(entity, oldSignature);
		m_entityManager->SetSignature(entity, newSignature);

		m_systemManager->EntitySignatureChanged(entity, newSignature);
	}

	/**
	 * @brief Get a component from an entity.
	 * @tparam T The type of the component.
	 * @param entity The identifier of the entity.
	 * @return A pointer of the component data.
	 */
	template <typename T>
	T* GetComponent(Entity entity)
	{
		return m_componentManager->GetComponent<T>(entity, m_entityManager->GetSignature(entity));
	}

	/**
	 * @brief Gets the engine registered type index of a component.
	 * @tparam T The type of the component.
	 * @return The ComponentType of the component.
	 */
	template <typename T>
	ComponentType GetComponentType()
	{
		return m_componentManager->GetComponentType<T>();
	}

	// SYSTEM METHODS

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

	/**
	 * @brief Build a signature given a typearray of component types.
	 * @tparam ...Components The types of the components.
	 * @return A signature which has true bits at the registered componented type indexes of the provided types.
	 */
	template <typename... Components>
	Signature BuildSignature()
	{
		static const Signature signature(sum((std::powf(2, (float)m_componentManager->GetComponentType<Components>()))...));
		return signature;
	}

	/**
	 * @brief Sums a variable amount of integers.
	 */
	template<typename... Args>
	static int sum(Args... args) {
		return (args + ...);
	}

	std::shared_ptr<ComponentManager> m_componentManager; // A pointer to the component manager.
	std::shared_ptr<EntityManager> m_entityManager; // A pointer to the entity manager.
	std::shared_ptr<SystemManager> m_systemManager; // A pointer to the system manager.
};

#endif // ECS_SCENE_H_