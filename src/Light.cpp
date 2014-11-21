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

#include "Light.h"

#include "ShadowFin.h"

#include <assert.h>

using namespace ltbl;
using namespace qdt;

Light::Light()
	: intensity(1.0f),
	radius(100.0f),
	center(0.0f, 0.0f),
	color(1.0f, 1.0f, 1.0f),
	size(40.0f),
	directionAngle(0.0f), spreadAngle(2.0f * static_cast<float>(M_PI)), softSpreadAngle(static_cast<float>(M_PI) / 24.0f),
	updateRequired(true), alwaysUpdate(true), pStaticTexture(NULL), // For static light
	pWin(NULL)
{
	aabb.SetCenter(center);
	aabb.SetDims(Vec2f(radius, radius));
}

Light::~Light()
{
	// Destroy the static Texture if one exists
	if(alwaysUpdate)
		delete pStaticTexture;
}

void Light::RenderLightSolidPortion(float depth)
{
	assert(intensity > 0.0f && intensity <= 1.0f);

	float r = color.r * intensity;
	float g = color.g * intensity;
	float b = color.b * intensity;
    
	glBegin(GL_TRIANGLE_FAN);

	glColor4f(r, g, b, intensity);

	glVertex3f(center.x, center.y, depth);
      
	// Set the edge color for rest of shape
	glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
      
	int numSubdivisions = static_cast<int>(spreadAngle / lightSubdivisionSize);
	float startAngle = directionAngle - spreadAngle / 2.0f;
      
	for(int currentSubDivision = 0; currentSubDivision <= numSubdivisions; currentSubDivision++)
	{
		float angle = startAngle + currentSubDivision * lightSubdivisionSize;
		glVertex3f(radius * cosf(angle) + center.x, radius * sinf(angle) + center.y, depth);  
	}

	glEnd();
}

void Light::RenderLightSoftPortion(float depth)
{
	// If light goes all the way around do not render fins
	if(spreadAngle == 2.0f * M_PI || softSpreadAngle == 0.0f)
		return;

	// Create to shadow fins to mask off a portion of the light
	ShadowFin fin1;

	float umbraAngle1 = directionAngle - spreadAngle / 2.0f;
	float penumbraAngle1 = umbraAngle1 + softSpreadAngle;
	fin1.penumbra = Vec2f(radius * cosf(penumbraAngle1), radius * sinf(penumbraAngle1));
	fin1.umbra = Vec2f(radius * cosf(umbraAngle1), radius * sinf(umbraAngle1));
	fin1.rootPos = center;

	fin1.Render(depth);

	ShadowFin fin2;

	float umbraAngle2 = directionAngle + spreadAngle / 2.0f;
	float penumbraAngle2 = umbraAngle2 - softSpreadAngle;
	fin2.penumbra = Vec2f(radius * cosf(penumbraAngle2), radius * sinf(penumbraAngle2));
	fin2.umbra = Vec2f(radius * cosf(umbraAngle2), radius * sinf(umbraAngle2));
	fin2.rootPos = center;
	
	fin2.Render(depth);
}

void Light::CalculateAABB()
{
	if(spreadAngle == 2.0f * static_cast<float>(M_PI) || spreadAngle == 0.0f) // 1 real value, 1 flag
	{
		Vec2f diff(radius, radius);
		aabb.lowerBound = center - diff;
		aabb.upperBound = center + diff;
	}
	else // Not a true frustum test, but puts the frustum in an aabb, better than nothing
	{
		aabb.lowerBound = center;
		aabb.upperBound = center;

		float halfSpread = spreadAngle / 2.0f;
		float angle1 = directionAngle + halfSpread;
		float angle2 = directionAngle - halfSpread;

		Vec2f frustVec1 = Vec2f(cosf(angle1) * radius, sinf(angle1) * radius);
		Vec2f frustVec2 = Vec2f(cosf(angle2) * radius, sinf(angle2) * radius);

		float dist = (frustVec1 + (frustVec2 - frustVec1) / 2).magnitude();

		float lengthMultiplier = radius / dist;

		frustVec1 *= lengthMultiplier;
		frustVec2 *= lengthMultiplier;

		Vec2f frustumPos1 = center + frustVec1;
		Vec2f frustumPos2 = center + frustVec2;

		// Expand lower bound
		if(frustumPos1.x < aabb.lowerBound.x)
			aabb.lowerBound.x = frustumPos1.x;

		if(frustumPos1.y < aabb.lowerBound.y)
			aabb.lowerBound.y = frustumPos1.y;

		if(frustumPos2.x < aabb.lowerBound.x)
			aabb.lowerBound.x = frustumPos2.x;

		if(frustumPos2.y < aabb.lowerBound.y)
			aabb.lowerBound.y = frustumPos2.y;

		// Expand upper bound
		if(frustumPos1.x > aabb.upperBound.x)
			aabb.upperBound.x = frustumPos1.x;

		if(frustumPos1.y > aabb.upperBound.y)
			aabb.upperBound.y = frustumPos1.y;

		if(frustumPos2.x > aabb.upperBound.x)
			aabb.upperBound.x = frustumPos2.x;

		if(frustumPos2.y > aabb.upperBound.y)
			aabb.upperBound.y = frustumPos2.y;
	}

	aabb.CalculateHalfDims();
	aabb.CalculateCenter();
}

