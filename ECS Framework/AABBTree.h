#pragma once
#include "Definitions.h"
#include "Vector3.h"
#include "AABB.h"
#include "Ray.h"
#include <vector>
#include <queue>

// inspired by the 2019 GDC talk by Erin Catto https://box2d.org/files/ErinCatto_DynamicBVH_GDC2019.pdf

constexpr int NULL_INDEX = -1;
constexpr float ENLARGEMENT_FACTOR = 0.1f;

struct Node
{
	AABB enlargedBox;
	AABB box;
	Entity entity;
	int parentIndex = NULL_INDEX;
	int child1 = NULL_INDEX;
	int child2 = NULL_INDEX;
	bool isLeaf = false;
};

class AABBTree
{
public:
	AABBTree();

	Node GetNode(int nodeIndex) const { return nodeIndex != NULL_INDEX ? m_nodes[nodeIndex] : Node(); }
	std::vector<Node> GetNodes() const { return m_nodes; }

	void InsertLeaf(Entity entity, AABB box);
	void RemoveLeaf(int leafIndex);

	void Update(Entity entity, const AABB& newBox);

	Entity Intersect(const Ray& ray, float& closestDistance);
	int CalculateNodeLevel(int nodeIndex) const;
	int GetNodeIndex(const Node& node) const;

	void PrintTree(int index, int depth = 0);
	int GetRootIndex() const { return m_rootIndex; }

private:
	int AllocateLeafNode(Entity entity, const AABB& box);
	int AllocateInternalNode();
	int PickBest(const AABB& leafBox);

	float CalculateCost(int index, const AABB& leafBox) const;

	void RefitFromNode(int index, bool rotateTree = false);
	bool NeedsUpdate(int index);

	void Rotate(int index);

	std::vector<Node> m_nodes;
	int m_rootIndex = NULL_INDEX;

	std::vector<int> m_nodeToDenseIndex;
	std::vector<int> m_entityToNodeIndex;

	std::queue<int> m_availableNodes;
};

