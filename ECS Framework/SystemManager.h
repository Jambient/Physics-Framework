#pragma once
#include "System.h"
#include <unordered_map>
#include <memory>
#include <cassert>

class SystemManager
{
public:
	void RegisterSystem(std::unique_ptr<System> system)
	{
		m_systems.push_back(std::move(system));
	}

	void Update(ECSScene& scene, float dt)
	{
		for (const auto& system : m_systems)
		{
			system->Update(scene, dt);
		}
	}

private:
	std::vector<std::unique_ptr<System>> m_systems;
};