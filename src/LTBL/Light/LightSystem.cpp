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

#include <LTBL/QuadTree/QuadTreeOccupant.h>
#include <LTBL/Light/LightSystem.h>
#include <LTBL/Light/ShadowFin.h>
#include <LTBL/Utils.h>

#include <assert.h>

namespace ltbl
{
	LightSystem::LightSystem()
		: m_ambientColor(55, 55, 55), m_checkForHullIntersect(true),
		m_prebuildTimer(0), m_useBloom(true), m_maxFins(1)
	{
	}

	LightSystem::LightSystem(const AABB &region, sf::RenderWindow* pRenderWindow, const std::string &finImagePath, const std::string &lightAttenuationShaderPath)
		: m_ambientColor(55, 55, 55), m_checkForHullIntersect(true),
		m_prebuildTimer(0), m_pWin(pRenderWindow), m_useBloom(true), m_maxFins(1)
	{
		// Load the soft shadows texture
		if(!m_softShadowTexture.loadFromFile(finImagePath))
			abort(); // Could not find the texture, abort

		if(!m_lightAttenuationShader.loadFromFile(lightAttenuationShaderPath, sf::Shader::Fragment))
			abort();

		SetUp(region);
	}

	LightSystem::~LightSystem()
	{
		// Destroy resources
		ClearLights();
		ClearConvexHulls();
		ClearEmissiveLights();
	}

	void LightSystem::Create(const AABB &region, sf::RenderWindow* pRenderWindow, const std::string &finImagePath, const std::string &lightAttenuationShaderPath)
	{
		m_pWin = pRenderWindow;

		// Load the soft shadows texture
		if(!m_softShadowTexture.loadFromFile(finImagePath))
			abort(); // Could not find the texture, abort

		if(!m_lightAttenuationShader.loadFromFile(lightAttenuationShaderPath, sf::Shader::Fragment))
			abort();

		SetUp(region);
	}

	void LightSystem::SetView(const sf::View &view)
	{
		sf::Vector2f viewSize(view.getSize());
		m_viewAABB.SetDims(Vec2f(viewSize.x, viewSize.y));
		sf::Vector2f viewCenter(view.getCenter());

		// Flipped
		m_viewAABB.SetCenter(Vec2f(viewCenter.x, viewSize.y - viewCenter.y));
	}

	void LightSystem::CameraSetup()
	{
		glLoadIdentity();
		glTranslatef(-m_viewAABB.m_lowerBound.x, -m_viewAABB.m_lowerBound.y, 0.0f);
	}

