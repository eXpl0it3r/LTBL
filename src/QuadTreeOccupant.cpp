/*
	Let There Be Light
	Copyright (C) 2012 Eric Laukien

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include "QuadTreeOccupant.h"

#include "QuadTreeNode.h"
#include "QuadTree.h"

#include "SFML_OpenGL.h"

#include <assert.h>

#include "Light.h"
#include "ConvexHull.h"

using namespace qdt;

QuadTreeOccupant::QuadTreeOccupant()
	: pQuadTreeNode(NULL), pQuadTree(NULL)
{
}

QuadTreeOccupant::~QuadTreeOccupant()
{
}

void QuadTreeOccupant::UpdateTreeStatus()
{
	if(pQuadTreeNode == NULL)
	{
		// Not in the tree, so see if it fits in the root partition now that the AABB has been changed.
		if(pQuadTree != NULL) // It must not have been added to a tree if this is NULL as well
			if(pQuadTree->rootNode->region.Contains(aabb))
			{
				// Fits, remove it from the outside root set and add it to the root
				pQuadTree->outsideRoot.erase(this);

				pQuadTree->rootNode->AddOccupant(this);
			}
	}
	else
	{
		assert(pQuadTreeNode->numOccupants >= 1);

		// First remove the occupant from the set (may be re-added later, this is not highly
		// optimized, but we use this method for simplicity's sake)
		pQuadTreeNode->occupants.erase(this);

		// See of the occupant still fits
		if(pQuadTreeNode->region.Contains(aabb))
		{
			// Re-add to possibly settle into a new position lower in the tree

			// AddOccupant will raise this number to indicate more occupants than there actually are
			pQuadTreeNode->numOccupants--;
			
			pQuadTreeNode->AddOccupant(this);
		}
		else
		{
			// Doesn't fit in this node anymore, so we will continue going
			// up levels in the tree until it fits. If it doesn't fit anywhere,
			// add ot to the quad tree outside of root set.

			// Check to see if this partition should be destroyed.
			// Then, go through the parents until enough occupants are present and
			// merge everything into that parent
			if(pQuadTreeNode->numOccupants - 1 < minimumOccupants)
			{
				// Move up pNode until we have a partition above the minimum count
				while(pQuadTreeNode->pParentNode != NULL)
				{
					if(pQuadTreeNode->numOccupants - 1 >= minimumOccupants)
						break;

					pQuadTreeNode = pQuadTreeNode->pParentNode;
				}

				pQuadTreeNode->Merge();
			}

			// Now, go up and decrement the occupant counts and search for a new place for the modified occupant
			while(pQuadTreeNode != NULL)
			{
				pQuadTreeNode->numOccupants--;

				// See if this node contains the occupant. If so, add it to that occupant.
				if(pQuadTreeNode->region.Contains(aabb))
				{
					// Add the occupant to this node and break
					pQuadTreeNode->AddOccupant(this);

					return;
				}

				pQuadTreeNode = pQuadTreeNode->pParentNode;
			}

			// If we did not break out of the previous loop, this means that we
			// cannot fit the occupantinto the root node. We must therefore add
			// it to the outside root set using our pointer to the quad tree container.
			assert(pQuadTree != NULL);
		
			pQuadTree->outsideRoot.insert(this);

			// Occupant's parent is already NULL, or else it would not have made it here
			assert(pQuadTreeNode == NULL);
		}
	}
}

void QuadTreeOccupant::RemoveFromTree()
{
	if(pQuadTreeNode != NULL) // If part of a quad tree
	{
		pQuadTreeNode->occupants.erase(this);

		// Doesn't fit in this node anymore, so we will continue going
		// up levels in the tree until it fits. If it doesn't fit anywhere,
		// add ot to the quad tree outside of root set.

		// Check to see if this partition should be destroyed.
		// Then, go through the parents until enough occupants are present and
		// merge everything into that parent
		if(pQuadTreeNode->numOccupants - 1 < minimumOccupants)
		{
			// Move up pNode until we have a partition above the minimum count
			while(pQuadTreeNode->pParentNode != NULL)
			{
				if(pQuadTreeNode->numOccupants - 1 >= minimumOccupants)
					break;

				pQuadTreeNode = pQuadTreeNode->pParentNode;
			}

			pQuadTreeNode->Merge();
		}

		// Decrement the remaining occupant counts
		while(pQuadTreeNode != NULL)
		{
			pQuadTreeNode->numOccupants--;
			pQuadTreeNode = pQuadTreeNode->pParentNode;
		}
	}
	else if(pQuadTree != NULL) // See if still part of a quad tree, indicating that it must be in the outside root set
		pQuadTree->outsideRoot.erase(this);

	pQuadTreeNode = NULL;
	pQuadTree = NULL;
}