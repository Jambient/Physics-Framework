#include "AABBTree.h"
#include <queue>
#include <utility>
#include <iostream>
#include <functional>
#include <stack>

struct QueueNode {
	int index;
	float cost;
	bool operator>(const QueueNode& other) const {
		return cost > other.cost; // Min-heap based on cost
	}
};

AABBTree::AABBTree()
{
	unsigned int maxNodes = 2 * MAX_ENTITIES - 1;
	for (unsigned int i = 0; i < maxNodes; i++)
	{
		m_availableNodes.push(i);
	}

	m_entityToNodeIndex.resize(MAX_ENTITIES, NULL_NODE_INDEX);
	m_nodeToDenseIndex.resize(maxNodes, NULL_NODE_INDEX);
}

Node& AABBTree::GetNode(int nodeIndex)
{
	return m_nodes[m_nodeToDenseIndex[nodeIndex]];
}

Node& AABBTree::GetNodeFromEntity(Entity entity)
{
	return m_nodes[m_nodeToDenseIndex[m_entityToNodeIndex[entity]]];
}

void AABBTree::InsertEntity(Entity entity, AABB box, bool isStatic)
{
	int leafIndex = AllocateLeafNode(entity, box);
	GetNode(leafIndex).isStatic = isStatic;
	m_entityToNodeIndex[entity] = leafIndex;

	if (m_rootIndex == NULL_NODE_INDEX)
	{
		m_rootIndex = leafIndex;
		return;
	}

	// stage 1: find the best sibling for the new leaf
	int bestSibling = PickBest(box);

	// stage 2: create a new parent
	int oldParent = GetNode(bestSibling).parentIndex;
	int newParent = AllocateInternalNode();
	GetNode(newParent).parentIndex = oldParent;
	GetNode(newParent).box = AABB::Union(box, GetNode(bestSibling).box);

	if (oldParent != NULL_NODE_INDEX)
	{
		if (GetNode(oldParent).child1 == bestSibling)
		{
			GetNode(oldParent).child1 = newParent;
		}
		else
		{
			GetNode(oldParent).child2 = newParent;
		}
	}
	else
	{
		// the sibling was the root
		m_rootIndex = newParent;
	}

	GetNode(newParent).child1 = bestSibling;
	GetNode(newParent).child2 = leafIndex;
	GetNode(bestSibling).parentIndex = newParent;
	GetNode(leafIndex).parentIndex = newParent;

	// stage 3: walk back up the tree refitting AABBs
	RefitFromNode(newParent);
}

void AABBTree::RemoveLeaf(int leafIndex)
{
	if (m_nodeToDenseIndex[leafIndex] == NULL_NODE_INDEX)
	{
		return;
	}

	Entity entity = GetNode(leafIndex).entity;
	m_entityToNodeIndex[entity] = NULL_NODE_INDEX;

	if (leafIndex == m_rootIndex)
	{
		// If the leaf is the root, just clear the root
		m_rootIndex = NULL_NODE_INDEX;
		DeallocateNode(leafIndex);
		return;
	}

	int parentIndex = GetNode(leafIndex).parentIndex;
	int grandParentIndex = GetNode(parentIndex).parentIndex;
	int siblingIndex = GetNode(parentIndex).child1 == leafIndex ? GetNode(parentIndex).child2 : GetNode(parentIndex).child1;

	if (grandParentIndex != NULL_NODE_INDEX)
	{
		// Update the grandparent to point to the sibling
		if (GetNode(grandParentIndex).child1 == parentIndex)
			GetNode(grandParentIndex).child1 = siblingIndex;
		else
			GetNode(grandParentIndex).child2 = siblingIndex;

		GetNode(siblingIndex).parentIndex = grandParentIndex;

		// Refit the tree upwards
		RefitFromNode(grandParentIndex);
	}
	else
	{
		// If no grandparent, the sibling becomes the new root
		m_rootIndex = siblingIndex;
		GetNode(siblingIndex).parentIndex = NULL_NODE_INDEX;
	}

	// Deallocate the parent node and the leaf node
	DeallocateNode(parentIndex);
	DeallocateNode(leafIndex);
}

void AABBTree::RemoveEntity(Entity entity)
{
	RemoveLeaf(m_entityToNodeIndex[entity]);
}

void AABBTree::UpdatePosition(Entity entity, const Vector3& newPosition)
{
	int leafIndex = m_entityToNodeIndex[entity];

	// check if the entity actually exists in the tree
	if (leafIndex == NULL_NODE_INDEX) { return; }

	GetNode(leafIndex).box.UpdatePosition(newPosition);
}

void AABBTree::UpdateScale(Entity entity, const Vector3& newScale)
{
	int leafIndex = m_entityToNodeIndex[entity];

	// check if the entity actually exists in the tree
	if (leafIndex == NULL_NODE_INDEX) { return; }

	GetNode(leafIndex).box.UpdateScale(newScale);
}

