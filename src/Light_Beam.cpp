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

#include "Light_Beam.h"

#include "ShadowFin.h"

#include <assert.h>

using namespace ltbl;
using namespace qdt;

Light_Beam::Light_Beam()
	: width(64.0f)
{
}

Light_Beam::~Light_Beam()
{
}

void Light_Beam::SetCenter(const Vec2f &newCenter)
{
	Vec2f diff(newCenter - center);
	center = newCenter;

	// Move all of the quad points
	innerPoint1 += diff;
	innerPoint2 += diff;
	outerPoint1 += diff;
	outerPoint2 += diff;
}

void Light_Beam::UpdateDirectionAngle()
{
	// Recalculate positions of points
	Vec2f innerComponents(cosf(directionAngle), sinf(directionAngle));
	innerPoint1.x = innerComponents.y * width;
	innerPoint1.y = innerComponents.x * width;
	innerPoint2.x = -innerComponents.y * width;
	innerPoint2.y = -innerComponents.x * width;

	Vec2f outerComponents1(cosf(directionAngle + spreadAngle), sinf(directionAngle + spreadAngle));
	outerPoint1.x = innerPoint2.x + outerComponents1.x * radius;
	outerPoint1.y = innerPoint2.y + outerComponents1.y * radius;

	Vec2f outerComponents2(cosf(directionAngle - spreadAngle), sinf(directionAngle - spreadAngle));
	outerPoint2.x = innerPoint1.x + outerComponents2.x * radius;
	outerPoint2.y = innerPoint1.y + outerComponents2.y * radius;
}

void Light_Beam::RenderLightSolidPortion(float depth)
{
	assert(intensity > 0.0f && intensity <= 1.0f);

	float r = color.r * intensity;
	float g = color.g * intensity;
	float b = color.b * intensity;
    
	glBegin(GL_QUADS);

	glColor4f(r, g, b, intensity);
	
	glVertex3f(innerPoint1.x, innerPoint1.y, depth);
	glVertex3f(innerPoint2.x, innerPoint2.y, depth);

	glColor4f(0.0f, 0.0f, 0.0f, 0.0f);

	glVertex3f(outerPoint1.x, outerPoint1.y, depth);
	glVertex3f(outerPoint2.x, outerPoint2.y, depth);

	glEnd();
}

void Light_Beam::RenderLightSoftPortion(float depth)
{
	// If light goes all the way around do not render fins
	if(spreadAngle == 2.0f * M_PI || softSpreadAngle == 0.0f)
		return;

	// Create to shadow fins to mask off a portion of the light
	ShadowFin fin1;

	float penumbraAngle1 = directionAngle - spreadAngle + softSpreadAngle;
	fin1.umbra = outerPoint2 - innerPoint1;
	fin1.penumbra = Vec2f(radius * cosf(penumbraAngle1), radius * sinf(penumbraAngle1));
	fin1.rootPos = innerPoint1;

	fin1.Render(depth);

	ShadowFin fin2;

	float penumbraAngle2 = directionAngle + spreadAngle - softSpreadAngle;
	fin2.umbra = outerPoint1 - innerPoint2;
	fin2.penumbra = Vec2f(radius * cosf(penumbraAngle2), radius * sinf(penumbraAngle2));
	fin2.rootPos = innerPoint2;

	fin2.Render(depth);
}

void Light_Beam::CalculateAABB()
{
	aabb.lowerBound = innerPoint1;
	aabb.upperBound = innerPoint1;

	// Expand
	if(aabb.lowerBound.x > innerPoint2.x)
		aabb.lowerBound.x = innerPoint2.x;
	if(aabb.lowerBound.y > innerPoint2.y)
		aabb.lowerBound.y = innerPoint2.y;
	if(aabb.lowerBound.x > outerPoint1.x)
		aabb.lowerBound.x = outerPoint1.x;
	if(aabb.lowerBound.y > outerPoint1.y)
		aabb.lowerBound.y = outerPoint1.y;
	if(aabb.lowerBound.x > outerPoint2.x)
		aabb.lowerBound.x = outerPoint2.x;
	if(aabb.lowerBound.y > outerPoint2.y)
		aabb.lowerBound.y = outerPoint2.y;

	if(aabb.upperBound.x < innerPoint2.x)
		aabb.upperBound.x = innerPoint2.x;
	if(aabb.upperBound.y < innerPoint2.y)
		aabb.upperBound.y = innerPoint2.y;
	if(aabb.upperBound.x < outerPoint1.x)
		aabb.upperBound.x = outerPoint1.x;
	if(aabb.upperBound.y < outerPoint1.y)
		aabb.upperBound.y = outerPoint1.y;
	if(aabb.upperBound.x < outerPoint2.x)
		aabb.upperBound.x = outerPoint2.x;
	if(aabb.upperBound.y < outerPoint2.y)
		aabb.upperBound.y = outerPoint2.y;

	aabb.CalculateHalfDims();
	aabb.CalculateCenter();
}
