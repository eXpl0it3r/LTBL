#include <LTBL/QuadTree/QuadTree.h>

#include <LTBL/QuadTree/QuadTreeNode.h>
#include <LTBL/QuadTree/QuadTree.h>

#include <SFML/OpenGL.hpp>

#include <list>

namespace qdt
{
	void QuadTree::OnRemoval()
	{
	}

	void QuadTree::SetQuadTree(QuadTreeOccupant* pOc)
	{
		pOc->m_pQuadTree = this;
	}

	void QuadTree::Query_Region(const AABB &region, std::vector<QuadTreeOccupant*> &result)
	{
		// Query outside root elements
		for(std::unordered_set<QuadTreeOccupant*>::iterator it = m_outsideRoot.begin(); it != m_outsideRoot.end(); it++)
		{
			QuadTreeOccupant* pOc = *it;

			if(region.Intersects(pOc->m_aabb))
			{
				// Intersects, add to list
				result.push_back(pOc);
			}
		}

		std::list<QuadTreeNode*> open;

		open.push_back(m_pRootNode.get());

		while(!open.empty())
		{
			// Depth-first (results in less memory usage), remove objects from open list
			QuadTreeNode* pCurrent = open.back();
			open.pop_back();

			// Add occupants if they are in the region
			for(std::unordered_set<QuadTreeOccupant*>::iterator it = pCurrent->m_pOccupants.begin(); it != pCurrent->m_pOccupants.end(); it++)
			{
				QuadTreeOccupant* pOc = *it;

				if(region.Intersects(pOc->m_aabb))
				{
					// Intersects, add to list
					result.push_back(pOc);
				}
			}

			// Add children to open list if they intersect the region
			if(pCurrent->m_hasChildren)
			{
				for(int x = 0; x < 2; x++)
					for(int y = 0; y < 2; y++)
					{
						if(region.Intersects((*pCurrent->m_children)[x][y].m_region))
							open.push_back(&(*pCurrent->m_children)[x][y]);
					}
			}
		}
	}

	void QuadTree::DebugRender()
	{
		// Render outside root AABB's
		glColor3f(0.5f, 0.2f, 0.1f);

		for(std::unordered_set<QuadTreeOccupant*>::iterator it = m_outsideRoot.begin(); it != m_outsideRoot.end(); it++)
			(*it)->m_aabb.DebugRender();

		// Now draw the tree
		std::list<QuadTreeNode*> open;

		open.push_back(m_pRootNode.get());

		while(!open.empty())
		{
			// Depth-first (results in less memory usage), remove objects from open list
			QuadTreeNode* pCurrent = open.back();
			open.pop_back();

			// Render node region AABB
			glColor3f(0.4f, 0.9f, 0.7f);

			pCurrent->m_region.DebugRender();

			glColor3f(0.5f, 0.2f, 0.2f);

			// Render occupants
			for(std::unordered_set<QuadTreeOccupant*>::iterator it = pCurrent->m_pOccupants.begin(); it != pCurrent->m_pOccupants.end(); it++)
			{
				QuadTreeOccupant* pOc = *it;

				pOc->m_aabb.DebugRender();
			}

			// Add children to open list if they are visible
			if(pCurrent->m_hasChildren)
			{
				for(int x = 0; x < 2; x++)
					for(int y = 0; y < 2; y++)
						open.push_back(&(*pCurrent->m_children)[x][y]);
			}
		}
	}
}