	void LightSystem::MaskShadow(Light* light, ConvexHull* convexHull, bool minPoly, float depth)
	{
		// ----------------------------- Determine the Shadow Boundaries -----------------------------

		Vec2f lCenter(light->m_center);
		float lRadius = light->m_radius;

		Vec2f hCenter(convexHull->GetWorldCenter());

		const int numVertices = convexHull->m_vertices.size();

		std::vector<bool> backFacing(numVertices);

		for(int i = 0; i < numVertices; i++)
		{
			Vec2f firstVertex(convexHull->GetWorldVertex(i));
			int secondIndex = (i + 1) % numVertices;
			Vec2f secondVertex(convexHull->GetWorldVertex(secondIndex));
			Vec2f middle((firstVertex + secondVertex) / 2.0f);

			// Use normal to take light width into account, this eliminates popping
			Vec2f lightNormal(-(lCenter.y - middle.y), lCenter.x - middle.x);

			Vec2f centerToBoundry(middle - hCenter);

			if(centerToBoundry.Dot(lightNormal) < 0)
				lightNormal *= -1;

			lightNormal = lightNormal.Normalize() * light->m_size;

			Vec2f L((lCenter - lightNormal) - middle);
                
			if (convexHull->m_normals[i].Dot(L) > 0)
				backFacing[i] = false;
			else
				backFacing[i] = true;
		}

		int firstBoundryIndex = 0;
		int secondBoundryIndex = 0;

		for(int currentEdge = 0; currentEdge < numVertices; currentEdge++)
		{
			int nextEdge = (currentEdge + 1) % numVertices;

			if (backFacing[currentEdge] && !backFacing[nextEdge])
				firstBoundryIndex = nextEdge;

			if (!backFacing[currentEdge] && backFacing[nextEdge])
				secondBoundryIndex = nextEdge;
		}

		// -------------------------------- Shadow Fins --------------------------------

		Vec2f firstBoundryPoint(convexHull->GetWorldVertex(firstBoundryIndex));

		Vec2f lightNormal(-(lCenter.y - firstBoundryPoint.y), lCenter.x - firstBoundryPoint.x);

		Vec2f centerToBoundry(firstBoundryPoint - hCenter);

		if(centerToBoundry.Dot(lightNormal) < 0)
			lightNormal *= -1;

		lightNormal = lightNormal.Normalize() * light->m_size;

		ShadowFin firstFin;

		firstFin.m_rootPos = firstBoundryPoint;
		firstFin.m_umbra = firstBoundryPoint - (lCenter + lightNormal);
		firstFin.m_umbra = firstFin.m_umbra.Normalize() * lRadius;

		firstFin.m_penumbra = firstBoundryPoint - (lCenter - lightNormal);
		firstFin.m_penumbra = firstFin.m_penumbra.Normalize() * lRadius;

		ShadowFin secondFin;

		Vec2f secondBoundryPoint = convexHull->GetWorldVertex(secondBoundryIndex);

		lightNormal.x = -(lCenter.y - secondBoundryPoint.y);
		lightNormal.y = lCenter.x - secondBoundryPoint.x;

		centerToBoundry = secondBoundryPoint - hCenter;

		if(centerToBoundry.Dot(lightNormal) < 0)
			lightNormal *= -1;

		lightNormal = lightNormal.Normalize() * light->m_size;

		secondFin.m_rootPos = secondBoundryPoint;
		secondFin.m_umbra = secondBoundryPoint - (lCenter + lightNormal);
		secondFin.m_umbra = secondFin.m_umbra.Normalize() * lRadius;
	
		secondFin.m_penumbra = secondBoundryPoint - (lCenter - lightNormal);
		secondFin.m_penumbra = secondFin.m_penumbra.Normalize() * lRadius;

		std::vector<ShadowFin> finsToRender_firstBoundary;
		std::vector<ShadowFin> finsToRender_secondBoundary;

		// Store generated fins to render later
		// First two, will always be there
		finsToRender_firstBoundary.push_back(firstFin);
		finsToRender_secondBoundary.push_back(secondFin);

		// Need to get address of fin in array instead of firstFin/secondFin since they are copies, and we want it to be modified directly
		// Can avoid by not creating firstFin and secondFin and instead using finsToRender[0] and finsToRender[1]
		// Also, move the boundary points for rendering depending on the number of hulls added
		Vec2f mainUmbraRoot1;
		Vec2f mainUmbraVec1;

		firstBoundryIndex = Wrap(firstBoundryIndex + AddExtraFins(*convexHull, finsToRender_firstBoundary, *light, firstBoundryIndex, false, mainUmbraRoot1, mainUmbraVec1), numVertices);

		Vec2f mainUmbraRoot2;
		Vec2f mainUmbraVec2;

		secondBoundryIndex = Wrap(secondBoundryIndex - AddExtraFins(*convexHull, finsToRender_secondBoundary, *light, secondBoundryIndex, true, mainUmbraRoot2, mainUmbraVec2), numVertices);

		// ----------------------------- Drawing the umbra -----------------------------

		glDisable(GL_TEXTURE_2D);

		glBlendFunc(GL_ZERO, GL_SRC_ALPHA);

		glColor4f(0.0f, 0.0f, 0.0f, 1.0f - convexHull->m_transparency);

		if(!convexHull->m_renderLightOverHull)
		{
			Vec2f throughCenter((hCenter - lCenter).Normalize() * lRadius);

			// 3 rays all the time, less polygons
			glBegin(GL_TRIANGLE_STRIP);

			glVertex3f(mainUmbraRoot1.x, mainUmbraRoot1.y, depth);
			glVertex3f(mainUmbraRoot1.x + mainUmbraVec1.x, mainUmbraRoot1.y + mainUmbraVec1.y, depth);
			glVertex3f(hCenter.x, hCenter.y, depth);
			glVertex3f(hCenter.x + throughCenter.x, hCenter.y + throughCenter.y, depth);
			glVertex3f(mainUmbraRoot2.x, mainUmbraRoot2.y, depth);
			glVertex3f(mainUmbraRoot2.x + mainUmbraVec2.x, mainUmbraRoot2.y + mainUmbraVec2.y, depth);

			glEnd();
		}
		else
		{
			glBegin(GL_TRIANGLE_STRIP);

			// Umbra and penumbra sides done separately, since they do not follow light rays
			glVertex3f(mainUmbraRoot1.x, mainUmbraRoot1.y, depth);
			glVertex3f(mainUmbraRoot1.x + mainUmbraVec1.x, mainUmbraRoot1.y + mainUmbraVec1.y, depth);

			int endV; 

			if(firstBoundryIndex < secondBoundryIndex)
				endV = secondBoundryIndex - 1;
			else
				endV = secondBoundryIndex + numVertices - 1;

			// Mask off around the hull, requires more polygons
			for(int v = firstBoundryIndex + 1; v <= endV; v++)
			{
				// Get actual vertex
				int vi = v % numVertices;

				Vec2f startVert(convexHull->GetWorldVertex(vi));
				Vec2f endVert((startVert - light->m_center).Normalize() * light->m_radius + startVert);

				// 2 points for ray in strip
				glVertex3f(startVert.x, startVert.y, depth);
				glVertex3f(endVert.x, endVert.y, depth);
			}

			glVertex3f(mainUmbraRoot2.x, mainUmbraRoot2.y, depth);
			glVertex3f(mainUmbraRoot2.x + mainUmbraVec2.x, mainUmbraRoot2.y + mainUmbraVec2.y, depth);

			glEnd();
		}

		// Render shadow fins
		glEnable(GL_TEXTURE_2D);

		m_softShadowTexture.bind();

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		for(unsigned int f = 0, numFins = finsToRender_firstBoundary.size(); f < numFins; f++)
			finsToRender_firstBoundary[f].Render(convexHull->m_transparency);

		for(unsigned int f = 0, numFins = finsToRender_secondBoundary.size(); f < numFins; f++)
			finsToRender_secondBoundary[f].Render(convexHull->m_transparency);
	}

