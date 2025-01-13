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
#include "TypeIDGenerator.h"
#include <iostream>
#include <utility>

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

	Column() : m_elementSize(0) {};

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

	template<typename T>
	T* GetComponentData(size_t index)
	{
		assert(index < GetCount() && "Index out of bounds!");

		char* data = m_elements.data() + index * m_elementSize;
		return (T*)data;
	}

	size_t GetCount() const
	{
		return m_elements.size() / m_elementSize;
	}
};

template <typename...Ts>
struct foo {
	static const size_t N = sizeof...(Ts);
	template <size_t ... I>
	static auto helper(std::integer_sequence<size_t, I...>) {
		return std::tuple<std::array<Ts, I>...>{};
	}
	using type = decltype(helper(std::make_integer_sequence<size_t, N>{}));
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
	void ForEach(Callback&& callback)
	{
		if (m_entityCount == 0) { return; }

		constexpr size_t componentCount = sizeof...(Components);

		// create an array of the used component columns in this archetype
		std::array<Column*, componentCount> orderedColumns = { (&m_componentColumns[(ComponentType)TypeIndexGenerator::GetTypeIndex<Components>()])... };

		// for each entity gets its component data and execute the callback
		for (size_t i = 0; i < m_entityCount; i++) {
			int index = componentCount;
			auto componentData = std::make_tuple(m_denseIndexToEntity[i], (orderedColumns[--index]->GetComponentData<Components>(i))...);

			std::apply(callback, componentData);
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

	template<typename T>
	T* GetComponent(Entity entity)
	{
		return m_componentColumns[(ComponentType)TypeIndexGenerator::GetTypeIndex<T>()].GetComponentData<T>(m_entityToDenseIndex[entity]);
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
		if (m_entityToDenseIndex[entity] == INVALID_ENTITY) return;

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

		m_entityCount--;
	}

private:

	Signature m_signature;
	Entity m_entityCount;
	std::unordered_map<ComponentType, Column> m_componentColumns;
	std::vector<Entity> m_denseIndexToEntity;    // Maps dense index -> entity
	std::vector<size_t> m_entityToDenseIndex;    // Maps entity -> dense index
};