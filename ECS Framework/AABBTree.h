// Dyanmic tree implementation for physics broadphase.
// 
// Inspired by the 2019 GDC talk by Erin Catto: 
// https://box2d.org/files/ErinCatto_DynamicBVH_GDC2019.pdf

#pragma once
#ifndef AABBTREE_H_
#define AABBTREE_H_

#include <vector>
#include <queue>
#include <unordered_set>

#include "Definitions.h"
#include "Vector3.h"
#include "Colliders.h"
#include "Ray.h"

constexpr int NULL_NODE_INDEX = -1; // Constant representing a null index for a node.
constexpr float BOX_ENLARGEMENT_FACTOR = 0.3f; // Constant factor by which enlarged box is scaled up from node box

/**
 * @struct Node
 * @brief Represents a node in the AABB tree.
 */
struct Node
{
	AABB enlargedBox; // Larger bounding box to delay node rebuilds. Leaf nodes only.
	AABB box; // Actual bounding box for the node. Leaf ndoes only.
	Entity entity; // ECS entity associated with this node. Leaf nodes only.
	int parentIndex = NULL_NODE_INDEX; // Node index for parent node.
	int child1 = NULL_NODE_INDEX; // Node index for left child node. Internal nodes only.
	int child2 = NULL_NODE_INDEX; // Node index for right child node. Internal nodes only.
	bool isLeaf = false; // Flag indicating whether this node is a leaf.
	bool isStatic = false;
};

/**
 * @class AABBTree
 * @brief A dynamic bounding volume hierachy that uses axis aligned bounding boxes.
 */
class AABBTree
{
public:
	/**
	 * @brief Default constructor.
	 */
	AABBTree();

	/**
	 * @brief Retrieves a node by its node index.
	 * @param nodeIndex The index of the node to retrieve.
	 * @return A reference to the node object.
	 */
	Node& GetNode(int nodeIndex);

	/**
	 * @brief Retrieves a leaf node by its associated ECS entity.
	 * @param entity The ECS entity of the leaf node to retrieve.
	 * @return A reference to the node object.
	 */
	Node& GetNodeFromEntity(Entity entity);

	/**
	 * @brief Get all the nodes currently in the tree.
	 * @return A vector of all tree nodes.
	 */
	std::vector<Node> GetNodes() const { return m_nodes; }

	/**
	 * @brief Create a new leaf node in the tree that represents an ECS entity.
	 * @param entity The ECS entity to associate with the new node.
	 * @param box The bounding box for the new node.
	 */
	void InsertEntity(Entity entity, AABB box, bool isStatic = false);

	void RemoveEntity(Entity entity);

	void UpdatePosition(Entity entity, const Vector3& newPosition);
	void UpdateScale(Entity entity, const Vector3& newScale);
	void TriggerUpdate(Entity entity);

	Entity Intersect(const Ray& ray, float& closestDistance);

	int GetRootIndex() const { return m_rootIndex; }

	std::vector<std::pair<Entity, Entity>> GetPotentialIntersections();

private:
	/**
	 * @brief Removes a leaf node from the tree.
	 * @param leafIndex The node index of the leaf node.
	 */
	void RemoveLeaf(int leafIndex);

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
	int m_rootIndex = NULL_NODE_INDEX;

	std::vector<size_t> m_denseToNodeIndex;
	std::vector<int> m_nodeToDenseIndex;
	std::vector<int> m_entityToNodeIndex;

	std::queue<int> m_availableNodes;
};

#endif // AABBTREE_H_