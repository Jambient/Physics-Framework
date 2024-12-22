#include "AABBTree.h"
#include <queue>
#include <utility>
#include <iostream>
#include <functional>

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

Node AABBTree::GetNode(int nodeIndex) const
{
	if (nodeIndex == NULL_INDEX)
	{
		return Node();
	}
	else
	{
		return m_nodes[m_nodeToDenseIndex[nodeIndex]];
	}
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
	int oldParent = m_nodes[bestSibling].parentIndex;
	int newParent = AllocateInternalNode();
	m_nodes[newParent].parentIndex = oldParent;
	m_nodes[newParent].box = AABB::Union(box, m_nodes[bestSibling].box);

	if (oldParent != NULL_INDEX)
	{
		if (m_nodes[oldParent].child1 == bestSibling)
		{
			m_nodes[oldParent].child1 = newParent;
		}
		else
		{
			m_nodes[oldParent].child2 = newParent;
		}
	}
	else
	{
		// the sibling was the root
		m_rootIndex = newParent;
	}

	m_nodes[newParent].child1 = bestSibling;
	m_nodes[newParent].child2 = leafIndex;
	m_nodes[bestSibling].parentIndex = newParent;
	m_nodes[leafIndex].parentIndex = newParent;

	// stage 3: walk back up the tree refitting AABBs
	RefitFromNode(newParent, true);
}

void AABBTree::RemoveLeaf(int leafIndex)
{
	Entity entity = m_nodes[leafIndex].entity;

	m_entityToNodeIndex[entity] = NULL_INDEX;

	if (leafIndex == m_rootIndex)
	{
		m_rootIndex = NULL_INDEX;
		return;
	}

	int parentIndex = m_nodes[leafIndex].parentIndex;
	int grandParentIndex = m_nodes[parentIndex].parentIndex;
	int siblingIndex = m_nodes[parentIndex].child1 == leafIndex ? m_nodes[parentIndex].child2 : m_nodes[parentIndex].child1;

	if (grandParentIndex != NULL_INDEX)
	{
		if (m_nodes[grandParentIndex].child1 == parentIndex)
			m_nodes[grandParentIndex].child1 = siblingIndex;
		else
			m_nodes[grandParentIndex].child2 = siblingIndex;

		m_nodes[siblingIndex].parentIndex = grandParentIndex;

		RefitFromNode(grandParentIndex);
	}
	else
	{
		m_rootIndex = siblingIndex;
		m_nodes[siblingIndex].parentIndex = NULL_INDEX;
	}
}

void AABBTree::Update(Entity entity, const AABB& newBox)
{
	int leafIndex = m_entityToNodeIndex[entity];

	// check if the entity actually exists in the tree
	if (leafIndex == NULL_INDEX) { return; }

	m_nodes[leafIndex].box = newBox;

	// check if the leaf node actually needs updating
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
	if (ray.Intersect(m_nodes[m_rootIndex].box, initialDistance))
		queue.push({ initialDistance, m_rootIndex });

	while (!queue.empty())
	{
		auto [distance, currentIndex] = queue.top();
		queue.pop();

		const Node& currentNode = m_nodes[currentIndex];

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

			bool intersectChild1 = ray.Intersect(m_nodes[currentNode.child1].box, dist1);
			bool intersectChild2 = ray.Intersect(m_nodes[currentNode.child2].box, dist2);

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

	m_nodeToDenseIndex[nodeIndex] = m_nodes.size() - 1;

	return nodeIndex;
}

int AABBTree::AllocateInternalNode()
{
	Node internalNode;
	m_nodes.push_back(internalNode);
	return m_nodes.size() - 1;
}

int AABBTree::PickBest(const AABB& leafBox)
{
	int leafBoxArea = leafBox.area();
	int bestSibling = m_rootIndex;
	float bestCost = CalculateCost(m_rootIndex, leafBox);

	std::vector<float> costCache(m_nodes.size());
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

		int parentIndex = m_nodes[current.index].parentIndex;
		int parentCost = parentIndex != NULL_INDEX ? costCache[parentIndex] : 0;
		costCache[current.index] = AABB::Union(leafBox, m_nodes[current.index].box).area() - m_nodes[current.index].box.area() + parentCost;

		// calculate the lower bound cost
		int subTreeLowerBoundCost = leafBoxArea + costCache[current.index];

		if (subTreeLowerBoundCost < bestCost)
		{
			// Push child nodes with their estimated costs
			int child1 = m_nodes[current.index].child1;
			int child2 = m_nodes[current.index].child2;

			if (child1 != NULL_INDEX)
			{
				float child1Cost = AABB::Union(leafBox, m_nodes[child1].box).area() + costCache[m_nodes[child1].parentIndex]; //CalculateCost(child1, leafBox);
				queue.push_back({ child1, child1Cost });
			}
			if (child2 != NULL_INDEX)
			{
				float child2Cost = AABB::Union(leafBox, m_nodes[child2].box).area() + costCache[m_nodes[child2].parentIndex]; //CalculateCost(child2, leafBox);
				queue.push_back({ child2, child2Cost });
			}
		}
	}

	return bestSibling;
}

