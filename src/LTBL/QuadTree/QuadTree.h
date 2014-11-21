#ifndef QDT_QUADTREE_H
#define QDT_QUADTREE_H

#include <LTBL/QuadTree/QuadTreeNode.h>
#include <LTBL/QuadTree/QuadTreeOccupant.h>

#include <unordered_set>
#include <memory>

namespace qdt
{
	// Base class for dynamic and static QuadTree types
	class QuadTree
	{
	protected:
		std::unordered_set<QuadTreeOccupant*> m_outsideRoot;

		std::unique_ptr<QuadTreeNode> m_pRootNode;

		// Called whenever something is removed, an action can be defined by derived classes
		// Defaults to doing nothing
		virtual void OnRemoval();

		void SetQuadTree(QuadTreeOccupant* pOc);

	public:
		virtual void Add(QuadTreeOccupant* pOc) = 0;

		void Query_Region(const AABB &region, std::vector<QuadTreeOccupant*> &result);

		void DebugRender();

		friend class QuadTreeNode;
		friend class QuadTreeOccupant;
	};
}

#endif