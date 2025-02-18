#pragma once

class ECSScene;
class AABBTree;
class Vector3;
class Quaternion;

class PhysicsHelper
{
public:
	static void CreateCube(ECSScene& scene, AABBTree& tree, Vector3 center, Vector3 size, Quaternion rotation, float mass);

	static void CreateCloth(ECSScene& scene, AABBTree& tree, Vector3 center, unsigned int rows, unsigned int cols, float spacing, float stiffness, bool hasStructureSprings = true, bool hasShearingSprings = true, bool hasBendingSrings = true);
};