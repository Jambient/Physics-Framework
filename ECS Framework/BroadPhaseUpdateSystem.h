#pragma once
#include "System.h"

class AABBTree;

class BroadPhaseUpdateSystem : public System
{
public:
	BroadPhaseUpdateSystem(AABBTree& tree) : aabbTree(tree) {}

	void Update(ECSScene& scene, float dt) final override;

private:
	AABBTree& aabbTree;
};

