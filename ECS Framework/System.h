#pragma once
#include "Definitions.h"
#include <set>

class Scene;

class System
{
public:
	std::set<Entity> m_entities;
	Scene* m_scene;
};