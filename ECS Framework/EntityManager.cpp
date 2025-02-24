#include "EntityManager.h"
#include <cassert>

EntityManager::EntityManager()
{
	m_livingEntityCount = 0;

	for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
	{
		m_availableEntities.push(entity);
	}
}

Entity EntityManager::CreateEntity()
{
	assert(m_livingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

	Entity id = m_availableEntities.front();
	m_availableEntities.pop();
	++m_livingEntityCount;

	return id;
}

void EntityManager::DestroyEntity(Entity entity)
{
	assert(entity < MAX_ENTITIES && "Entity out of range.");

	// Invalidate the destroyed entity's signature
	m_signatures[entity].reset();

	// Put the destroyed ID at the back of the queue
	m_availableEntities.push(entity);
	--m_livingEntityCount;

}

void EntityManager::SetSignature(Entity entity, Signature signature)
{
	assert(entity < MAX_ENTITIES && "Entity out of range.");

	// Put this entity's signature into the array
	m_signatures[entity] = signature;
}

Signature EntityManager::GetSignature(Entity entity)
{
	assert(entity < MAX_ENTITIES && "Entity out of range.");

	// Get this entity's signature from the array
	return m_signatures[entity];
}

uint32_t EntityManager::GetEntityCount()
{
	return m_livingEntityCount;
}

bool EntityManager::DoesEntityExist(Entity entity)
{
	return m_signatures[entity].any();
}

bool EntityManager::HasComponent(Entity entity, ComponentType componentType)
{
	return m_signatures[entity].test(componentType);
}