AABB* Light::GetAABB()
{
	return &aabb;
}

bool Light::AlwaysUpdate()
{
	return alwaysUpdate;
}

void Light::SetAlwaysUpdate(bool always)
{
	// Must add to the light system before calling this
	assert(pWin != NULL);

	if(!always && alwaysUpdate) // If previously set to false, create a render Texture for the static texture
	{
		Vec2f dims = aabb.GetDims();

		// Check if large enough textures are supported
		unsigned int maxDim = sf::Texture::GetMaximumSize();

		if(maxDim <= dims.x || maxDim <= dims.y)
		{
			std::cout << "Attempted to create a too large static light. Switching to dynamic." << std::endl;
			return;
		}

		// Create the render Texture based on aabb dims
		pStaticTexture = new sf::RenderTexture();

		unsigned int uiDimsX = static_cast<unsigned int>(dims.x);
		unsigned int uiDimsY = static_cast<unsigned int>(dims.y);

		pStaticTexture->Create(uiDimsX, uiDimsY, true);

		pStaticTexture->SetSmooth(true);

		updateRequired = true;
	}
	else if(always && !alwaysUpdate) // If previously set to true, destroy the render Texture
		delete pStaticTexture;
	
	alwaysUpdate = always;
}

void Light::SwitchStaticTexture()
{
	Vec2f dims = aabb.GetDims();

	pStaticTexture->SetActive();

	glViewport(0, 0, static_cast<unsigned int>(dims.x), static_cast<unsigned int>(dims.y));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Flip the projection, since SFML draws the bound textures (on the FBO) upside down
	glOrtho(0, static_cast<unsigned int>(dims.x), static_cast<unsigned int>(dims.y), 0, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);
}

void Light::SetRadius(float Radius)
{
	assert(alwaysUpdate);

	radius = Radius;

	CalculateAABB();
	UpdateTreeStatus();
}

void Light::IncRadius(float increment)
{
	assert(alwaysUpdate);

	radius += increment;

	CalculateAABB();
	UpdateTreeStatus();
}

float Light::GetRadius()
{
	return radius;
}

void Light::SetCenter(Vec2f Center)
{
	Vec2f difference(Center - center);

	center = Center;

	aabb.IncCenter(difference);

	UpdateTreeStatus();

	updateRequired = true;
}

void Light::IncCenter(Vec2f increment)
{
	center += increment;

	aabb.IncCenter(increment);

	UpdateTreeStatus();

	updateRequired = true;
}

Vec2f Light::GetCenter()
{
	return center;
}

void Light::SetDirectionAngle(float DirectionAngle)
{
	assert(alwaysUpdate);

	directionAngle = DirectionAngle;

	CalculateAABB();
	UpdateTreeStatus();
}

void Light::IncDirectionAngle(float increment)
{
	assert(alwaysUpdate);

	directionAngle += increment;

	CalculateAABB();
	UpdateTreeStatus();
}

float Light::GetDirectionAngle()
{
	return directionAngle;
}

void Light::SetSpreadAngle(float SpreadAngle)
{
	assert(alwaysUpdate);

	spreadAngle = SpreadAngle;

	CalculateAABB();
	UpdateTreeStatus();
}

void Light::IncSpreadAngle(float increment)
{
	assert(alwaysUpdate);

	spreadAngle += increment;

	CalculateAABB();
	UpdateTreeStatus();
}

float Light::GetSpreadAngle()
{
	return spreadAngle;
}