float AABBTree::CalculateCost(int index, const AABB& leafBox) const
{
	float cost = AABB::Union(leafBox, m_nodes[index].box).area();
	int currentIndex = m_nodes[index].parentIndex;

	while (currentIndex != NULL_INDEX)
	{
		AABB currentIndexBox = m_nodes[currentIndex].box;
		cost += AABB::Union(leafBox, currentIndexBox).area() - currentIndexBox.area();

		currentIndex = m_nodes[currentIndex].parentIndex;
	}

	return cost;
}

void AABBTree::RefitFromNode(int index, bool rotateTree)
{
	while (index != NULL_INDEX)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		m_nodes[index].box = AABB::Union(m_nodes[child1].box, m_nodes[child2].box);
		
		if (rotateTree)
		{
			//Rotate(index);
		}

		index = m_nodes[index].parentIndex;
	}
}

bool AABBTree::NeedsUpdate(int index)
{
	AABB newBox = m_nodes[index].box;
	AABB oldBox = m_nodes[index].enlargedBox;

	return (newBox.lowerBound.x < oldBox.lowerBound.x || newBox.lowerBound.y < oldBox.lowerBound.y || newBox.lowerBound.z < oldBox.lowerBound.z ||
		newBox.upperBound.x > oldBox.upperBound.x || newBox.upperBound.y > oldBox.upperBound.y || newBox.upperBound.z > oldBox.upperBound.z);
}

void AABBTree::Rotate(int index)
{
	std::cout << "Rotating index: " << index << std::endl;

	if (index == NULL_INDEX || m_nodes[index].isLeaf)
		return; // Only rotate internal nodes

	int child1 = m_nodes[index].child1;
	int child2 = m_nodes[index].child2;

	if (child1 == NULL_INDEX || child2 == NULL_INDEX)
		return; // Safety check: invalid children

	Node& parentNode = m_nodes[index];
	Node& childNode1 = m_nodes[child1];
	Node& childNode2 = m_nodes[child2];

	float currentArea = parentNode.box.area();

	// Evaluate potential rotations and select the one that minimizes the area
	float sibling1Area = (childNode2.child1 != NULL_INDEX) ? AABB::Union(childNode1.box, m_nodes[childNode2.child1].box).area() : FLT_MAX;
	float sibling2Area = (childNode2.child2 != NULL_INDEX) ? AABB::Union(childNode1.box, m_nodes[childNode2.child2].box).area() : FLT_MAX;
	float sibling1NewArea = (childNode1.child1 != NULL_INDEX) ? AABB::Union(childNode2.box, m_nodes[childNode1.child1].box).area() : FLT_MAX;
	float sibling2NewArea = (childNode1.child2 != NULL_INDEX) ? AABB::Union(childNode2.box, m_nodes[childNode1.child2].box).area() : FLT_MAX;

	if (sibling1Area < currentArea)
	{
		// Perform left rotation
		parentNode.child2 = childNode2.child1;
		childNode2.child1 = index;
		childNode2.parentIndex = parentNode.parentIndex;
		parentNode.parentIndex = child2;
		parentNode.box = AABB::Union(m_nodes[parentNode.child1].box, m_nodes[parentNode.child2].box);
		childNode2.box = AABB::Union(m_nodes[childNode2.child1].box, m_nodes[childNode2.child2].box);
	}
	else if (sibling2Area < currentArea)
	{
		// Perform right rotation
		parentNode.child2 = childNode2.child2;
		childNode2.child2 = index;
		childNode2.parentIndex = parentNode.parentIndex;
		parentNode.parentIndex = child2;
		parentNode.box = AABB::Union(m_nodes[parentNode.child1].box, m_nodes[parentNode.child2].box);
		childNode2.box = AABB::Union(m_nodes[childNode2.child1].box, m_nodes[childNode2.child2].box);
	}
	else if (sibling1NewArea < currentArea)
	{
		// Perform left rotation on child1
		parentNode.child1 = childNode1.child1;
		childNode1.child1 = index;
		childNode1.parentIndex = parentNode.parentIndex;
		parentNode.parentIndex = child1;
		parentNode.box = AABB::Union(m_nodes[parentNode.child1].box, m_nodes[parentNode.child2].box);
		childNode1.box = AABB::Union(m_nodes[childNode1.child1].box, m_nodes[childNode1.child2].box);
	}
	else if (sibling2NewArea < currentArea)
	{
		// Perform right rotation on child1
		parentNode.child1 = childNode1.child2;
		childNode1.child2 = index;
		childNode1.parentIndex = parentNode.parentIndex;
		parentNode.parentIndex = child1;
		parentNode.box = AABB::Union(m_nodes[parentNode.child1].box, m_nodes[parentNode.child2].box);
		childNode1.box = AABB::Union(m_nodes[childNode1.child1].box, m_nodes[childNode1.child2].box);
	}
}
