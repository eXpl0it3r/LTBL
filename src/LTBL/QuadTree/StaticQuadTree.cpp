#include <LTBL/QuadTree/StaticQuadTree.h>

#include <assert.h>

namespace qdt
{
	StaticQuadTree::StaticQuadTree()
		: m_created(false)
	{
	}

	StaticQuadTree::StaticQuadTree(const AABB &rootRegion)
		: m_created(false)
	{
		m_pRootNode.reset(new QuadTreeNode(rootRegion, 0));

		m_created = true;
	}

	void StaticQuadTree::Create(const AABB &rootRegion)
	{
		m_pRootNode.reset(new QuadTreeNode(rootRegion, 0));

		m_created = true;
	}

	void StaticQuadTree::Add(QuadTreeOccupant* pOc)
	{
		assert(m_created);

		SetQuadTree(pOc);

		// If the occupant fits in the root node
		if(m_pRootNode->GetRegion().Contains(pOc->GetAABB()))
			m_pRootNode->Add(pOc);
		else
			m_outsideRoot.insert(pOc);
	}

	void StaticQuadTree::Clear()
	{
		m_pRootNode.reset();

		m_outsideRoot.clear();

		m_created = false;
	}

	bool StaticQuadTree::Created()
	{
		return m_created;
	}
}