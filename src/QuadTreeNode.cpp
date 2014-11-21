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

#include "QuadTreeNode.h"
#include "QuadTree.h"
#include <assert.h>

#include "SFML_OpenGL.h"

using namespace qdt;

QuadTreeNode::QuadTreeNode(const AABB &newRegion, unsigned int numLevels, QuadTreeNode* pParent, QuadTree* pContainer)
	: region(newRegion), hasChildren(false), numOccupants(0),
	pParentNode(pParent), pQuadTree(pContainer), level(numLevels)
{
	center = region.GetCenter();
}

QuadTreeNode::~QuadTreeNode()
{
	if(hasChildren)
		DestroyChildren();
}

void QuadTreeNode::Merge()
{
	// Merge all children into this node
	if(hasChildren)
	{
		for(unsigned int x = 0; x < 2; x++)
			for(unsigned int y = 0; y < 2; y++)
				children[x][y]->GetOccupants(occupants, this);

		DestroyChildren();
		hasChildren = false;
	}
}

void QuadTreeNode::GetOccupants(std::unordered_set<QuadTreeOccupant*> &upperOccupants, QuadTreeNode* newNode)
{
	// Assign the new node pointers while adding the occupants to the upper node
	for(std::unordered_set<QuadTreeOccupant*>::iterator it = occupants.begin(); it != occupants.end(); it++)
	{
		(*it)->pQuadTreeNode = newNode;
		upperOccupants.insert(*it);
	}

	// Recusively go through children if there are any
	if(hasChildren)
		for(unsigned int x = 0; x < 2; x++)
			for(unsigned int y = 0; y < 2; y++)
				children[x][y]->GetOccupants(upperOccupants, newNode);
}

void QuadTreeNode::GetOccupants(std::vector<QuadTreeOccupant*> &queryResult)
{
	// Assign the new node pointers while adding the occupants to the upper node
	for(std::unordered_set<QuadTreeOccupant*>::iterator it = occupants.begin(); it != occupants.end(); it++)
		queryResult.push_back(*it);

	// Recusively go through children if there are any
	if(hasChildren)
		for(unsigned int x = 0; x < 2; x++)
			for(unsigned int y = 0; y < 2; y++)
				children[x][y]->GetOccupants(queryResult);
}

void QuadTreeNode::Partition()
{
	// Create the children nodes with the appropriate bounds set
	Vec2f halfWidth = region.GetDims() / 2.0f;

	const unsigned int nextLevel = level + 1;

	for(unsigned int x = 0; x < 2; x++)
		for(unsigned int y = 0; y < 2; y++)
		{
			children[x][y] = new QuadTreeNode(AABB(Vec2f(region.lowerBound.x + x * halfWidth.x, region.lowerBound.y + y * halfWidth.y),
				Vec2f(center.x + x * halfWidth.x, center.y + y * halfWidth.y)), nextLevel, this, pQuadTree);

			// Oversize
			children[x][y]->region.SetDims(children[x][y]->region.GetDims() * oversizedMultiplier);
		}

	hasChildren = true;
}

void QuadTreeNode::DestroyChildren()
{
	for(unsigned int x = 0; x < 2; x++)
		for(unsigned int y = 0; y < 2; y++)
			delete children[x][y];
}

Point2i QuadTreeNode::GetPossibleOccupantPos(QuadTreeOccupant* pOc)
{
	Point2i pos;

	// NOTE: The center position of the pOc->aabb is not tested, instead a corner is.
	// The center point is not required because if a corner is not in the
	// same partition as the center then it won't fit in any partition anyways.

	if(pOc->aabb.lowerBound.x > center.x)
		pos.x = 1;
	else
		pos.x = 0;

	if(pOc->aabb.lowerBound.y > center.y)
		pos.y = 1;
	else
		pos.y = 0;

	return pos;
}

void QuadTreeNode::AddOccupant(QuadTreeOccupant* pOc)
{
	numOccupants++;

	// See if the new occupant fits into any of
	// the children if this node has children
	if(hasChildren)
	{
		// Add the occupant to a child which contains it
		Point2i pos = GetPossibleOccupantPos(pOc);

		if(children[pos.x][pos.y]->region.Contains(pOc->aabb))
		{
			// Fits into this child node, to continue
			// the adding process from there
			children[pos.x][pos.y]->AddOccupant(pOc);

			return;
		}
	}
	else
	{
		// See if there is enough room still left in this node
		if(occupants.size() + 1 <= maximumOccupants || level > maxLevels)
		{
			// Add to this node's set
			occupants.insert(pOc);

			// Set the occupant's quad tree pointer to this node
			pOc->pQuadTreeNode = this;
			pOc->pQuadTree = pQuadTree;
		}
		else
		{
			// Doesn't fit here, so create a new partition
			Partition();

			// Add the occupant to a child which contains it
			Point2i pos = GetPossibleOccupantPos(pOc);

			if(children[pos.x][pos.y]->region.Contains(pOc->aabb))
			{
				// Fits into this child node, to continue
				// the adding process from there
				children[pos.x][pos.y]->AddOccupant(pOc);

				return;
			}
		}
	}

	// Previous tests failed, so add the occupant this node (even if it goes over the normal maximum occupant count)
	occupants.insert(pOc);

	// Set the occupant's quad tree pointer to this node
	pOc->pQuadTreeNode = this;
	pOc->pQuadTree = pQuadTree;
}

void QuadTreeNode::Query(const AABB &queryRegion, std::vector<QuadTreeOccupant*> &queryResult)
{
	// See if this region is visible
	if(region.Intersects(queryRegion))
	{
		// Add the occupants of this node to the array and then parse the children
		for(std::unordered_set<QuadTreeOccupant*>::iterator it = occupants.begin(); it != occupants.end(); it++)
			if((*it)->aabb.Intersects(queryRegion))
				queryResult.push_back(*it);

		if(hasChildren)
		{
			for(unsigned int x = 0; x < 2; x++)
				for(unsigned int y = 0; y < 2; y++)
					children[x][y]->Query(queryRegion, queryResult);
		}
	}
}

void QuadTreeNode::QueryToDepth(const AABB &queryRegion, std::vector<QuadTreeOccupant*> &queryResult, int depth)
{
	if(depth == 0)
	{
		GetOccupants(queryResult);

		return;
	}

	// See if this region is visible
	if(region.Intersects(queryRegion))
	{
		// Add the occupants of this node to the array and then parse the children
		for(std::unordered_set<QuadTreeOccupant*>::iterator it = occupants.begin(); it != occupants.end(); it++)
			if((*it)->aabb.Intersects(queryRegion))
				queryResult.push_back(*it);

		if(hasChildren)
		{
			for(unsigned int x = 0; x < 2; x++)
				for(unsigned int y = 0; y < 2; y++)
					children[x][y]->QueryToDepth(queryRegion, queryResult, depth - 1);
		}
	}
}

void QuadTreeNode::DebugRender()
{
	// Render the region AABB
	glColor4f(0.7f, 0.1f, 0.5f, 1.0f);

	region.DebugRender();

	glColor4f(0.3f, 0.5f, 0.5f, 1.0f);

	// Render the AABB's of the occupants in this node
	for(std::unordered_set<QuadTreeOccupant*>::iterator it = occupants.begin(); it != occupants.end(); it++)
		(*it)->aabb.DebugRender();

	if(hasChildren)
	{
		// Recursively render all of the AABB's below this node
		for(unsigned int x = 0; x < 2; x++)
			for(unsigned int y = 0; y < 2; y++)
				children[x][y]->DebugRender();
	}
}