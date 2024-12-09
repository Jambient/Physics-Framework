#pragma once
#include "Definitions.h"
#include "Structures.h"

class Octree
{
private:
	std::unique_ptr<OctreeNode> m_root;
	int m_maxDepth; // max depth of tree
	int m_maxEntities; // max entities per node before splitting

	void Subdivide(OctreeNode& node);

public:
	Octree(const AABB& bounds, int maxDepth = 5, int maxEntities = 8);
};