void AABBTree::TriggerUpdate(Entity entity)
{
	int leafIndex = m_entityToNodeIndex[entity];

	// check if the entity actually exists in the tree
	if (leafIndex == NULL_NODE_INDEX) { return; }

	if (GetNode(leafIndex).isStatic) { return; }

	// check if the leaf node actually needs updating
	if (!NeedsUpdate(leafIndex)) 
	{ 
		RefitFromNode(GetNode(leafIndex).parentIndex);
		return; 
	}

	AABB previousBox = GetNode(leafIndex).box;

	RemoveLeaf(leafIndex);
	InsertEntity(entity, previousBox, false);
}

Entity AABBTree::Intersect(const Ray& ray, float& closestDistance)
{
	Entity closestEntity = INVALID_ENTITY;
	closestDistance = FLT_MAX;

	if (m_rootIndex == NULL_NODE_INDEX)
		return closestEntity;

	std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int>>, std::greater<>> queue;

	float initialDistance;
	if (ray.Intersect(GetNode(m_rootIndex).box, initialDistance))
		queue.push({ initialDistance, m_rootIndex });

	while (!queue.empty())
	{
		auto [distance, currentIndex] = queue.top();
		queue.pop();

		const Node& currentNode = GetNode(currentIndex);

		// Skip nodes farther than the closest intersection found so far
		if (distance >= closestDistance)
			continue;

		// Test leaf nodes
		if (currentNode.isLeaf)
		{
			// This is a leaf node, update closest intersection if needed
			if (distance < closestDistance)
			{
				closestEntity = currentNode.entity;
				closestDistance = distance;
			}
		}
		else
		{
			// Internal node, test children
			float dist1, dist2;

			bool intersectChild1 = ray.Intersect(GetNode(currentNode.child1).box, dist1);
			bool intersectChild2 = ray.Intersect(GetNode(currentNode.child2).box, dist2);

			if (intersectChild1)
				queue.push({ dist1, currentNode.child1 });
			if (intersectChild2)
				queue.push({ dist2, currentNode.child2 });
		}
	}

	return closestEntity;
}

std::vector<std::pair<Entity, Entity>> AABBTree::GetPotentialIntersections()
{
	std::vector<std::pair<Entity, Entity>> intersections;
	if (m_rootIndex == NULL_NODE_INDEX) return intersections;

	std::unordered_set<uint64_t> found;

	// Start traversal from the root
	PotentialIntersectionHelper(intersections, found, m_rootIndex, m_rootIndex);

	return intersections;
}

void AABBTree::PotentialIntersectionHelper(std::vector<std::pair<Entity, Entity>>& intersections, std::unordered_set<uint64_t>& found, int nodeA, int nodeB)
{
	if (nodeA == NULL_NODE_INDEX || nodeB == NULL_NODE_INDEX) return;

	const Node& a = GetNode(nodeA);
	const Node& b = GetNode(nodeB);

	if (!AABB::Overlap(a.box, b.box)) return;
	if (a.isStatic && b.isStatic) return;

	if (a.isLeaf && b.isLeaf && a.entity != b.entity) {
		uint64_t a64 = (uint64_t)a.entity;
		uint64_t b64 = (uint64_t)b.entity;
		uint64_t hash = a.entity > b.entity ? ((a64 << 32) | b64) : ((b64 << 32) | a64);

		if (found.find(hash) == found.end())
		{
			intersections.emplace_back(a.entity, b.entity);
			found.insert(hash);
		}
	}
	else {
		if (!a.isLeaf && !b.isLeaf) {
			PotentialIntersectionHelper(intersections, found, a.child1, b.child2);
			PotentialIntersectionHelper(intersections, found, a.child2, b.child1);
			PotentialIntersectionHelper(intersections, found, a.child2, b.child2);
			PotentialIntersectionHelper(intersections, found, a.child1, b.child1);
		}
		else if (!a.isLeaf) {
			PotentialIntersectionHelper(intersections, found, a.child1, nodeB);
			PotentialIntersectionHelper(intersections, found, a.child2, nodeB);
		}
		else {
			PotentialIntersectionHelper(intersections, found, nodeA, b.child1);
			PotentialIntersectionHelper(intersections, found, nodeA, b.child2);
		}
	}
}

int AABBTree::AllocateLeafNode(Entity entity, const AABB& box)
{
	Node leafNode;
	leafNode.box = box;
	leafNode.enlargedBox = box.GetEnlarged(BOX_ENLARGEMENT_FACTOR);
	leafNode.entity = entity;
	leafNode.isLeaf = true;
	m_nodes.push_back(leafNode);

	int nodeIndex = m_availableNodes.front();
	m_availableNodes.pop();

	m_nodeToDenseIndex[nodeIndex] = m_nodeCount;
	m_denseToNodeIndex.push_back(nodeIndex);
	m_nodeCount++;

	return nodeIndex;
}