	int LightSystem::AddExtraFins(const ConvexHull &hull, std::vector<ShadowFin> &fins, const Light &light, int boundryIndex, bool wrapCW, Vec2f &mainUmbraRoot, Vec2f &mainUmbraVec)
	{
		ShadowFin* pFin = &fins.back();

		Vec2f hCenter(hull.GetWorldCenter());

		int secondEdgeIndex;
		int numVertices = static_cast<signed>(hull.m_vertices.size());

		mainUmbraRoot = fins.back().m_rootPos;
		mainUmbraVec = fins.back().m_umbra;

		unsigned int i;

		for(i = 0; i < m_maxFins; i++)
		{	
			if(wrapCW)
				secondEdgeIndex = Wrap(boundryIndex - 1, numVertices);
			else
				secondEdgeIndex = Wrap(boundryIndex + 1, numVertices);

			Vec2f edgeVec((hull.m_vertices[secondEdgeIndex] - hull.m_vertices[boundryIndex]).Normalize());

			Vec2f penNorm(pFin->m_penumbra.Normalize());

			float angle1 = acosf(penNorm.Dot(edgeVec));
			float angle2 = acosf(penNorm.Dot(pFin->m_umbra.Normalize()));

			if(angle1 >= angle2)
				break; // No intersection, break

			// Change existing fin to attatch to side of hull
			pFin->m_umbra = edgeVec * light.m_radius;

			// Calculate a lower fin instensity based on ratio of angles (0 if angles are same, so disappears then)
			pFin->m_umbraBrightness = 1.0f - angle1 / angle2;

			// Add the extra fin
			Vec2f secondBoundryPoint(hull.GetWorldVertex(secondEdgeIndex));

			Vec2f lightNormal(-(light.m_center.y - secondBoundryPoint.y), light.m_center.x - secondBoundryPoint.x);

			Vec2f centerToBoundry(secondBoundryPoint - hCenter);

			if(centerToBoundry.Dot(lightNormal) < 0)
				lightNormal *= -1;

			lightNormal = lightNormal.Normalize() * light.m_size;

			ShadowFin newFin;

			mainUmbraRoot = newFin.m_rootPos = secondBoundryPoint;
			newFin.m_umbra = secondBoundryPoint - (light.m_center + lightNormal);
			mainUmbraVec = newFin.m_umbra = newFin.m_umbra.Normalize() * light.m_radius;
			newFin.m_penumbra = pFin->m_umbra;

			newFin.m_penumbraBrightness = pFin->m_umbraBrightness;

			fins.push_back(newFin);

			pFin = &fins.back();

			boundryIndex = secondEdgeIndex;
		}

		return i;
	}

