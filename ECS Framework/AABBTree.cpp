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
	for (int i = 0; i < maxNodes; i++)
	{
		m_availableNodes.push(i);
	}

	m_entityToNodeIndex.resize(MAX_ENTITIES, NULL_INDEX);
	m_nodeToDenseIndex.resize(maxNodes, NULL_INDEX);
}

Node& AABBTree::GetNode(int nodeIndex)
{
	return m_nodes[m_nodeToDenseIndex[nodeIndex]];
}

void AABBTree::InsertLeaf(Entity entity, AABB box)
{
	int leafIndex = AllocateLeafNode(entity, box);
	m_entityToNodeIndex[entity] = leafIndex;

	if (m_rootIndex == NULL_INDEX)
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

	if (oldParent != NULL_INDEX)
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
	RefitFromNode(newParent, true);
}

void AABBTree::RemoveLeaf(int leafIndex)
{
	if (m_nodeToDenseIndex[leafIndex] == NULL_INDEX)
	{
		return;
	}

	Entity entity = GetNode(leafIndex).entity;
	m_entityToNodeIndex[entity] = NULL_INDEX;

	if (leafIndex == m_rootIndex)
	{
		// If the leaf is the root, just clear the root
		m_rootIndex = NULL_INDEX;
		DeallocateNode(leafIndex);
		return;
	}

	int parentIndex = GetNode(leafIndex).parentIndex;
	int grandParentIndex = GetNode(parentIndex).parentIndex;
	int siblingIndex = GetNode(parentIndex).child1 == leafIndex ? GetNode(parentIndex).child2 : GetNode(parentIndex).child1;

	if (grandParentIndex != NULL_INDEX)
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
		GetNode(siblingIndex).parentIndex = NULL_INDEX;
	}

	// Deallocate the parent node and the leaf node
	DeallocateNode(parentIndex);
	DeallocateNode(leafIndex);
}

void AABBTree::Update(Entity entity, const AABB& newBox)
{
	int leafIndex = m_entityToNodeIndex[entity];

	// check if the entity actually exists in the tree
	if (leafIndex == NULL_INDEX) { return; }

	GetNode(leafIndex).box = newBox;

	// check if the leaf node actually needs updating
	//if (!NeedsUpdate(leafIndex)) { RefitFromNode(GetNode(leafIndex).parentIndex); return; }
	if (!NeedsUpdate(leafIndex)) { return; }

	RemoveLeaf(leafIndex);
	InsertLeaf(entity, newBox);
}

Entity AABBTree::Intersect(const Ray& ray, float& closestDistance)
{
	Entity closestEntity = INVALID_ENTITY;
	closestDistance = FLT_MAX;

	if (m_rootIndex == NULL_INDEX)
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

int AABBTree::CalculateNodeLevel(int nodeIndex) const
{
	if (nodeIndex == NULL_INDEX)
		return -1; // Invalid index, return -1 to indicate error.

	int level = 0;
	while (nodeIndex != m_rootIndex)
	{
		nodeIndex = m_nodes[nodeIndex].parentIndex;
		if (nodeIndex == NULL_INDEX)
			return -1; // Safety check: node is not in the tree.
		level++;
	}
	return level;
}

int AABBTree::GetNodeIndex(const Node& node) const
{
	if (node.parentIndex == NULL_INDEX)
	{
		return m_rootIndex;
	}
	else
	{
		int child1 = m_nodes[node.parentIndex].child1;
		int child2 = m_nodes[node.parentIndex].child2;
		return m_nodes[child1].entity == node.entity ? child1 : child2;
	}
}

void AABBTree::PrintTree(int index, int depth)
{
	if (index == NULL_INDEX) return;

	for (int i = 0; i < depth; ++i) std::cout << "  ";
	std::cout << "Node " << index << ": " << (m_nodes[index].isLeaf ? "Leaf" : "Internal")
		<< " | Entity: " << m_nodes[index].entity << "\n";

	if (!m_nodes[index].isLeaf)
	{
		PrintTree(m_nodes[index].child1, depth + 1);
		PrintTree(m_nodes[index].child2, depth + 1);
	}
}

int AABBTree::GetDeepestLevel() const
{
	if (m_rootIndex == NULL_INDEX) {
		return 0; // Empty tree
	}

	// Helper function for recursive depth traversal
	std::function<int(int)> CalculateDepth = [&](int nodeIndex) -> int {
		if (nodeIndex == NULL_INDEX) {
			return 0;
		}

		Node node = m_nodes[m_nodeToDenseIndex[nodeIndex]];

		// Recursively calculate depth of child nodes
		int leftDepth = CalculateDepth(node.child1);
		int rightDepth = CalculateDepth(node.child2);

		// Return the greater depth + 1 (current level)
		return 1 + std::max(leftDepth, rightDepth);
		};

	return CalculateDepth(m_rootIndex);
}

std::vector<std::pair<Entity, Entity>> AABBTree::GetPotentialIntersections()
{
	std::vector<std::pair<Entity, Entity>> potentialIntersections;

	if (m_rootIndex == NULL_INDEX || GetNode(m_rootIndex).isLeaf) {
		return potentialIntersections; // Empty tree
	}

	// Stack for node pairs to check for overlap
	std::stack<std::pair<int, int>> nodePairs;
	nodePairs.push({ GetNode(m_rootIndex).child1, GetNode(m_rootIndex).child2 });

	while (!nodePairs.empty())
	{
		auto [indexA, indexB] = nodePairs.top();
		nodePairs.pop();

		//Get nodes
		const Node& nodeA = GetNode(indexA);
		const Node& nodeB = GetNode(indexB);

		if (nodeA.isLeaf)
		{
			if (nodeB.isLeaf)
			{
				if (AABB::Overlap(nodeA.box, nodeB.box))
				{
					potentialIntersections.push_back({ nodeA.entity, nodeB.entity });
				}
			}
			else
			{
				nodePairs.push({ indexA, nodeB.child1 });
				nodePairs.push({ indexA, nodeB.child2 });
			}
		}
		else
		{
			if (nodeB.isLeaf)
			{
				nodePairs.push({ nodeA.child1, indexB });
				nodePairs.push({ nodeA.child2, indexB });
			}
			else
			{
				nodePairs.push({ nodeA.child1, nodeB.child1 });
				nodePairs.push({ nodeA.child1, nodeB.child2 });
				nodePairs.push({ nodeA.child2, nodeB.child1 });
				nodePairs.push({ nodeA.child2, nodeB.child2 });
			}
		}
	}
	//nodePairs.push({ m_rootIndex, m_rootIndex });

	//while (!nodePairs.empty()) {
	//	auto [indexA, indexB] = nodePairs.top();
	//	nodePairs.pop();

	//	// Get nodes
	//	const Node& nodeA = GetNode(indexA);
	//	const Node& nodeB = GetNode(indexB);

	//	// Skip if their AABBs do not overlap
	//	if (!AABB::Overlap(nodeA.box, nodeB.box)) {
	//		continue;
	//	}

	//	// If both nodes are leaves and not the same node, add them to the result
	//	if (nodeA.isLeaf && nodeB.isLeaf && indexA != indexB) {
	//		potentialIntersections.push_back({ nodeA.entity, nodeB.entity });
	//	}
	//	// If one or both are internal nodes, push their children for further checks
	//	else {
	//		if (!nodeA.isLeaf) {
	//			nodePairs.push({ nodeA.child1, indexB });
	//			nodePairs.push({ nodeA.child2, indexB });
	//		}
	//		if (!nodeB.isLeaf) {
	//			nodePairs.push({ indexA, nodeB.child1 });
	//			nodePairs.push({ indexA, nodeB.child2 });
	//		}
	//	}
	//}

	return potentialIntersections;
}

int AABBTree::AllocateLeafNode(Entity entity, const AABB& box)
{
	Node leafNode;
	leafNode.box = box;
	leafNode.enlargedBox = box.enlarged(ENLARGEMENT_FACTOR);
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
		int lastNodeIndex = m_denseToNodeIndex[lastDenseIndex];

		// Update mappings
		m_nodeToDenseIndex[lastNodeIndex] = removedDenseIndex;
		m_denseToNodeIndex[removedDenseIndex] = lastNodeIndex;
	}

	// Erase the last element
	m_nodes.pop_back();
	m_denseToNodeIndex.pop_back();
	m_nodeToDenseIndex[index] = NULL_INDEX;
	m_availableNodes.push(index);
	m_nodeCount--;
}

