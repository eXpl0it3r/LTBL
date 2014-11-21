#include <LTBL/QuadTree/QuadTreeOccupant.h>

#include <LTBL/QuadTree/QuadTreeNode.h>
#include <LTBL/QuadTree/QuadTree.h>

#include <LTBL/Constructs/Vec2f.h>

#include <assert.h>

namespace qdt
{
	QuadTreeOccupant::QuadTreeOccupant()
		: m_aabb(Vec2f(0.0f, 0.0f), Vec2f(1.0f, 1.0f)),
		m_pQuadTreeNode(NULL), m_pQuadTree(NULL)
	{
	}

	void QuadTreeOccupant::TreeUpdate()
	{
		if(m_pQuadTree == NULL)
			return;

		if(m_pQuadTreeNode == NULL)
		{
			// If fits in the root now, add it
			QuadTreeNode* pRootNode = m_pQuadTree->m_pRootNode.get();

			if(pRootNode->m_region.Contains(m_aabb))
			{
				// Remove from outside root and add to tree
				m_pQuadTree->m_outsideRoot.erase(this);

				pRootNode->Add(this);
			}
		}
		else
			m_pQuadTreeNode->Update(this);
	}

	void QuadTreeOccupant::RemoveFromTree()
	{
		if(m_pQuadTreeNode == NULL)
		{
			// Not in a node, should be outside root then
			assert(m_pQuadTree != NULL);

			m_pQuadTree->m_outsideRoot.erase(this);

			m_pQuadTree->OnRemoval();
		}
		else
			m_pQuadTreeNode->Remove(this);
	}

	const AABB &QuadTreeOccupant::GetAABB()
	{
		return m_aabb;
	}
}