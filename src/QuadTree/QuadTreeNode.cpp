#include <LTBL/QuadTree/QuadTreeNode.h>

#include <LTBL/QuadTree/QuadTree.h>

#include <assert.h>

namespace qdt
{
	// Defaults
	int QuadTreeNode::minNumOccupants = 3;
	int QuadTreeNode::maxNumOccupants = 6;
	int QuadTreeNode::maxNumLevels = 20;
	float QuadTreeNode::m_oversizeMultiplier = 1.2f;

	QuadTreeNode::QuadTreeNode()
		: m_hasChildren(false), m_numOccupantsBelow(0)
	{
	}

	QuadTreeNode::QuadTreeNode(const AABB &region, int level, QuadTreeNode* pParent, QuadTree* pQuadTree)
		: m_region(region), m_level(level), m_pParent(pParent), m_pQuadTree(pQuadTree),
		m_hasChildren(false), m_numOccupantsBelow(0)
	{
	}

	QuadTreeNode::~QuadTreeNode()
	{
		if(m_hasChildren)
			DestroyChildren();
	}

	void QuadTreeNode::Create(const AABB &region, int level, QuadTreeNode* pParent, QuadTree* pQuadTree)
	{
		m_region = region;
		m_level = level;
		m_pParent = pParent;
		m_pQuadTree = pQuadTree;
	}

	inline QuadTreeNode* QuadTreeNode::GetChild(const Point2i position)
	{
		return &(*m_children)[position.x][position.y];
	}

	void QuadTreeNode::GetPossibleOccupantPosition(QuadTreeOccupant* pOc, Point2i &point)
	{
		// Compare the center of the AABB of the occupant to that of this node to determine
		// which child it may (possibly, not certainly) fit in
		const Vec2f &occupantCenter(pOc->m_aabb.GetCenter());
		const Vec2f &nodeRegionCenter(m_region.GetCenter());

		if(occupantCenter.x > nodeRegionCenter.x)
			point.x = 1;
		else
			point.x = 0;

		if(occupantCenter.y > nodeRegionCenter.y)
			point.y = 1;
		else
			point.y = 0;
	}

	void QuadTreeNode::AddToThisLevel(QuadTreeOccupant* pOc)
	{
		pOc->m_pQuadTreeNode = this;

		m_pOccupants.insert(pOc);
	}

	bool QuadTreeNode::AddToChildren(QuadTreeOccupant* pOc)
	{
		assert(m_hasChildren);

		Point2i position;

		GetPossibleOccupantPosition(pOc, position);

		QuadTreeNode* pChild = GetChild(position);

		// See if the occupant fits in the child at the selected position
		if(pChild->m_region.Contains(pOc->m_aabb))
		{
			// Fits, so can add to the child and finish
			pChild->Add(pOc);

			return true;
		}

		return false;
	}

	void QuadTreeNode::Partition()
	{
		assert(!m_hasChildren);

		const Vec2f &halfRegionDims(m_region.GetHalfDims());
		const Vec2f &regionLowerBound(m_region.GetLowerBound());
		const Vec2f &regionCenter(m_region.GetCenter());

		int nextLowerLevel = m_level - 1;

		m_children = new std::vector<std::vector<QuadTreeNode>>();

		// Create the children nodes
		(*m_children).resize(2);

		for(int x = 0; x < 2; x++)
		{
			(*m_children)[x].resize(2);

			for(int y = 0; y < 2; y++)
			{
				Vec2f offset(x * halfRegionDims.x, y * halfRegionDims.y);

				AABB childAABB(regionLowerBound + offset, regionCenter + offset);

				childAABB.SetHalfDims(childAABB.GetHalfDims() * m_oversizeMultiplier);

				// Scale up AABB by the oversize multiplier

				(*m_children)[x][y].Create(childAABB, nextLowerLevel, this, m_pQuadTree);
			}
		}

		m_hasChildren = true;
	}

	void QuadTreeNode::DestroyChildren()
	{
		assert(m_hasChildren);

		delete m_children;

		m_hasChildren = false;
	}

	void QuadTreeNode::Merge()
	{
		if(m_hasChildren)
		{
			// Place all occupants at lower levels into this node
			GetOccupants(m_pOccupants);

			DestroyChildren();
		}
	}

