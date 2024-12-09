#pragma once
#include "SparseSet.h"
#include "Definitions.h"
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
	}

	template <typename T>
	ComponentType GetComponentType()
	{
		return GetTypeIndex<T>();

		//assert(m_componentTypes.find(typeHash) != m_componentTypes.end() && "Component not registered before use.");

		//return m_componentTypes[typeHash];
	}

	template <typename T>
	void AddComponent(Entity entity, T component)
	{
		GetComponentArray<T>()->AddComponent(entity, component);
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
	std::vector<std::shared_ptr<ISparseSet>> m_componentSets;
	//std::unordered_map<std::size_t, ComponentType> m_componentTypes;
	ComponentType m_nextComponentType = 0;

	template <typename T>
	SparseSet<T>* GetComponentArray()
	{
		std::size_t typeIndex = GetTypeIndex<T>();

		//assert(m_componentTypes.find(typeHash) != m_componentTypes.end() && "Component not registered before use.");

		return static_cast<SparseSet<T>*>(m_componentSets[typeIndex].get());
	}

	template <typename T>
	std::size_t ComponentTypeHash()
	{
		return std::type_index(typeid(T)).hash_code();
	}

	template <typename T>
	std::size_t GetTypeIndex()
	{
		// Directly use the type index as the unique ID for the component
		static const std::size_t typeIndex = m_nextComponentType++;
		return typeIndex;
	}
};

//class ComponentManager
//{
//public:
//	ComponentManager()
//	{
//		m_componentSets.resize(MAX_COMPONENT_TYPES);
//		//m_componentTypes.resize(MAX_COMPONENT_TYPES, INVALID_COMPONENT_TYPE);
//	}
//
//	template <typename T>
//	void RegisterComponent()
//	{
//		std::size_t typeHash = ComponentTypeHash<T>();
//		std::size_t test = GetTypeIndex<T>();
//
//		assert(m_componentTypes.find(typeHash) == m_componentTypes.end() && "Registering component type more than once.");
//
//		m_componentTypes[typeHash] = m_nextComponentType;
//		m_componentSets[m_nextComponentType] = std::make_shared<SparseSet<T>>();
//
//		m_nextComponentType++;
//	}
//
//	template <typename T>
//	ComponentType GetComponentType()
//	{
//		std::size_t typeHash = ComponentTypeHash<T>();
//
//		assert(m_componentTypes.find(typeHash) != m_componentTypes.end() && "Component not registered before use.");
//
//		return m_componentTypes[typeHash];
//	}
//
//	template <typename T>
//	void AddComponent(Entity entity, T component)
//	{
//		GetComponentArray<T>()->AddComponent(entity, component);
//	}
//
//	template <typename T>
//	void RemoveComponent(Entity entity)
//	{
//		GetComponentArray<T>()->RemoveComponent(entity);
//	}
//
//	template <typename T>
//	T& GetComponent(Entity entity)
//	{
//		return GetComponentArray<T>()->GetComponent(entity);
//	}
//
//	template <typename T>
//	std::vector<T>& GetAllComponents()
//	{
//		return GetComponentArray<T>()->GetAllComponents();
//	}
//
//	void EntityDestroyed(Entity entity)
//	{
//		for (auto& componentSet : m_componentSets)
//		{
//			if (componentSet)
//			{
//				componentSet->EntityDestroyed(entity);
//			}
//		}
//	}
//
//private:
//	std::vector<std::shared_ptr<ISparseSet>> m_componentSets;
//	std::unordered_map<std::size_t, ComponentType> m_componentTypes;
//    ComponentType m_nextComponentType = 0;
//
//    template <typename T>
//    SparseSet<T>* GetComponentArray()
//    {
//        std::size_t typeHash = ComponentTypeHash<T>();
//
//        assert(m_componentTypes.find(typeHash) != m_componentTypes.end() && "Component not registered before use.");
//
//        return static_cast<SparseSet<T>*>(m_componentSets[m_componentTypes[typeHash]].get());
//    }
//
//	template <typename T>
//	std::size_t ComponentTypeHash()
//	{
//		return std::type_index(typeid(T)).hash_code();
//	}
//
//	template <typename T>
//	std::size_t GetTypeIndex()
//	{
//		// Directly use the type index as the unique ID for the component
//		static const std::size_t typeIndex = m_nextComponentType++;
//		return typeIndex;
//	}
//};
