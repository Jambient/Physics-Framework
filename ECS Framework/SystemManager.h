#pragma once
#include "System.h"
#include <unordered_map>
#include <memory>
#include <cassert>

class SystemManager
{
public:
	template <typename T>
	std::shared_ptr<T> RegisterSystem()
	{
		const char* typeName = typeid(T).name();

		assert(m_systems.find(typeName) == m_systems.end() && "Registering system more than once.");

		std::shared_ptr<T> system = std::make_shared<T>();
		m_systems.insert({ typeName, system });

		return system;
	}

	template <typename T>
	void SetSignature(Signature signature)
	{
		const char* typeName = typeid(T).name();

		assert(m_systems.find(typeName) != m_systems.end() && "System used before registered.");

		m_signatures.insert({ typeName, signature });
	}

	void EntityDestroyed(Entity entity)
	{
		for (auto const& pair : m_systems)
		{
			std::shared_ptr<System> system = pair.second;
			system->m_entities.erase(entity);
		}
	}

	void EntitySignatureChanged(Entity entity, Signature newEntitySignature)
	{
		for (auto const& pair : m_systems)
		{
			const char* type = pair.first;
			std::shared_ptr<System> system = pair.second;
			Signature systemSignature = m_signatures[type];

			if ((newEntitySignature & systemSignature) == systemSignature)
			{
				system->m_entities.insert(entity);
			}
			else
			{
				system->m_entities.erase(entity);
			}
		}
	}

private:
	std::unordered_map<const char*, Signature> m_signatures;
	std::unordered_map<const char*, std::shared_ptr<System>> m_systems;
};