	void QuadTreeNode::GetOccupants(std::unordered_set<QuadTreeOccupant*> &occupants)
	{
		// Iteratively parse subnodes in order to collect all occupants below this node
		std::list<QuadTreeNode*> open;

		open.push_back(this);

		while(!open.empty())
		{
			// Depth-first (results in less memory usage), remove objects from open list
			QuadTreeNode* pCurrent = open.back();
			open.pop_back();

			// Get occupants
			for(std::unordered_set<QuadTreeOccupant*>::iterator it = pCurrent->m_pOccupants.begin(); it != pCurrent->m_pOccupants.end(); it++)
			{
				// Assign new node
				(*it)->m_pQuadTreeNode = this;

				// Add to this node
				occupants.insert(*it);
			}

			// If the node has children, add them to the open list
			if(pCurrent->m_hasChildren)
			{
				for(int x = 0; x < 2; x++)
					for(int y = 0; y < 2; y++)
						open.push_back(&(*pCurrent->m_children)[x][y]);
			}
		}
	}

	void QuadTreeNode::GetAllOccupantsBelow(std::vector<QuadTreeOccupant*> &occupants)
	{
		// Iteratively parse subnodes in order to collect all occupants below this node
		std::list<QuadTreeNode*> open;

		open.push_back(this);

		while(!open.empty())
		{
			// Depth-first (results in less memory usage), remove objects from open list
			QuadTreeNode* pCurrent = open.back();
			open.pop_back();

			// Get occupants
			for(std::unordered_set<QuadTreeOccupant*>::iterator it = pCurrent->m_pOccupants.begin(); it != pCurrent->m_pOccupants.end(); it++)
				// Add to this node
				occupants.push_back(*it);

			// If the node has children, add them to the open list
			if(pCurrent->m_hasChildren)
			{
				for(int x = 0; x < 2; x++)
					for(int y = 0; y < 2; y++)
						open.push_back(&(*pCurrent->m_children)[x][y]);
			}
		}
	}

	void QuadTreeNode::Update(QuadTreeOccupant* pOc)
	{
		// Remove, may be re-added to this node later
		m_pOccupants.erase(pOc);

		// Propogate upwards, looking for a node that has room (the current one may still have room)
		QuadTreeNode* pNode = this;

		while(pNode != NULL)
		{
			pNode->m_numOccupantsBelow--;

			// If has room for 1 more, found a spot
			if(pNode->m_region.Contains(pOc->m_aabb))
				break;

			pNode = pNode->m_pParent;
		}

		// If no node that could contain the occupant was found, add to outside root set
		if(pNode == NULL)
		{
			m_pQuadTree->m_outsideRoot.insert(pOc);

			pOc->m_pQuadTreeNode = NULL;
		}
		else // Add to the selected node
			pNode->Add(pOc);
	}

	void QuadTreeNode::Remove(QuadTreeOccupant* pOc)
	{
		assert(!m_pOccupants.empty());

		// Remove from node
		m_pOccupants.erase(pOc);

		// Propogate upwards, merging if there are enough occupants in the node
		QuadTreeNode* pNode = this;

		while(pNode != NULL)
		{
			pNode->m_numOccupantsBelow--;

			if(pNode->m_numOccupantsBelow >= minNumOccupants)
			{
				pNode->Merge();

				break;
			}

			pNode = pNode->m_pParent;
		}
	}

	void QuadTreeNode::Add(QuadTreeOccupant* pOc)
	{
		m_numOccupantsBelow++;

		// See if the occupant fits into any children (if there are any)
		if(m_hasChildren)
		{
			if(AddToChildren(pOc))
				return; // Fit, can stop
		}
		else
		{
			// Check if we need a new partition
			if(static_cast<signed>(m_pOccupants.size()) >= maxNumOccupants && m_level < maxNumLevels)
			{
				Partition();

				if(AddToChildren(pOc))
					return;
			}
		}

		// Did not fit in anywhere, add to this level, even if it goes over the maximum size
		AddToThisLevel(pOc);
	}

	QuadTree* QuadTreeNode::GetTree()
	{
		return m_pQuadTree;
	}

	const AABB &QuadTreeNode::GetRegion()
	{
		return m_region;
	}
}