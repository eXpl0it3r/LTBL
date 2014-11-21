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

#include <LTBL/Light/EmissiveLight.h>


#define _USE_MATH_DEFINES
#include <math.h>

using namespace ltbl;

EmissiveLight::EmissiveLight()
	: m_angleDegs(0.0f), m_color(1.0f, 1.0f, 1.0f), m_intensity(1.0f)
{
}

void EmissiveLight::SetTexture(sf::Texture* texture)
{
	m_texture = texture;

	// Update aabb and the render dims
	sf::Vector2u textureSize(m_texture->getSize());
	m_halfRenderDims.x = static_cast<float>(textureSize.x) / 2.0f;
	m_halfRenderDims.y = static_cast<float>(textureSize.y) / 2.0f;

	m_aabb.SetHalfDims(m_halfRenderDims);

	m_aabb.CalculateHalfDims();
	m_aabb.CalculateCenter();

	TreeUpdate();
}

void EmissiveLight::Render()
{
	glPushMatrix();

	const Vec2f &center = m_aabb.GetCenter();

	m_texture->bind();

	glTranslatef(center.x, center.y, 0.0f);
	glRotatef(m_angleDegs, 0.0f, 0.0f, 1.0f);

	// Clamp the intensity
	float renderIntensity = m_intensity;

	if(renderIntensity > 1.0f)
		renderIntensity = 1.0f;

	glColor4f(m_color.r, m_color.g, m_color.b, renderIntensity);

	// Have to render upside-down because SFML loads the Textures upside-down
	glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex2f(-m_halfRenderDims.x, -m_halfRenderDims.y);
		glTexCoord2i(1, 0); glVertex2f(m_halfRenderDims.x, -m_halfRenderDims.y);
		glTexCoord2i(1, 1); glVertex2f(m_halfRenderDims.x, m_halfRenderDims.y);
		glTexCoord2i(0, 1); glVertex2f(-m_halfRenderDims.x, m_halfRenderDims.y);
	glEnd();

	// Reset color
	glColor3f(1.0f,	1.0f, 1.0f);

	glPopMatrix();
}

void EmissiveLight::SetCenter(const Vec2f &newCenter)
{
	m_aabb.SetCenter(newCenter);
	TreeUpdate();
}

void EmissiveLight::IncCenter(const Vec2f &increment)
{
	m_aabb.IncCenter(increment);
	TreeUpdate();
}

void EmissiveLight::SetDims(const Vec2f &newDims)
{
	m_halfRenderDims = newDims / 2.0f;

	// Set AABB
	m_aabb.SetHalfDims(m_halfRenderDims);
	
	// Re-rotate AABB if it is rotated
	if(m_angleDegs != 0.0f)
		SetRotation(m_angleDegs);

	TreeUpdate();
}

void EmissiveLight::SetRotation(float angle)
{
	// Update the render angle (convert to degrees)
	m_angleDegs = angle;

	// Get original AABB
	m_aabb.SetHalfDims(m_halfRenderDims);

	m_aabb.CalculateHalfDims();
	m_aabb.CalculateCenter();

	// If angle is the normal angle, exit
	if(m_angleDegs == 0.0f)
		return;

	// Get angle in radians
	float angleRads = m_angleDegs * (static_cast<float>(M_PI) / 180.0f);

	// Rotate the aabb
	m_aabb.SetRotatedAABB(angleRads);

	TreeUpdate();
}

void EmissiveLight::IncRotation(float increment)
{
	// Increment render angle
	m_angleDegs += increment;

	// Get angle increment in radians
	float incrementRads = increment * (static_cast<float>(M_PI) / 180.0f);

	// Rotate the aabb without setting to original aabb, so rotation is relative
	m_aabb.SetRotatedAABB(incrementRads);
}

Vec2f EmissiveLight::GetDims()
{
	return m_aabb.GetDims();
}

Vec2f EmissiveLight::GetCenter()
{
	return m_aabb.GetCenter();
}

float EmissiveLight::GetAngle()
{
	return m_angleDegs;
}