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

#include <assert.h>

using namespace ltbl;
using namespace qdt;

Light::Light()
	: intensity(1.0f),
	radius(100.0f),
	center(0.0f, 0.0f),
	color(1.0f, 1.0f, 1.0f),
	size(40.0f),
	updateRequired(true), alwaysUpdate(true), pStaticTexture(NULL), // For static light
	pWin(NULL), pLightSystem(NULL), shaderAttenuation(true),
	bleed(1.0f), linearizeFactor(0.2f)
{
}

Light::~Light()
{
	// Destroy the static Texture if one exists
	if(alwaysUpdate)
		delete pStaticTexture;
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

		pStaticTexture->Create(uiDimsX, uiDimsY, false);

		pStaticTexture->SetSmooth(true);

		updateRequired = true;
	}
	else if(always && !alwaysUpdate) // If previously set to true, destroy the render Texture
		delete pStaticTexture;
	
	alwaysUpdate = always;
}

void Light::SwitchStaticTexture()
{
	Vec2f dims(aabb.GetDims());

	pStaticTexture->SetActive();

	glViewport(0, 0, static_cast<unsigned int>(dims.x), static_cast<unsigned int>(dims.y));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Flip the projection, since SFML draws the bound textures (on the FBO) upside down
	glOrtho(0, static_cast<unsigned int>(dims.x), static_cast<unsigned int>(dims.y), 0, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
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

void Light::CalculateAABB()
{
	aabb.SetCenter(center);
	aabb.SetDims(Vec2f(radius, radius));
}