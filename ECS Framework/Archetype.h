#pragma once
#include "Definitions.h"
#include <vector>
#include <cstddef> // For size_t
#include <cstdlib> // For malloc, free
#include <cassert>
#include <cstring> // For memcpy

// TODO: I never actually use the map lookup for passing component data in
// perhaps this could just be an array

// type-erased buffer for component data
struct Column
{
	void* m_elements;      // buffer with component data
	size_t m_elementSize;  // size of a single element
	size_t m_count;        // number of elements

	Column(size_t elementSize)
		: m_elements(nullptr), m_elementSize(elementSize), m_count(0) {}

	~Column()
	{
		if (m_elements != nullptr)
		{
			free(m_elements);
		}
	}

	void AddComponent(const void* data)
	{
		++m_count;

		m_elements = realloc(m_elements, m_count * m_elementSize);
		assert(m_elements && "Memory allocation failed!");

		// Copy the new element into the buffer
		memcpy(static_cast<char*>(m_elements) + (m_count - 1) * m_elementSize, data, m_elementSize);
	}

	void RemoveComponent(size_t index)
	{
		assert(index < m_count && "Index out of bounds!");

		// Copy the last element into the specified index
		void* lastElement = static_cast<char*>(m_elements) + (m_count - 1) * m_elementSize;
		void* targetElement = static_cast<char*>(m_elements) + index * m_elementSize;
		memcpy(targetElement, lastElement, m_elementSize);

		// Decrease the count
		--m_count;

		// Reallocate the buffer to shrink it
		if (m_count > 0)
		{
			m_elements = realloc(m_elements, m_count * m_elementSize);
			assert(m_elements && "Memory allocation failed!");
		}
		else
		{
			// Free the buffer if no elements remain
			free(m_elements);
			m_elements = nullptr;
		}
	}

	void* GetComponent(size_t index) const
	{
		assert(index < m_count && "Index out of bounds!");

		return static_cast<char*>(m_elements) + index * m_elementSize;
	}
};

class Archetype
{
public:
	Archetype(Signature signature, size_t* componentSizes)
	{
		m_signature = signature;
		
		for (size_t i = 0; i < MAX_COMPONENT_TYPES; i++)
		{
			if (signature[i])
			{
				// need to change this to properly get component size
				m_componentColumns.emplace((ComponentType)i, Column(componentSizes[i]));
			}
		}
	}

	// Method to add an entity with its component data
	void AddEntity(Entity entity, const std::unordered_map<ComponentType, void*>& components) {

		m_entityToDenseIndex[entity] = m_entityCount++;
		m_denseIndexToEntity.push_back(entity);

		for (const auto& pair : components) {
			const auto& type = pair.first;  // Component type
			const auto& data = pair.second; // Pointer to component data

			assert(m_componentColumns.find(type) != m_componentColumns.end() && "Component type not present in this archetype!");

			m_componentColumns[type].AddComponent(data);
		}
	}

	std::unordered_map<ComponentType, void*> GetComponentData(Entity entity)
	{
		std::unordered_map<ComponentType, void*> componentData;

		for (auto& pair : m_componentColumns) {
			m_componentColumns.emplace(pair.first, pair.second.GetComponent(entity));
		}
	}

	void RemoveEntity(Entity entity)
	{
		assert(m_entityToDenseIndex[entity] != UINT32_MAX && "Removing non-existent component.");

		// Get indices
		size_t removedEntityIndex = m_entityToDenseIndex[entity];
		size_t lastEntityIndex = (size_t)m_entityCount - 1;

		// remove the entities components from all columns
		for (auto& pair : m_componentColumns) {
			pair.second.RemoveComponent(removedEntityIndex);
		}

		Entity lastEntity = m_denseIndexToEntity[lastEntityIndex];

		// Update mappings
		m_entityToDenseIndex[lastEntity] = removedEntityIndex;
		m_denseIndexToEntity[removedEntityIndex] = lastEntity;

		// Erase removed entity
		m_entityToDenseIndex[entity] = UINT32_MAX;
		m_denseIndexToEntity.pop_back();
	}

private:
	Signature m_signature;
	Entity m_entityCount;
	std::unordered_map<ComponentType, Column> m_componentColumns;
	std::vector<Entity> m_denseIndexToEntity;    // Maps dense index -> entity
	std::vector<size_t> m_entityToDenseIndex;    // Maps entity -> dense index
};