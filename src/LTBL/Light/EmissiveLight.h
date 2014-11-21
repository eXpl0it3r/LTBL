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

#ifndef LTBL_EMISSIVELIGHT_H
#define LTBL_EMISSIVELIGHT_H

#include <LTBL/QuadTree/QuadTree.h>
#include <LTBL/Constructs/Color3f.h>

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ltbl
{
	class EmissiveLight :
		public qdt::QuadTreeOccupant
	{
	private:
		sf::Texture* m_texture;

		float m_angleDegs;

		Vec2f m_halfRenderDims;

	public:
		float m_intensity;

		Color3f m_color;

		EmissiveLight();

		void SetTexture(sf::Texture* texture);
		void Render();

		void SetCenter(const Vec2f &newCenter);
		void SetDims(const Vec2f &newScale);
		void IncCenter(const Vec2f &increment);
		void SetRotation(float angle);
		void IncRotation(float increment);

		Vec2f GetCenter();
		Vec2f GetDims();
		float GetAngle();
	};
}

#endif