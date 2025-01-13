#pragma once
#include "Definitions.h"
#include <queue>
#include <array>

class EntityManager
{
public:
	EntityManager();

	Entity CreateEntity();

	void DestroyEntity(Entity entity);

	void SetSignature(Entity entity, Signature signature);
	Signature GetSignature(Entity entity);

	uint32_t GetEntityCount();

	bool DoesEntityExist(Entity entity);

private:
	std::queue<Entity> m_availableEntities;
	std::array<Signature, MAX_ENTITIES> m_signatures;
	uint32_t m_livingEntityCount;

};

