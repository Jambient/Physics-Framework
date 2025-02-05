#pragma once
#include "Definitions.h"
#include <set>

class ECSScene;

class System
{
public:
	std::set<Entity> m_entities;
	ECSScene* m_scene = nullptr;
};