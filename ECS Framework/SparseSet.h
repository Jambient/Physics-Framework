//#pragma once
//#include "Definitions.h"
//#include <vector>
//#include "Components.h"
//#include <unordered_map>
//
//class ISparseSet
//{
//public:
//	virtual ~ISparseSet() = default;
//	virtual void EntityDestroyed(Entity entity) = 0;
//};
//
//template <typename T>
//class SparseSet : public ISparseSet
//{
//public:
//	SparseSet() : m_componentCount(0) {};
//
//	void AddComponent(Entity entity, const T& component)
//	{
//		assert(m_entityToDenseIndex.find(entity) == m_entityToDenseIndex.end() && "Entity already has this component.");
//
//		m_entityToDenseIndex[entity] = m_componentCount;
//		m_denseIndexToEntity[m_componentCount] = entity;
//		m_denseArray.push_back(component);
//		m_componentCount++;
//	}
//
//	void RemoveComponent(Entity entity)
//	{
//		assert(m_entityToDenseIndex.find(entity) != m_entityToDenseIndex.end() && "Removing non-existent component.");
//
//		size_t removedEntityIndex = m_entityToDenseIndex[entity];
//		size_t lastEntityIndex = m_componentCount - 1;
//
//		m_denseArray[removedEntityIndex] = m_denseArray[lastEntityIndex];
//
//		// Update map to point to moved spot
//		Entity entityOfLastElement = m_denseIndexToEntity[lastEntityIndex];
//		m_entityToDenseIndex[entityOfLastElement] = removedEntityIndex;
//		m_denseIndexToEntity[removedEntityIndex] = entityOfLastElement;
//
//		m_entityToDenseIndex.erase(entity);
//		m_denseIndexToEntity.erase(lastEntityIndex);
//
//		m_componentCount--;
//	}
//
//	T& GetComponent(Entity entity)
//	{
//		assert(m_entityToDenseIndex.find(entity) != m_entityToDenseIndex.end() && "Retrieving non-existent component.");
//
//		return m_denseArray[m_entityToDenseIndex[entity]];
//	}
//
//	std::vector<T>& GetAllComponents()
//	{
//		return m_denseArray;
//	}
//
//	void EntityDestroyed(Entity entity) override
//	{
//		if (m_entityToDenseIndex.find(entity) != m_entityToDenseIndex.end())
//		{
//			RemoveComponent(entity);
//		}
//	}
//
//private:
//	std::vector<T> m_denseArray;
//	std::unordered_map<Entity, size_t> m_entityToDenseIndex;
//	std::unordered_map<size_t, Entity> m_denseIndexToEntity;
//	size_t m_componentCount;
//};

#pragma once
#include "Definitions.h"
#include <vector>
#include <cassert>

class ISparseSet
{
public:
    virtual ~ISparseSet() = default;
    virtual void EntityDestroyed(Entity entity) = 0;
};

template <typename T>
class SparseSet : public ISparseSet
{
public:
    SparseSet() : m_entityToDenseIndex(MAX_ENTITIES, UINT32_MAX) {}

    void AddComponent(Entity entity, const T& component)
    {
        assert(m_entityToDenseIndex[entity] == UINT32_MAX && "Entity already has this component.");

        // Map entity to dense array index
        m_entityToDenseIndex[entity] = m_denseArray.size();
        m_denseArray.push_back(component);
        m_denseIndexToEntity.push_back(entity);
    }

    void RemoveComponent(Entity entity)
    {
        assert(m_entityToDenseIndex[entity] != UINT32_MAX && "Removing non-existent component.");

        // Get indices
        size_t removedEntityIndex = m_entityToDenseIndex[entity];
        size_t lastEntityIndex = m_denseArray.size() - 1;

        // Replace removed element with the last one
        m_denseArray[removedEntityIndex] = m_denseArray[lastEntityIndex];
        Entity lastEntity = m_denseIndexToEntity[lastEntityIndex];

        // Update mappings
        m_entityToDenseIndex[lastEntity] = removedEntityIndex;
        m_denseIndexToEntity[removedEntityIndex] = lastEntity;

        // Erase removed entity
        m_entityToDenseIndex[entity] = UINT32_MAX;
        m_denseArray.pop_back();
        m_denseIndexToEntity.pop_back();
    }

    T& GetComponent(Entity entity)
    {
        assert(m_entityToDenseIndex[entity] != UINT32_MAX && "Retrieving non-existent component.");
        return m_denseArray[m_entityToDenseIndex[entity]];
    }

    std::vector<T>& GetAllComponents()
    {
        return m_denseArray;
    }

    void EntityDestroyed(Entity entity) override
    {
        if (m_entityToDenseIndex[entity] != UINT32_MAX)
        {
            RemoveComponent(entity);
        }
    }

private:
    std::vector<T> m_denseArray;                 // Contiguous array of components
    std::vector<Entity> m_denseIndexToEntity;    // Maps dense index -> entity
    std::vector<size_t> m_entityToDenseIndex;    // Maps entity -> dense index
};