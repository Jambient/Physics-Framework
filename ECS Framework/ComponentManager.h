#pragma once
#ifndef COMPONENT_MANAGER_H_
#define COMPONENT_MANAGER_H_
#include <cassert>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <array>

#include "SparseSet.h"
#include "Definitions.h"
#include "Archetype.h"
#include "TypeIDGenerator.h"

/**
 * @class ComponentManager
 * @brief Manager class that manages all the components in the engine.
 */
class ComponentManager
{
public:
	/**
	 * @brief Default constructor
	 */
	ComponentManager()
	{
		m_componentSets.resize(MAX_COMPONENT_TYPES);
		m_componentSizes = { 0 };
	}

	/**
	 * @brief Registers a new component into the engine.
	 * @tparam T The type of the component.
	 */
	template <typename T>
	void RegisterComponent()
	{
		std::size_t typeIndex = GetTypeIndex<T>();

		m_componentSets[typeIndex] = std::make_shared<SparseSet<T>>();
		m_componentSizes[typeIndex] = sizeof(T);
	}

	/**
	 * @brief Gets the registered type index of the component type.
	 * @return The ComponentType index of the provided component.
	 */
	template <typename T>
	ComponentType GetComponentType()
	{
		return (ComponentType)GetTypeIndex<T>();
	}

	/**
	 * @brief Adds a component to an entity. It does this by copying the entities component data from its
	 * previous archetype (if applicable), adding on the new component data and inserting it into the new archetype.
	 * @param entity The identifier of the entity to add the component to.
	 * @param signature The current signature of the entity.
	 * @param component The data of the component being added.
	 * @return The new signature of the entity.
	 */
	template <typename T>
	Signature AddComponent(Entity entity, Signature signature, T component)
	{
		Signature oldSignature = signature;

		// update signature with new component
		ComponentType newComponentType = GetComponentType<T>();
		signature.set(newComponentType);
		
		// check if the new signatures archetype already exists
		// TODO: this check should be done using the "Archetype Graph (3/14)" suggested in the ECS #3 article
		std::shared_ptr<Archetype> newArchetype;
		if (m_archetypes.find(signature) != m_archetypes.end())
		{
			newArchetype = m_archetypes[signature];
		}
		else
		{
			// create a new archetype with this signature
			newArchetype = std::make_shared<Archetype>(signature, m_componentSizes);
			m_archetypes[signature] = newArchetype;
		}

		std::vector<ComponentData> componentData;

		// check if the entity had a component before
		// if they did, then their data needs to be transferred over
		if (oldSignature.any())
		{
			std::shared_ptr<Archetype> oldArchetype = m_archetypes[oldSignature];
			componentData = oldArchetype->GetComponentData(entity);
			oldArchetype->RemoveEntity(entity);
		}

		const ComponentData newComponentData = { newComponentType, &component };
		componentData.push_back(newComponentData);

		newArchetype->AddEntity(entity, componentData);

		return signature;
	}

	template <typename T>
	void RemoveComponent(Entity entity)
	{
		GetComponentArray<T>()->RemoveComponent(entity);
	}

	template <typename T>
	T* GetComponent(Entity entity, Signature signature)
	{
		std::shared_ptr<Archetype> archetype = m_archetypes[signature];
		return archetype->GetComponent<T>(entity);
	}

	template <typename T>
	std::vector<T>& GetAllComponents()
	{
		return GetComponentArray<T>()->GetAllComponents();
	}

	std::vector<std::shared_ptr<Archetype>> GetArchetypes(Signature signature)
	{
		std::vector<std::shared_ptr<Archetype>> archetypes;
		for (const auto& pair : m_archetypes)
		{
			if ((pair.first & signature) == signature)
			{
				archetypes.push_back(pair.second);
			}
		}

		return archetypes;
	}

	void EntityDestroyed(Entity entity)
	{
		for (const auto& pair : m_archetypes)
		{
			pair.second->RemoveEntity(entity);
		}
	}

private:
	std::array<size_t, MAX_COMPONENT_TYPES> m_componentSizes;
	std::vector<std::shared_ptr<ISparseSet>> m_componentSets;
	std::unordered_map<Signature, std::shared_ptr<Archetype>> m_archetypes;
	ComponentType m_nextComponentType = 0;

	template <typename T>
	SparseSet<T>* GetComponentArray()
	{
		size_t typeIndex = GetTypeIndex<T>();

		//assert(m_componentTypes.find(typeHash) != m_componentTypes.end() && "Component not registered before use.");

		return static_cast<SparseSet<T>*>(m_componentSets[typeIndex].get());
	}

	template <typename T>
	size_t GetTypeIndex()
	{
		return TypeIndexGenerator::GetTypeIndex<T>();
	}
};

#endif //COMPONENT_MANAGER_H_