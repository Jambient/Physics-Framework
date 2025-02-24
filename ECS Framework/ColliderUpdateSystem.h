#pragma once
#include "Components.h"
#include "ECSScene.h"

class ColliderUpdateSystem : public System
{
public:
	void Update(ECSScene& scene, float dt) final override;
};