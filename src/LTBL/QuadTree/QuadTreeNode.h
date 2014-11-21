#ifndef QDT_QUADTREENODE_H
#define QDT_QUADTREENODE_H

#include <LTBL/Constructs/AABB.h>
#include <LTBL/Constructs/Point2i.h>
#include <LTBL/QuadTree/QuadTreeOccupant.h>

#include <vector>
#include <unordered_set>
#include <memory>

namespace qdt
{
	class QuadTreeNode
	{
	private:
		class QuadTreeNode* m_pParent;
		class QuadTree* m_pQuadTree;

		// Cannot use a unique_ptr, since the vector requires copy ctor/assignment ops
		std::vector<std::vector<QuadTreeNode>>* m_children;
		bool m_hasChildren;

		std::unordered_set<class QuadTreeOccupant*> m_pOccupants;

		AABB m_region;

		int m_level;

		int m_numOccupantsBelow;

		inline QuadTreeNode* GetChild(const Point2i position);

		void GetPossibleOccupantPosition(QuadTreeOccupant* pOc, Point2i &point);

		void AddToThisLevel(QuadTreeOccupant* pOc);

		// Returns true if occupant was added to children
		bool AddToChildren(QuadTreeOccupant* pOc);

		void GetOccupants(std::unordered_set<QuadTreeOccupant*> &occupants);

		void Partition();
		void DestroyChildren();
		void Merge();

		void Update(QuadTreeOccupant* pOc);
		void Remove(QuadTreeOccupant* pOc);

	public:
		static int minNumOccupants;
		static int maxNumOccupants;
		static int maxNumLevels;

		static float m_oversizeMultiplier;

		QuadTreeNode();
		QuadTreeNode(const AABB &region, int level, QuadTreeNode* pParent = NULL, QuadTree* pQuadTree = NULL);
		~QuadTreeNode();

		// For use after using default constructor
		void Create(const AABB &region, int level, QuadTreeNode* pParent = NULL, QuadTree* pQuadTree = NULL);

		QuadTree* GetTree();

		void Add(QuadTreeOccupant* pOc);

		const AABB &GetRegion();

		void GetAllOccupantsBelow(std::vector<QuadTreeOccupant*> &occupants);

		friend class QuadTreeOccupant;
		friend class QuadTree;
	};
}

#endif