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

#include "QuadTree.h"

#include "SFML_OpenGL.h"

using namespace qdt;

QuadTree::QuadTree(const AABB &startRegion)
{
	rootNode = new QuadTreeNode(startRegion, 1, NULL, this);
}

QuadTree::~QuadTree()
{
	delete rootNode;
}

void QuadTree::AddOccupant(QuadTreeOccupant* pOc)
{
	if(rootNode->region.Contains(pOc->aabb)) // If it fits inside the root node
		rootNode->AddOccupant(pOc);
	else // Otherwise, add it to the outside root set
	{
		outsideRoot.insert(pOc);

		// Set the pointers properly
		pOc->pQuadTreeNode = NULL; // Not required unless removing a node and then adding it again
		pOc->pQuadTree = this;
	}
}

void QuadTree::ClearTree(const AABB &newStartRegion)
{
	delete rootNode;
	rootNode = new QuadTreeNode(newStartRegion, 1, NULL, this);

	// Clear ouside root
	outsideRoot.clear();
}

void QuadTree::Query(const AABB &queryRegion, std::vector<QuadTreeOccupant*> &queryResult)
{
	// First parse the occupants outside of the root and
	// add them to the array if the fit in the query region
	for(std::unordered_set<QuadTreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end(); it++)
		if((*it)->aabb.Intersects(queryRegion))
			queryResult.push_back(*it);

	// Then query the tree itself
	rootNode->Query(queryRegion, queryResult);
}

void QuadTree::QueryToDepth(const AABB &queryRegion, std::vector<QuadTreeOccupant*> &queryResult, int depth)
{
	// First parse the occupants outside of the root and
	// add them to the array if the fit in the query region
	for(std::unordered_set<QuadTreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end(); it++)
		if((*it)->aabb.Intersects(queryRegion))
			queryResult.push_back(*it);

	// Then query the tree itself
	rootNode->QueryToDepth(queryRegion, queryResult, depth);
}

unsigned int QuadTree::GetNumOccupants()
{
	return rootNode->numOccupants;
}

AABB QuadTree::GetRootAABB()
{
	return rootNode->region;
}

void QuadTree::DebugRender()
{
	glColor4f(0.1f, 0.6f, 0.4f, 1.0f);

	// Parse all AABB's in the tree and render them
	for(std::unordered_set<QuadTreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end(); it++)
		(*it)->aabb.DebugRender();

	// Render the tree itself
	rootNode->DebugRender();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}