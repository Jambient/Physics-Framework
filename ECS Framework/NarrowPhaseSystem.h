#pragma once
#include <vector>
#include "System.h"
#include "Vector3.h"

class AABBTree;

class NarrowPhaseSystem : public System
{
public:
	NarrowPhaseSystem(AABBTree& tree, std::vector<Vector3>& debugPoints) : m_aabbTree(tree), m_debugPoints(debugPoints) {}

	void Update(ECSScene& scene, float dt) final override;

private:
	AABBTree& m_aabbTree;
	std::vector<Vector3>& m_debugPoints;
};

