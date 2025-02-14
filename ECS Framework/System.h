#pragma once
#include "Definitions.h"
#include <set>

class ECSScene;

class System
{
public:
	virtual void Update(ECSScene& scene, float dt) = 0;
};