int AABBTree::AllocateInternalNode()
{
	Node internalNode;
	m_nodes.push_back(internalNode);

	int nodeIndex = m_availableNodes.front();
	m_availableNodes.pop();

	m_nodeToDenseIndex[nodeIndex] = m_nodeCount;
	m_denseToNodeIndex.push_back(nodeIndex);
	m_nodeCount++;

	return nodeIndex;
}

void AABBTree::DeallocateNode(int index)
{
	size_t removedDenseIndex = m_nodeToDenseIndex[index];
	size_t lastDenseIndex = m_nodes.size() - 1;

	if (removedDenseIndex != lastDenseIndex)
	{
		// Replace removed element with the last one
		m_nodes[removedDenseIndex] = m_nodes[lastDenseIndex];
		size_t lastNodeIndex = m_denseToNodeIndex[lastDenseIndex];

		// Update mappings
		m_nodeToDenseIndex[lastNodeIndex] = removedDenseIndex;
		m_denseToNodeIndex[removedDenseIndex] = lastNodeIndex;
	}

	// Erase the last element
	m_nodes.pop_back();
	m_denseToNodeIndex.pop_back();
	m_nodeToDenseIndex[index] = NULL_NODE_INDEX;
	m_availableNodes.push(index);
	m_nodeCount--;
}

int AABBTree::PickBest(const AABB& leafBox)
{
	float leafBoxArea = leafBox.GetArea();
	int bestSibling = m_rootIndex;
	float bestCost = AABB::Union(GetNode(m_rootIndex).box, leafBox).GetArea();

	std::vector<float> costCache(m_nodeToDenseIndex.size());
	std::vector<QueueNode> queue;
	queue.push_back({ m_rootIndex, bestCost });

	size_t front = 0;

	while (front < queue.size())
	{
		QueueNode current = queue[front++];

		if (current.index == NULL_NODE_INDEX)
			continue;

		if (current.cost < bestCost)
		{
			bestCost = current.cost;
			bestSibling = current.index;
		}

		Node& currentNode = GetNode(current.index);
		int parentIndex = currentNode.parentIndex;
		int parentCost = parentIndex != NULL_NODE_INDEX ? costCache[parentIndex] : 0;
		costCache[current.index] = AABB::Union(leafBox, currentNode.box).GetArea() - currentNode.box.GetArea() + parentCost;

		// calculate the lower bound cost
		float subTreeLowerBoundCost = leafBoxArea + costCache[current.index];

		if (subTreeLowerBoundCost < bestCost)
		{
			// Push child nodes with their estimated costs
			int child1 = currentNode.child1;
			int child2 = currentNode.child2;

			if (child1 != NULL_NODE_INDEX)
			{
				Node& child1Node = GetNode(child1);
				float child1Cost = AABB::Union(leafBox, child1Node.box).GetArea() + costCache[child1Node.parentIndex];
				queue.push_back({ child1, child1Cost });
			}
			if (child2 != NULL_NODE_INDEX)
			{
				Node& child2Node = GetNode(child2);
				float child2Cost = AABB::Union(leafBox, child2Node.box).GetArea() + costCache[child2Node.parentIndex];
				queue.push_back({ child2, child2Cost });
			}
		}
	}

	return bestSibling;
}

void AABBTree::RefitFromNode(int index)
{
	while (index != NULL_NODE_INDEX)
	{
		Node& currentNode = GetNode(index);
		Node& child1 = GetNode(currentNode.child1);
		Node& child2 = GetNode(currentNode.child2);

		currentNode.box = AABB::Union(child1.box, child2.box);
		currentNode.isStatic = child1.isStatic && child2.isStatic;

		index = currentNode.parentIndex;
	}
}

bool AABBTree::NeedsUpdate(int index)
{
	Node& node = GetNode(index);
	AABB newBox = node.box;
	AABB oldBox = node.enlargedBox;

	Vector3 lowerBoundNew = newBox.GetLowerBound();
	Vector3 upperBoundNew = newBox.GetUpperBound();
	Vector3 lowerBoundOld = oldBox.GetLowerBound();
	Vector3 upperBoundOld = oldBox.GetUpperBound();

	return (lowerBoundNew.x < lowerBoundOld.x || lowerBoundNew.y < lowerBoundOld.y || lowerBoundNew.z < lowerBoundOld.z ||
		upperBoundNew.x > upperBoundOld.x || upperBoundNew.y > upperBoundOld.y || upperBoundNew.z > upperBoundOld.z);
}
