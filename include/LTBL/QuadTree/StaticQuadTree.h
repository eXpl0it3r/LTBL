#ifndef QDT_STATICQUADTREE_H
#define QDT_STATICQUADTREE_H

#include <LTBL/QuadTree/QuadTree.h>

namespace qdt
{
	class StaticQuadTree :
		public QuadTree
	{
	private:
		bool m_created;

	public:
		StaticQuadTree();
		StaticQuadTree(const AABB &rootRegion);

		void Create(const AABB &rootRegion);

		// Inherited from QuadTree
		void Add(QuadTreeOccupant* pOc);

		void Clear();

		bool Created();
	};
}

#endif