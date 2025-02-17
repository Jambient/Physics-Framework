#pragma once
#include "Vector3.h"

class ECSScene;
class AABBTree;

class PhysicsHelper
{
public:
	static void CreateCloth(ECSScene& scene, AABBTree& tree, Vector3 center, unsigned int rows, unsigned int cols, float spacing, float stiffness, bool hasStructureSprings = true, bool hasShearingSprings = true, bool hasBendingSrings = true);
};