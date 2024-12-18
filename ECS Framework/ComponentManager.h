#pragma once
#include "SparseSet.h"
#include "Definitions.h"
#include "Archetype.h"
#include <cassert>
#include <vector>
#include <memory>
#include <typeindex>
#include <unordered_map>

class ComponentManager
{
public:
	ComponentManager()
	{
		m_componentSets.resize(MAX_COMPONENT_TYPES);
	}

	template <typename T>
	void RegisterComponent()
	{
		std::size_t typeIndex = GetTypeIndex<T>();

		//assert(m_componentTypes.find(typeHash) == m_componentTypes.end() && "Registering component type more than once.");

		m_componentSets[typeIndex] = std::make_shared<SparseSet<T>>();
		m_componentSizes[typeIndex] = sizeof(T);
	}

	template <typename T>
	ComponentType GetComponentType()
	{
		return GetTypeIndex<T>();

		//assert(m_componentTypes.find(typeHash) != m_componentTypes.end() && "Component not registered before use.");

		//return m_componentTypes[typeHash];
	}

	template <typename T>
	Signature AddComponent(Entity entity, Signature signature, T component)
	{
		Signature oldSignature = signature;

		// check if the entity has had a component before
		//if (signature.any())
		//{
		//	// get its previous archetype so data can be moved over
		//	std::shared_ptr<Archetype> oldArchetype = m_archetypes[signature];

		//	auto oldData = oldArchetype->GetComponentData(entity);
		//}

		// update signature with new component
		signature.set(GetTypeIndex<T>());
		
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
			newArchetype = std::make_shared<Archetype>(signature);
		}

		return signature;
	}

	template <typename T>
	void RemoveComponent(Entity entity)
	{
		GetComponentArray<T>()->RemoveComponent(entity);
	}

	template <typename T>
	T& GetComponent(Entity entity)
	{
		return GetComponentArray<T>()->GetComponent(entity);
	}

	template <typename T>
	std::vector<T>& GetAllComponents()
	{
		return GetComponentArray<T>()->GetAllComponents();
	}

	void EntityDestroyed(Entity entity)
	{
		for (auto& componentSet : m_componentSets)
		{
			if (componentSet)
			{
				componentSet->EntityDestroyed(entity);
			}
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
		std::size_t typeIndex = GetTypeIndex<T>();

		//assert(m_componentTypes.find(typeHash) != m_componentTypes.end() && "Component not registered before use.");

		return static_cast<SparseSet<T>*>(m_componentSets[typeIndex].get());
	}

	template <typename T>
	std::size_t GetTypeIndex()
	{
		static const std::size_t typeIndex = m_nextComponentType++;
		return typeIndex;
	}
};