	void LightSystem::SetUp(const AABB &region)
	{
		// Create the quad trees
		m_lightTree.Create(region);
		m_hullTree.Create(region);
		m_emissiveTree.Create(region);

		// Base RT size off of window resolution
		sf::Vector2u viewSizeui(m_pWin->getSize());

		m_compositionTexture.create(viewSizeui.x, viewSizeui.y, false);
		m_compositionTexture.setSmooth(true);

		m_compositionTexture.setActive();
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

		m_bloomTexture.create(viewSizeui.x, viewSizeui.y, false);
		m_bloomTexture.setSmooth(true);

		m_bloomTexture.setActive();
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

		m_lightTempTexture.create(viewSizeui.x, viewSizeui.y, false);
		m_lightTempTexture.setSmooth(true);

		m_lightTempTexture.setActive();
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

		m_pWin->setActive();
	}

	void LightSystem::AddLight(Light* newLight)
	{
		newLight->m_pWin = m_pWin;
		newLight->m_pLightSystem = this;
		m_lights.insert(newLight);
		m_lightTree.Add(newLight);
	}

	void LightSystem::AddConvexHull(ConvexHull* newConvexHull)
	{
		m_convexHulls.insert(newConvexHull);
		m_hullTree.Add(newConvexHull);
	}

	void LightSystem::AddEmissiveLight(EmissiveLight* newEmissiveLight)
	{
		m_emissiveLights.insert(newEmissiveLight);
		m_emissiveTree.Add(newEmissiveLight);
	}

	void LightSystem::RemoveLight(Light* pLight)
	{
		std::unordered_set<Light*>::iterator it = m_lights.find(pLight);

		assert(it != m_lights.end());

		(*it)->RemoveFromTree();

		m_lights.erase(it);

		delete pLight;
	}

	void LightSystem::RemoveConvexHull(ConvexHull* pHull)
	{
		std::unordered_set<ConvexHull*>::iterator it = m_convexHulls.find(pHull);

		assert(it != m_convexHulls.end());

		(*it)->RemoveFromTree();

		m_convexHulls.erase(it);

		delete pHull;
	}

	void LightSystem::RemoveEmissiveLight(EmissiveLight* pEmissiveLight)
	{
		std::unordered_set<EmissiveLight*>::iterator it = m_emissiveLights.find(pEmissiveLight);

		assert(it != m_emissiveLights.end());

		(*it)->RemoveFromTree();

		m_emissiveLights.erase(it);

		delete pEmissiveLight;
	}

	void LightSystem::ClearLights()
	{
		// Delete contents
		for(std::unordered_set<Light*>::iterator it = m_lights.begin(); it != m_lights.end(); it++)
			delete *it;

		m_lights.clear();

		if(m_lightTree.Created())
		{
			m_lightTree.Clear();
			m_lightTree.Create(AABB(Vec2f(-50.0f, -50.0f), Vec2f(-50.0f, -50.0f)));
		}
	}

	void LightSystem::ClearConvexHulls()
	{
		// Delete contents
		for(std::unordered_set<ConvexHull*>::iterator it = m_convexHulls.begin(); it != m_convexHulls.end(); it++)
			delete *it;

		m_convexHulls.clear();

		if(!m_hullTree.Created())
		{
			m_hullTree.Clear();
			m_hullTree.Create(AABB(Vec2f(-50.0f, -50.0f), Vec2f(-50.0f, -50.0f)));
		}
	}

	void LightSystem::ClearEmissiveLights()
	{
		// Delete contents
		for(std::unordered_set<EmissiveLight*>::iterator it = m_emissiveLights.begin(); it != m_emissiveLights.end(); it++)
			delete *it;

		m_emissiveLights.clear();

		if(m_emissiveTree.Created())
		{
			m_emissiveTree.Clear();
			m_emissiveTree.Create(AABB(Vec2f(-50.0f, -50.0f), Vec2f(-50.0f, -50.0f)));
		}
	}

	void LightSystem::SwitchLightTemp()
	{
		if(m_currentRenderTexture != cur_lightTemp)
		{
			m_lightTempTexture.setActive();

			//if(currentRenderTexture == cur_lightStatic)
				SwitchWindowProjection();

			m_currentRenderTexture = cur_lightTemp;
		}
	}

