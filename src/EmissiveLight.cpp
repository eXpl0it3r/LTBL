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

#include "EmissiveLight.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace ltbl;
using namespace qdt;

EmissiveLight::EmissiveLight()
	: angleDegs(0.0f), color(1.0f, 1.0f, 1.0f)
{
}

void EmissiveLight::SetTexture(sf::Texture* texture)
{
	text = texture;

	// Update aabb and the render dims
	halfRenderDims.x = static_cast<float>(text->GetWidth()) / 2.0f;
	halfRenderDims.y = static_cast<float>(text->GetHeight()) / 2.0f;

	aabb.SetHalfDims(halfRenderDims);

	aabb.CalculateHalfDims();
	aabb.CalculateCenter();

	UpdateTreeStatus();
}

void EmissiveLight::Render()
{
	glPushMatrix();

	const Vec2f &center = aabb.GetCenter();

	text->Bind();

	glTranslatef(center.x, center.y, 0.0f);
	glRotatef(angleDegs, 0.0f, 0.0f, 1.0f);

	glColor3f(color.r, color.g, color.b);

	// Have to render upside-down because SFML loads the Textures upside-down
	glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex3f(-halfRenderDims.x, -halfRenderDims.y, 0.0f);
		glTexCoord2i(1, 0); glVertex3f(halfRenderDims.x, -halfRenderDims.y, 0.0f);
		glTexCoord2i(1, 1); glVertex3f(halfRenderDims.x, halfRenderDims.y, 0.0f);
		glTexCoord2i(0, 1); glVertex3f(-halfRenderDims.x, halfRenderDims.y, 0.0f);
	glEnd();

	// Reset color
	glColor3f(1.0f,	1.0f, 1.0f);

	DrawQuad(*text);

	glPopMatrix();
}

void EmissiveLight::SetCenter(const Vec2f &newCenter)
{
	aabb.SetCenter(newCenter);
	UpdateTreeStatus();
}

void EmissiveLight::IncCenter(const Vec2f &increment)
{
	aabb.IncCenter(increment);
	UpdateTreeStatus();
}

void EmissiveLight::SetDims(const Vec2f &newDims)
{
	halfRenderDims = newDims / 2.0f;

	// Set AABB
	aabb.SetHalfDims(halfRenderDims);
	
	// Re-rotate AABB if it is rotated
	if(angleDegs != 0.0f)
		SetRotation(angleDegs);

	UpdateTreeStatus();
}

void EmissiveLight::SetRotation(float angle)
{
	// Update the render angle (convert to degrees)
	angleDegs = angle;

	// Get original AABB
	aabb.SetHalfDims(halfRenderDims);

	aabb.CalculateHalfDims();
	aabb.CalculateCenter();

	// If angle is the normal angle, exit
	if(angleDegs == 0.0f)
		return;

	// Get angle in radians
	float angleRads = angleDegs * (static_cast<float>(M_PI) / 180.0f);

	// Rotate the aabb
	aabb.SetRotatedAABB(angleRads);

	UpdateTreeStatus();
}

void EmissiveLight::IncRotation(float increment)
{
	// Increment render angle
	angleDegs += increment;

	// Get angle increment in radians
	float incrementRads = increment * (static_cast<float>(M_PI) / 180.0f);

	// Rotate the aabb without setting to original aabb, so rotation is relative
	aabb.SetRotatedAABB(incrementRads);
}

Vec2f EmissiveLight::GetDims()
{
	return aabb.GetDims();
}

Vec2f EmissiveLight::GetCenter()
{
	return aabb.GetCenter();
}

float EmissiveLight::GetAngle()
{
	return angleDegs;
}