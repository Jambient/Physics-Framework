#pragma once
#include "System.h"

class AABBTree;

class BroadPhaseUpdateSystem : public System
{
public:
	BroadPhaseUpdateSystem(AABBTree& tree) : m_aabbTree(tree) {}

	void Update(ECSScene& scene, float dt) final override;

private:
	AABBTree& m_aabbTree;
};