	void LightSystem::SwitchComposition()
	{
		if(m_currentRenderTexture != cur_main)
		{
			m_compositionTexture.setActive();

			if(m_currentRenderTexture == cur_lightStatic)
				SwitchWindowProjection();

			m_currentRenderTexture = cur_main;
		}
	}

	void LightSystem::SwitchBloom()
	{
		if(m_currentRenderTexture != cur_bloom)
		{
			m_bloomTexture.setActive();

			if(m_currentRenderTexture == cur_lightStatic)
				SwitchWindowProjection();

			m_currentRenderTexture = cur_bloom;
		}
	}

	void LightSystem::SwitchWindow()
	{
		m_pWin->resetGLStates();
	}

	void LightSystem::SwitchWindowProjection()
	{
		Vec2f viewSize(m_viewAABB.GetDims());
		sf::Vector2u viewSizeui(static_cast<unsigned int>(viewSize.x), static_cast<unsigned int>(viewSize.y));

		glViewport(0, 0, viewSizeui.x, viewSizeui.y);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// Upside-down, because SFML is pro like that
		glOrtho(0, viewSize.x, 0, viewSize.y, -100.0f, 100.0f);
		glMatrixMode(GL_MODELVIEW);
	}

	void LightSystem::ClearLightTexture(sf::RenderTexture &renTex)
	{	
		glLoadIdentity();
				
		renTex.clear(sf::Color::Transparent);

		// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts. MUST clear with full color, with alpha!
		glColor4f(0.0f, 0.0f, 0.0f, 0.0f);

		sf::Vector2u size(renTex.getSize());
		float width = static_cast<float>(size.x);
		float height = static_cast<float>(size.y);

		glBlendFunc(GL_ONE, GL_ZERO);

		glBegin(GL_QUADS);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(width, 0.0f);
			glVertex2f(width, height);
			glVertex2f(0.0f, height);
		glEnd();
	}

