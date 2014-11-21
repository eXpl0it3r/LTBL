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

#ifndef LIGHTSYSTEM_H
#define LIGHTSYSTEM_H

#include "Light.h"
#include "EmissiveLight.h"
#include "ConvexHull.h"
#include "ShadowFin.h"
#include "SFML_OpenGL.h"
#include "Constructs.h"
#include <unordered_set>
#include <vector>

namespace ltbl
{
	// The radius value of a light is larger than what one actually 
	// sees in the render of the light. Therefore, this multiplier 
	// can be used to "cut down" the radius a bit more for more culling.
	const float lightRadiusCullMultiplier = 1.0f;
	const float renderDepth = 50.0f;

	const int maxFins = 2;

	class LightSystem
	{
	private:
		sf::RenderWindow* pWin;

		std::unordered_set<Light*> lights;
	
		std::unordered_set<EmissiveLight*> emissiveLights;

		std::unordered_set<ConvexHull*> convexHulls;

		std::vector<Light*> lightsToPreBuild;

		std::unique_ptr<qdt::QuadTree> lightTree;
		std::unique_ptr<qdt::QuadTree> hullTree;
		std::unique_ptr<qdt::QuadTree> emissiveTree;

		sf::RenderTexture renderTexture;
		sf::RenderTexture lightTemp;
		sf::RenderTexture bloomTexture;

		sf::Shader lightAttenuationShader;

		std::vector<ShadowFin> finsToRender;

		sf::Texture softShadowTexture;

		int prebuildTimer;

		void MaskShadow(Light* light, ConvexHull* convexHull, float depth);
		void AddExtraFins(const ConvexHull &hull, ShadowFin* fin, const Light &light, Vec2f &mainUmbra, Vec2f &mainUmbraRoot, int boundryIndex, bool wrapCW);
		void CameraSetup();
		void SetUp(const AABB &region);

		// Switching between render textures
		void SwitchLightTemp();
		void SwitchMain();
		void SwitchBloom();
		void SwitchWindow();

		void SwitchWindowProjection();

		enum CurrentRenderTexture
		{
			cur_lightTemp, cur_main, cur_bloom, cur_window, cur_lightStatic
		} currentRenderTexture;

	public:
		AABB viewAABB;
		sf::Color ambientColor;
		bool checkForHullIntersect;

		bool useBloom;

		LightSystem(const AABB &region, sf::RenderWindow* pRenderWindow);
		~LightSystem();

		void SetView(const sf::View &view);

		// All objects are controller through pointer, but these functions return indices that allow easy removal
		void AddLight(Light* newLight);
		void AddConvexHull(ConvexHull* newConvexHull);
		void AddEmissiveLight(EmissiveLight* newEmissiveLight);

		void RemoveLight(Light* pLight);
		void RemoveConvexHull(ConvexHull* pHull);
		void RemoveEmissiveLight(EmissiveLight* pEmissiveLight);

		// Pre-builds the light
		void BuildLight(Light* pLight);

		void ClearLights();
		void ClearConvexHulls();
		void ClearEmissiveLights();

		// Renders lights to the light texture
		void RenderLights();

		void RenderLightTexture(float renderDepth = 0.0f);

		void DebugRender();
	};
}

#endif
