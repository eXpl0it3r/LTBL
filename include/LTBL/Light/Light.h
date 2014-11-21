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

#ifndef LTBL_LIGHT_H
#define LTBL_LIGHT_H

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <LTBL/Constructs.h>
#include <LTBL/QuadTree/QuadTree.h>

namespace ltbl
{
	class Light :
		public qdt::QuadTreeOccupant
	{
	private:
		sf::RenderTexture* m_pStaticTexture;

		bool m_alwaysUpdate;

		sf::RenderWindow* m_pWin;

		// Set up viewport information for the render texture
		void SwitchStaticTexture();

		bool m_updateRequired;

	protected:
		class LightSystem* m_pLightSystem;

		// Set to false in base classes in order to avoid shader attenuation
		bool m_shaderAttenuation;

	public:
		Vec2f m_center;
		float m_intensity;
		float m_radius;
		float m_size;

		// If using light autenuation shader
		float m_bleed;
		float m_linearizeFactor;

		Color3f m_color;

		Light();
		~Light();

		void SetRadius(float radius);
		void IncRadius(float increment);
		float GetRadius();

		void SetCenter(Vec2f center);
		void IncCenter(Vec2f increment);
		Vec2f GetCenter();

		virtual void RenderLightSolidPortion() = 0;
		virtual void RenderLightSoftPortion() = 0;
		virtual void CalculateAABB();
		AABB* GetAABB();

		bool AlwaysUpdate();
		void SetAlwaysUpdate(bool always);

		friend class LightSystem;
	};
}

#endif