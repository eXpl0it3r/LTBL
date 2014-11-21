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

#ifndef LIGHT_H
#define LIGHT_H

#include "SFML_OpenGL.h"
#include "Constructs.h"
#include "QuadTree.h"

namespace ltbl
{
	class Light :
		public qdt::QuadTreeOccupant
	{
	private:
		sf::RenderTexture* pStaticTexture;

		bool alwaysUpdate;

		sf::RenderWindow* pWin;

		// Set up viewport information for the render texture
		void SwitchStaticTexture();

		bool updateRequired;

	protected:
		class LightSystem* pLightSystem;

		// Set to false in base classes in order to avoid shader attenuation
		bool shaderAttenuation;

	public:
		Vec2f center;
		float intensity;
		float radius;
		float size;

		// If using light autenuation shader
		float bleed;
		float linearizeFactor;

		Color3f color;

		Light();
		~Light();

		void SetRadius(float Radius);
		void IncRadius(float increment);
		float GetRadius();

		void SetCenter(Vec2f Center);
		void IncCenter(Vec2f increment);
		Vec2f GetCenter();

		virtual void RenderLightSolidPortion(float depth) = 0;
		virtual void RenderLightSoftPortion(float depth) = 0;
		virtual void CalculateAABB();
		AABB* GetAABB();

		bool AlwaysUpdate();
		void SetAlwaysUpdate(bool always);

		friend class LightSystem;
	};
}

#endif