int AABBTree::PickBest(const AABB& leafBox)
{
	int leafBoxArea = leafBox.area();
	int bestSibling = m_rootIndex;
	float bestCost = AABB::Union(GetNode(m_rootIndex).box, leafBox).area();

	std::vector<float> costCache(m_nodeToDenseIndex.size());
	std::vector<QueueNode> queue;
	queue.push_back({ m_rootIndex, bestCost });

	size_t front = 0;

	while (front < queue.size())
	{
		QueueNode current = queue[front++];

		if (current.index == NULL_INDEX)
			continue;

		if (current.cost < bestCost)
		{
			bestCost = current.cost;
			bestSibling = current.index;
		}

		Node& currentNode = GetNode(current.index);
		int parentIndex = currentNode.parentIndex;
		int parentCost = parentIndex != NULL_INDEX ? costCache[parentIndex] : 0;
		costCache[current.index] = AABB::Union(leafBox, currentNode.box).area() - currentNode.box.area() + parentCost;

		// calculate the lower bound cost
		int subTreeLowerBoundCost = leafBoxArea + costCache[current.index];

		if (subTreeLowerBoundCost < bestCost)
		{
			// Push child nodes with their estimated costs
			int child1 = currentNode.child1;
			int child2 = currentNode.child2;

			if (child1 != NULL_INDEX)
			{
				Node& child1Node = GetNode(child1);
				float child1Cost = AABB::Union(leafBox, child1Node.box).area() + costCache[child1Node.parentIndex];
				queue.push_back({ child1, child1Cost });
			}
			if (child2 != NULL_INDEX)
			{
				Node& child2Node = GetNode(child2);
				float child2Cost = AABB::Union(leafBox, child2Node.box).area() + costCache[child2Node.parentIndex];
				queue.push_back({ child2, child2Cost });
			}
		}
	}

	return bestSibling;
}

void AABBTree::RefitFromNode(int index, bool rotateTree)
{
	while (index != NULL_INDEX)
	{
		Node& currentNode = GetNode(index);
		Node& child1 = GetNode(currentNode.child1);
		Node& child2 = GetNode(currentNode.child2);

		currentNode.box = AABB::Union(child1.box, child2.box);
		
		if (rotateTree)
		{
			//Rotate(index);
		}

		index = currentNode.parentIndex;
	}
}

bool AABBTree::NeedsUpdate(int index)
{
	Node& node = GetNode(index);
	AABB newBox = node.box;
	AABB oldBox = node.enlargedBox;

	return (newBox.lowerBound.x < oldBox.lowerBound.x || newBox.lowerBound.y < oldBox.lowerBound.y || newBox.lowerBound.z < oldBox.lowerBound.z ||
		newBox.upperBound.x > oldBox.upperBound.x || newBox.upperBound.y > oldBox.upperBound.y || newBox.upperBound.z > oldBox.upperBound.z);
}

void AABBTree::Rotate(int index)
{

}
