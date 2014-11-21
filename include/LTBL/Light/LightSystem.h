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

#ifndef LTBL_LIGHTSYSTEM_H
#define LTBL_LIGHTSYSTEM_H

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>

#include <LTBL/QuadTree/StaticQuadTree.h>
#include <LTBL/Light/Light.h>
#include <LTBL/Light/EmissiveLight.h>
#include <LTBL/Light/ConvexHull.h>
#include <LTBL/Light/ShadowFin.h>
#include <LTBL/Constructs.h>

#include <unordered_set>
#include <vector>

namespace ltbl
{
	class LightSystem
	{
	private:
		sf::RenderWindow* m_pWin;

		std::unordered_set<Light*> m_lights;
	
		std::unordered_set<EmissiveLight*> m_emissiveLights;

		std::unordered_set<ConvexHull*> m_convexHulls;

		std::vector<Light*> m_lightsToPreBuild;

		qdt::StaticQuadTree m_lightTree;
		qdt::StaticQuadTree m_hullTree;
		qdt::StaticQuadTree m_emissiveTree;

		sf::RenderTexture m_compositionTexture;
		sf::RenderTexture m_lightTempTexture;
		sf::RenderTexture m_bloomTexture;

		sf::Shader m_lightAttenuationShader;

		sf::Texture m_softShadowTexture;

		int m_prebuildTimer;

		void MaskShadow(Light* light, ConvexHull* convexHull, bool minPoly, float depth);

		// Returns number of fins added
		int AddExtraFins(const ConvexHull &hull, std::vector<ShadowFin> &fins, const Light &light, int boundryIndex, bool wrapCW, Vec2f &mainUmbraRoot, Vec2f &mainUmbraVec);

		void CameraSetup();
		void SetUp(const AABB &region);

		// Switching between render textures
		void SwitchLightTemp();
		void SwitchComposition();
		void SwitchBloom();
		void SwitchWindow();

		void SwitchWindowProjection();

		enum CurrentRenderTexture
		{
			cur_lightTemp, cur_shadow, cur_main, cur_bloom, cur_window, cur_lightStatic
		} m_currentRenderTexture;

		void ClearLightTexture(sf::RenderTexture &renTex);

	public:
		AABB m_viewAABB;

		sf::Color m_ambientColor;

		bool m_checkForHullIntersect;
		bool m_useBloom;

		unsigned int m_maxFins;

		LightSystem();
		LightSystem(const AABB &region, sf::RenderWindow* pRenderWindow, const std::string &finImagePath, const std::string &lightAttenuationShaderPath);
		~LightSystem();

		void Create(const AABB &region, sf::RenderWindow* pRenderWindow, const std::string &finImagePath, const std::string &lightAttenuationShaderPath);

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

		void RenderLightTexture();

		void DebugRender();
	};
}

#endif