	void LightSystem::RenderLights()
	{
		// So will switch to main render textures from SFML projection
		m_currentRenderTexture = cur_lightStatic;

		Vec2f viewCenter(m_viewAABB.GetCenter());
		Vec2f viewSize(m_viewAABB.GetDims());

		glDisable(GL_TEXTURE_2D);

		if(m_useBloom)
		{
			// Clear the bloom texture
			SwitchBloom();
			glLoadIdentity();

			m_bloomTexture.clear(sf::Color::Transparent);

			glColor4f(0.0f, 0.0f, 0.0f, 0.0f);

			glBlendFunc(GL_ONE, GL_ZERO);

			// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts
			glBegin(GL_QUADS);
				glVertex2f(0.0f, 0.0f);
				glVertex2f(viewSize.x, 0.0f);
				glVertex2f(viewSize.x, viewSize.y);
				glVertex2f(0.0f, viewSize.y);
			glEnd();

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}

		SwitchComposition();
		glLoadIdentity();

		m_compositionTexture.clear(m_ambientColor);

		glColor4b(m_ambientColor.r, m_ambientColor.g, m_ambientColor.b, m_ambientColor.a);

		glBlendFunc(GL_ONE, GL_ZERO);

		// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts
		glBegin(GL_QUADS);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(viewSize.x, 0.0f);
			glVertex2f(viewSize.x, viewSize.y);
			glVertex2f(0.0f, viewSize.y);
		glEnd();

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glEnable(GL_TEXTURE_2D);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Get visible lights
		std::vector<qdt::QuadTreeOccupant*> visibleLights;
		m_lightTree.Query_Region(m_viewAABB, visibleLights);

		// Add lights from pre build list if there are any
		if(!m_lightsToPreBuild.empty())
		{
			if(m_prebuildTimer < 2)
			{
				m_prebuildTimer++;

				const unsigned int numLightsToPreBuild = m_lightsToPreBuild.size();

				for(unsigned int i = 0; i < numLightsToPreBuild; i++)
				{
					m_lightsToPreBuild[i]->m_updateRequired = true;
					visibleLights.push_back(m_lightsToPreBuild[i]);
				}
			}
			else
				m_lightsToPreBuild.clear();
		}

		const unsigned int numVisibleLights = visibleLights.size();

		for(unsigned int l = 0; l < numVisibleLights; l++)
		{
			Light* pLight = static_cast<Light*>(visibleLights[l]);

			// Skip invisible lights
			if(pLight->m_intensity == 0.0f)
				continue;

			bool updateRequired = false;

			if(pLight->AlwaysUpdate())
				updateRequired = true;
			else if(pLight->m_updateRequired)
				updateRequired = true;

			// Get hulls that the light affects
			std::vector<qdt::QuadTreeOccupant*> regionHulls;
			m_hullTree.Query_Region(*pLight->GetAABB(), regionHulls);

			const unsigned int numHulls = regionHulls.size();

			if(!updateRequired)
			{
				// See of any of the hulls need updating
				for(unsigned int h = 0; h < numHulls; h++)
				{
					ConvexHull* pHull = static_cast<ConvexHull*>(regionHulls[h]);

					if(pHull->m_updateRequired)
					{
						pHull->m_updateRequired = false;
						updateRequired = true;
						break;
					}
				}
			}

			if(updateRequired)
			{
				Vec2f staticTextureOffset;

				// Activate the intermediate render Texture
				if(pLight->AlwaysUpdate())
				{
					SwitchLightTemp();

					ClearLightTexture(m_lightTempTexture);

					CameraSetup();

					// Reset color
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				}
				else
				{
					pLight->SwitchStaticTexture();
					m_currentRenderTexture = cur_lightStatic;

					ClearLightTexture(*pLight->m_pStaticTexture);

					// Reset color
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

					staticTextureOffset = pLight->m_aabb.GetDims() / 2.0f;

					glTranslatef(-pLight->m_aabb.m_lowerBound.x, -pLight->m_aabb.m_lowerBound.y, 0.0f);
				}

				if(pLight->m_shaderAttenuation)
				{
					m_lightAttenuationShader.bind();

					if(pLight->AlwaysUpdate())
						m_lightAttenuationShader.setParameter("lightPos", pLight->m_center.x - m_viewAABB.m_lowerBound.x, pLight->m_center.y - m_viewAABB.m_lowerBound.y);
					else
						m_lightAttenuationShader.setParameter("lightPos", pLight->m_center.x - pLight->m_aabb.m_lowerBound.x, pLight->m_center.y - pLight->m_aabb.m_lowerBound.y);

					m_lightAttenuationShader.setParameter("lightColor", pLight->m_color.r, pLight->m_color.g, pLight->m_color.b);
					m_lightAttenuationShader.setParameter("radius", pLight->m_radius);
					m_lightAttenuationShader.setParameter("bleed", pLight->m_bleed);
					m_lightAttenuationShader.setParameter("linearizeFactor", pLight->m_linearizeFactor);

					// Render the current light
					pLight->RenderLightSolidPortion();

					m_lightAttenuationShader.unbind();
				}
				else
					// Render the current light
					pLight->RenderLightSolidPortion();

				// Mask off lights
				if(m_checkForHullIntersect)
					for(unsigned int h = 0; h < numHulls; h++)
					{
						ConvexHull* pHull = static_cast<ConvexHull*>(regionHulls[h]);

						Vec2f hullToLight(pLight->m_center - pHull->GetWorldCenter());
						hullToLight = hullToLight.Normalize() * pLight->m_size;

						if(!pHull->PointInsideHull(pLight->m_center - hullToLight))
							MaskShadow(pLight, pHull, !pHull->m_renderLightOverHull, 2.0f);
					}
				else
					for(unsigned int h = 0; h < numHulls; h++)
					{
						ConvexHull* pHull = static_cast<ConvexHull*>(regionHulls[h]);

						MaskShadow(pLight, pHull, !pHull->m_renderLightOverHull, 2.0f);
					}

				// Render the hulls only for the hulls that had
				// there shadows rendered earlier (not out of bounds)
				for(unsigned int h = 0; h < numHulls; h++)
					static_cast<ConvexHull*>(regionHulls[h])->RenderHull(2.0f);

				// Soft light angle fins (additional masking)
				pLight->RenderLightSoftPortion();

				// Color reset
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

				// Now render that intermediate render Texture to the main render Texture
				if(pLight->AlwaysUpdate())
				{
					m_lightTempTexture.display();

					SwitchComposition();
					glLoadIdentity();

					m_lightTempTexture.getTexture().bind();

					glBlendFunc(GL_ONE, GL_ONE);

					// Texture is upside-down for some reason, so draw flipped
					glBegin(GL_QUADS);
						glTexCoord2i(0, 1); glVertex2f(0.0f, 0.0f);
						glTexCoord2i(1, 1); glVertex2f(viewSize.x, 0.0f);
						glTexCoord2i(1, 0); glVertex2f(viewSize.x, viewSize.y);
						glTexCoord2i(0, 0); glVertex2f(0.0f, viewSize.y);
					glEnd();

					// Bloom render
					if(m_useBloom && pLight->m_intensity > 1.0f)
					{
						SwitchBloom();
						glLoadIdentity();

						m_lightTempTexture.getTexture().bind();

						glBlendFunc(GL_ONE, GL_ONE);
	
						// Bloom amount
						glColor4f(1.0f, 1.0f, 1.0f, pLight->m_intensity - 1.0f);

						// Texture is upside-down for some reason, so draw flipped
						glBegin(GL_QUADS);
							glTexCoord2i(0, 1); glVertex2f(0.0f, 0.0f);
							glTexCoord2i(1, 1); glVertex2f(viewSize.x, 0.0f);
							glTexCoord2i(1, 0); glVertex2f(viewSize.x, viewSize.y);
							glTexCoord2i(0, 0); glVertex2f(0.0f, viewSize.y);
						glEnd();

						glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					}
				}
				else
				{
					pLight->m_pStaticTexture->display();

					SwitchComposition();
					CameraSetup();

					pLight->m_pStaticTexture->getTexture().bind();

					glTranslatef(pLight->m_center.x - staticTextureOffset.x, pLight->m_center.y - staticTextureOffset.y, 0.0f);

					glBlendFunc(GL_ONE, GL_ONE);

					sf::Vector2u lightStaticTextureSize(pLight->m_pStaticTexture->getSize());
					const float lightStaticTextureWidth = static_cast<float>(lightStaticTextureSize.x);
					const float lightStaticTextureHeight = static_cast<float>(lightStaticTextureSize.y);

					glBegin(GL_QUADS);
						glTexCoord2i(0, 0); glVertex2f(0.0f, 0.0f);
						glTexCoord2i(1, 0); glVertex2f(lightStaticTextureWidth, 0.0f);
						glTexCoord2i(1, 1); glVertex2f(lightStaticTextureWidth, lightStaticTextureHeight);
						glTexCoord2i(0, 1); glVertex2f(0.0f, lightStaticTextureHeight);
					glEnd();

					// Bloom render
					if(m_useBloom && pLight->m_intensity > 1.0f)
					{
						SwitchBloom();
						CameraSetup();

						glTranslatef(pLight->m_center.x - staticTextureOffset.x, pLight->m_center.y - staticTextureOffset.y, 0.0f);

						pLight->m_pStaticTexture->getTexture().bind();

						glBlendFunc(GL_ONE, GL_ONE);
	
						// Bloom amount
						glColor4f(1.0f, 1.0f, 1.0f, pLight->m_intensity - 1.0f);

						glBegin(GL_QUADS);
							glTexCoord2i(0, 0); glVertex2f(0.0f, 0.0f);
							glTexCoord2i(1, 0); glVertex2f(lightStaticTextureWidth, 0.0f);
							glTexCoord2i(1, 1); glVertex2f(lightStaticTextureWidth, lightStaticTextureHeight);
							glTexCoord2i(0, 1); glVertex2f(0.0f, lightStaticTextureHeight);
						glEnd();

						glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					}
				}

				pLight->m_updateRequired = false;
			}
			else
			{
				SwitchComposition();

				// Render existing texture
				pLight->m_pStaticTexture->getTexture().bind();

				Vec2f staticTextureOffset(pLight->m_center - pLight->m_aabb.m_lowerBound);

				CameraSetup();
				glTranslatef(pLight->m_center.x - staticTextureOffset.x, pLight->m_center.y - staticTextureOffset.y, 0.0f);

				glBlendFunc(GL_ONE, GL_ONE);

				sf::Vector2u lightStaticTextureSize(pLight->m_pStaticTexture->getSize());
				const float lightStaticTextureWidth = static_cast<float>(lightStaticTextureSize.x);
				const float lightStaticTextureHeight = static_cast<float>(lightStaticTextureSize.y);

				glBegin(GL_QUADS);
					glTexCoord2i(0, 0); glVertex2f(0.0f, 0.0f);
					glTexCoord2i(1, 0); glVertex2f(lightStaticTextureWidth, 0.0f);
					glTexCoord2i(1, 1); glVertex2f(lightStaticTextureWidth, lightStaticTextureHeight);
					glTexCoord2i(0, 1); glVertex2f(0.0f, lightStaticTextureHeight);
				glEnd();

				// Bloom render
				if(m_useBloom && pLight->m_intensity > 1.0f)
				{
					SwitchBloom();
					CameraSetup();

					glTranslatef(pLight->m_center.x - staticTextureOffset.x, pLight->m_center.y - staticTextureOffset.y, 0.0f);

					pLight->m_pStaticTexture->getTexture().bind();

					glBlendFunc(GL_ONE, GL_ONE);
	
					// Bloom amount
					glColor4f(1.0f, 1.0f, 1.0f, pLight->m_intensity - 1.0f);

					glBegin(GL_QUADS);
						glTexCoord2i(0, 0); glVertex2f(0.0f, 0.0f);
						glTexCoord2i(1, 0); glVertex2f(lightStaticTextureWidth, 0.0f);
						glTexCoord2i(1, 1); glVertex2f(lightStaticTextureWidth, lightStaticTextureHeight);
						glTexCoord2i(0, 1); glVertex2f(0.0f, lightStaticTextureHeight);
					glEnd();

					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}

			regionHulls.clear();
		}

		// Emissive lights
		std::vector<qdt::QuadTreeOccupant*> visibleEmissiveLights;
		m_emissiveTree.Query_Region(m_viewAABB, visibleEmissiveLights);

		const unsigned int numEmissiveLights = visibleEmissiveLights.size();

		for(unsigned int i = 0; i < numEmissiveLights; i++)
		{
			EmissiveLight* pEmissive = static_cast<EmissiveLight*>(visibleEmissiveLights[i]);

			if(m_useBloom && pEmissive->m_intensity > 1.0f)
			{
				SwitchBloom();
				CameraSetup();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				pEmissive->Render();
			}
			else
			{
				SwitchComposition();
				CameraSetup();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				pEmissive->Render();
			}
		}

		m_bloomTexture.display();

		m_compositionTexture.display();

		SwitchWindow();
	}

	void LightSystem::BuildLight(Light* pLight)
	{
		m_lightsToPreBuild.push_back(pLight);
	}

	void LightSystem::RenderLightTexture()
	{
		Vec2f viewSize(m_viewAABB.GetDims());

		// Translate by negative camera coordinates. glLoadIdentity will not work, probably
		// because SFML stores view transformations in the projection matrix
		glTranslatef(m_viewAABB.GetLowerBound().x, -m_viewAABB.GetLowerBound().y, 0.0f);

		m_compositionTexture.getTexture().bind();

		// Set up color function to multiply the existing color with the render texture color
		glBlendFunc(GL_DST_COLOR, GL_ZERO); // Seperate allows you to set color and alpha functions seperately

		glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex2f(0.0f, 0.0f);
			glTexCoord2i(1, 0); glVertex2f(viewSize.x, 0.0f);
			glTexCoord2i(1, 1); glVertex2f(viewSize.x, viewSize.y);
			glTexCoord2i(0, 1); glVertex2f(0.0f, viewSize.y);
		glEnd();

		if(m_useBloom)
		{
			m_bloomTexture.getTexture().bind();

			glBlendFunc(GL_ONE, GL_ONE);

			glBegin(GL_QUADS);
				glTexCoord2i(0, 0); glVertex2f(0.0f, 0.0f);
				glTexCoord2i(1, 0); glVertex2f(viewSize.x, 0.0f);
				glTexCoord2i(1, 1); glVertex2f(viewSize.x, viewSize.y);
				glTexCoord2i(0, 1); glVertex2f(0.0f, viewSize.y);
			glEnd();
		}

		// Reset blend function
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		m_pWin->resetGLStates();
	}

	void LightSystem::DebugRender()
	{
		// Set to a more useful-for-OpenGL projection
		Vec2f viewSize(m_viewAABB.GetDims());

		glLoadIdentity();

		CameraSetup();

		// Render all trees
		m_lightTree.DebugRender();
		m_emissiveTree.DebugRender();
		m_hullTree.DebugRender();

		glLoadIdentity();

		// Reset to SFML's projection
		m_pWin->resetGLStates();
	}
}