#pragma once
#include "Definitions.h"
#include "Vector3.h"
#include "AABB.h"
#include "Ray.h"
#include <vector>
#include <queue>
#include <unordered_set>

// inspired by the 2019 GDC talk by Erin Catto https://box2d.org/files/ErinCatto_DynamicBVH_GDC2019.pdf

constexpr int NULL_INDEX = -1;
constexpr float ENLARGEMENT_FACTOR = 0.3f;

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

	Node& GetNode(int nodeIndex);
	Node& GetNodeFromEntity(Entity entity);
	std::vector<Node> GetNodes() const { return m_nodes; }

	void InsertLeaf(Entity entity, AABB box);
	void RemoveLeaf(int leafIndex);
	void RemoveEntity(Entity entity);

	void UpdatePosition(Entity entity);
	void UpdateSize(Entity entity);
	void TriggerUpdate(Entity entity);

	Entity Intersect(const Ray& ray, float& closestDistance);
	int CalculateNodeLevel(int nodeIndex) const;
	int GetNodeIndex(const Node& node) const;

	void PrintTree(int index, int depth = 0);
	int GetRootIndex() const { return m_rootIndex; }

	int GetDeepestLevel() const;

	std::vector<std::pair<Entity, Entity>> GetPotentialIntersections();

private:
	int AllocateLeafNode(Entity entity, const AABB& box);
	int AllocateInternalNode();
	void DeallocateNode(int index);

	int PickBest(const AABB& leafBox);

	void RefitFromNode(int index, bool rotateTree = false);
	bool NeedsUpdate(int index);

	void Rotate(int index);

	void PotentialIntersectionHelper(std::vector<std::pair<Entity, Entity>>& intersections, std::unordered_set<uint64_t>& found, int nodeA, int nodeB);

	std::vector<Node> m_nodes;
	int m_nodeCount;
	int m_rootIndex = NULL_INDEX;

	std::vector<size_t> m_denseToNodeIndex;
	std::vector<int> m_nodeToDenseIndex;
	std::vector<int> m_entityToNodeIndex;

	std::queue<int> m_availableNodes;
};

