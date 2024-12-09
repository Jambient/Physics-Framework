#include "Octree.h"

void Octree::Subdivide(OctreeNode& node)
{
	XMVECTOR center = XMLoadFloat3(&node.Bounds.Center);
	XMVECTOR quater = XMVectorScale(XMLoadFloat3(&node.Bounds.HalfSize), 0.5f);

	for (int i = 0; i < 8; i++)
	{
		XMVECTOR offset = XMVectorMultiply(quater, XMVectorSet(i & 1 ? 1 : -1, i & 2 ? 1 : -1, i & 4 ? 1 : -1, 0.0f));

		node.Children[i] = std::make_unique<OctreeNode>();
		node.Children[i]->Depth = node.Depth + 1;

		XMStoreFloat3(&node.Children[i]->Bounds.Center, XMVectorAdd(center, offset));
		XMStoreFloat3(&node.Children[i]->Bounds.HalfSize, quater);
	}
}

Octree::Octree(const AABB& bounds, int maxDepth, int maxEntities)
{
	m_maxDepth = maxDepth;
	m_maxEntities = maxEntities;

	m_root = std::make_unique<OctreeNode>();
	m_root->Bounds = bounds;
}
