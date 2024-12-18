#pragma once
#include "Definitions.h"
#include <vector>
#include <cstddef> // For size_t
#include <cstdlib> // For malloc, free
#include <cassert>
#include <cstring> // For memcpy
#include <unordered_map>
#include <array>
#include <tuple>

struct ComponentData
{
	ComponentType type;
	void* data;
};

// type-erased buffer for component data
struct Column
{
	std::vector<char> m_elements; // buffer with component data
	size_t m_elementSize; // size of a single element

	Column() {};

	Column(size_t elementSize) : m_elementSize(elementSize) {}

	void AddComponent(const void* data)
	{
		assert(data != nullptr && "Component data cannot be null!");

		// cast data into array of bytes
		const char* rawData = static_cast<const char*>(data);

		// insert it into the buffer
		m_elements.insert(m_elements.end(), rawData, rawData + m_elementSize);
	}

	void RemoveComponent(size_t index)
	{
		assert(index < GetCount() && "Index out of bounds!");

		// Get iterators to the element to remove
		auto startIt = m_elements.begin() + index * m_elementSize;
		auto endIt = startIt + m_elementSize;

		// Replace the removed element with the last element in the vector
		if (index != GetCount() - 1)
		{
			auto lastElementStart = m_elements.end() - m_elementSize;
			std::copy(lastElementStart, m_elements.end(), startIt);
		}

		// Erase the last element
		m_elements.erase(m_elements.end() - m_elementSize, m_elements.end());
	}

	void* GetComponent(size_t index) const
	{
		assert(index < GetCount() && "Index out of bounds!");

		return const_cast<char*>(m_elements.data() + index * m_elementSize);
	}

	size_t GetCount() const
	{
		return m_elements.size() / m_elementSize;
	}
};

class Archetype
{
public:
	Archetype(Signature signature, std::array<size_t, MAX_COMPONENT_TYPES>& componentSizes) : m_entityToDenseIndex(MAX_ENTITIES, UINT32_MAX)
	{
		m_signature = signature;
		m_entityCount = 0;
		
		for (size_t i = 0; i < MAX_COMPONENT_TYPES; i++)
		{
			if (signature[i])
			{
				// need to change this to properly get component size
				//m_componentColumns.emplace((ComponentType)i, Column(componentSizes[i]));
				m_componentColumns[(ComponentType)i] = Column(componentSizes[i]);
			}
		}
	}

	template<typename... Components, typename Callback>
	void ForEach(Signature signature, Callback&& callback)
	{
		// Filter the component columns to process only those specified by the signature
		std::vector<std::pair<ComponentType, Column*>> activeColumns;

		for (auto& [componentType, column] : m_componentColumns) {
			if (signature[componentType]) {
				activeColumns.emplace_back(componentType, &column);
			}
		}

		// Prepare a tuple of pointers for each entity
		for (size_t i = 0; i < m_entityCount; ++i) {
			// Create a tuple of pointers for the current entity's components
			auto componentPointers = std::make_tuple(static_cast<Components*>(activeColumns[0].second->GetComponent(i))...);

			// Use std::apply to call the callback with unpacked components
			std::apply(callback, componentPointers);
		}
	}

	Signature GetSignature() const { return m_signature; }

	// Method to add an entity with its component data
	void AddEntity(Entity entity, const std::vector<ComponentData>& components) {

		m_entityToDenseIndex[entity] = m_entityCount++;
		m_denseIndexToEntity.push_back(entity);

		for (const ComponentData& component : components) {
			assert(m_componentColumns.find(component.type) != m_componentColumns.end() && "Component type not present in this archetype!");

			m_componentColumns[component.type].AddComponent(component.data);
		}
	}

	std::vector<ComponentData> GetComponentData(Entity entity)
	{
		std::vector<ComponentData> componentData;

		for (auto& pair : m_componentColumns) {
			const ComponentData data = { pair.first, pair.second.GetComponent(entity) };
			componentData.push_back(data);
		}

		return componentData;
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