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

#include <LTBL/Light/Light.h>

#include <assert.h>

namespace ltbl
{
	Light::Light()
		: m_intensity(1.0f),
		m_radius(100.0f),
		m_center(0.0f, 0.0f),
		m_color(1.0f, 1.0f, 1.0f),
		m_size(40.0f),
		m_updateRequired(true), m_alwaysUpdate(true), m_pStaticTexture(NULL), // For static light
		m_pWin(NULL), m_pLightSystem(NULL), m_shaderAttenuation(true),
		m_bleed(1.0f), m_linearizeFactor(0.2f)
	{
	}

	Light::~Light()
	{
		// Destroy the static Texture if one exists
		if(m_alwaysUpdate)
			delete m_pStaticTexture;
	}

	AABB* Light::GetAABB()
	{
		return &m_aabb;
	}

	bool Light::AlwaysUpdate()
	{
		return m_alwaysUpdate;
	}

	void Light::SetAlwaysUpdate(bool always)
	{
		// Must add to the light system before calling this
		assert(m_pWin != NULL);

		if(!always && m_alwaysUpdate) // If previously set to false, create a render Texture for the static texture
		{
			Vec2f dims(m_aabb.GetDims());

			// Check if large enough textures are supported
			unsigned int maxDim = sf::Texture::getMaximumSize();

			if(maxDim <= dims.x || maxDim <= dims.y)
			{
				std::cout << "Attempted to create a too large static light. Switching to dynamic." << std::endl;
				return;
			}

			// Create the render Texture based on aabb dims
			m_pStaticTexture = new sf::RenderTexture();

			unsigned int uiDimsX = static_cast<unsigned int>(dims.x);
			unsigned int uiDimsY = static_cast<unsigned int>(dims.y);

			m_pStaticTexture->create(uiDimsX, uiDimsY, false);

			m_pStaticTexture->setSmooth(true);

			m_pStaticTexture->setActive();
			glEnable(GL_BLEND);
			glEnable(GL_TEXTURE_2D);
			m_pWin->setActive();

			m_updateRequired = true;
		}
		else if(always && !m_alwaysUpdate) // If previously set to true, destroy the render Texture
			delete m_pStaticTexture;
	
		m_alwaysUpdate = always;
	}

	void Light::SwitchStaticTexture()
	{
		Vec2f dims(m_aabb.GetDims());

		m_pStaticTexture->setActive();

		glViewport(0, 0, static_cast<unsigned int>(dims.x), static_cast<unsigned int>(dims.y));

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// Flip the projection, since SFML draws the bound textures (on the FBO) upside down
		glOrtho(0, static_cast<unsigned int>(dims.x), static_cast<unsigned int>(dims.y), 0, -100.0f, 100.0f);
		glMatrixMode(GL_MODELVIEW);
	}

	void Light::SetRadius(float radius)
	{
		assert(m_alwaysUpdate);

		m_radius = radius;

		CalculateAABB();
		TreeUpdate();
	}

	void Light::IncRadius(float increment)
	{
		assert(m_alwaysUpdate);

		m_radius += increment;

		CalculateAABB();
		TreeUpdate();
	}

	float Light::GetRadius()
	{
		return m_radius;
	}

	void Light::SetCenter(Vec2f center)
	{
		Vec2f difference(center - m_center);

		m_center = center;

		m_aabb.IncCenter(difference);

		TreeUpdate();

		m_updateRequired = true;
	}

	void Light::IncCenter(Vec2f increment)
	{
		m_center += increment;

		m_aabb.IncCenter(increment);

		TreeUpdate();

		m_updateRequired = true;
	}

	Vec2f Light::GetCenter()
	{
		return m_center;
	}

	void Light::CalculateAABB()
	{
		m_aabb.SetCenter(m_center);
		m_aabb.SetDims(Vec2f(m_radius, m_radius));
	}
}