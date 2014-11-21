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

#include "Light_Point.h"

#include "ShadowFin.h"

#include <assert.h>

using namespace ltbl;

float ltbl::lightSubdivisionSize = static_cast<float>(M_PI) / 24.0f;

Light_Point::Light_Point()
	: directionAngle(0.0f), spreadAngle(2.0f * static_cast<float>(M_PI)), softSpreadAngle(0.0f)
{
}

Light_Point::~Light_Point()
{
}

void Light_Point::RenderLightSolidPortion(float depth)
{
	float renderIntensity = intensity;

	// Clamp the render intensity
	if(renderIntensity > 1.0f)
		renderIntensity = 1.0f;

	assert(renderIntensity >= 0.0f);

	float r = color.r * intensity;
	float g = color.g * intensity;
	float b = color.b * intensity;

	glBegin(GL_TRIANGLE_FAN);

	glVertex3f(center.x, center.y, depth);
      
	// Set the edge color for rest of shape
	int numSubdivisions = static_cast<int>(spreadAngle / lightSubdivisionSize);
	float startAngle = directionAngle - spreadAngle / 2.0f;
      
	for(int currentSubDivision = 0; currentSubDivision <= numSubdivisions; currentSubDivision++)
	{
		float angle = startAngle + currentSubDivision * lightSubdivisionSize;
		glVertex3f(radius * cosf(angle) + center.x, radius * sinf(angle) + center.y, depth);  
	}

	glEnd();
}

void Light_Point::RenderLightSoftPortion(float depth)
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

void Light_Point::CalculateAABB()
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

void Light_Point::SetDirectionAngle(float DirectionAngle)
{
	assert(AlwaysUpdate());

	directionAngle = DirectionAngle;

	CalculateAABB();
	UpdateTreeStatus();
}

void Light_Point::IncDirectionAngle(float increment)
{
	assert(AlwaysUpdate());

	directionAngle += increment;

	CalculateAABB();
	UpdateTreeStatus();
}

float Light_Point::GetDirectionAngle()
{
	return directionAngle;
}

void Light_Point::SetSpreadAngle(float SpreadAngle)
{
	assert(AlwaysUpdate());

	spreadAngle = SpreadAngle;

	CalculateAABB();
	UpdateTreeStatus();
}

void Light_Point::IncSpreadAngle(float increment)
{
	assert(AlwaysUpdate());

	spreadAngle += increment;

	CalculateAABB();
	UpdateTreeStatus();
}

float Light_Point::GetSpreadAngle()
{
	return spreadAngle;
}