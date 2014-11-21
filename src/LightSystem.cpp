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

#include "LightSystem.h"

#include "ShadowFin.h"

#include <assert.h>

using namespace ltbl;
using namespace qdt;

LightSystem::LightSystem(const AABB &region, sf::RenderWindow* pRenderWindow, const std::string &finImagePath, const std::string &lightAttenuationShaderPath)
	: ambientColor(55, 55, 55), checkForHullIntersect(true),
	prebuildTimer(0), pWin(pRenderWindow), useBloom(true)
{
	// Load the soft shadows texture
	if(!softShadowTexture.LoadFromFile(finImagePath))
		abort(); // Could not find the texture, abort

	if(!lightAttenuationShader.LoadFromFile(lightAttenuationShaderPath, sf::Shader::Fragment))
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

void LightSystem::SetView(const sf::View &view)
{
	sf::Vector2f viewSize(view.GetSize());
	viewAABB.SetDims(Vec2f(viewSize.x, viewSize.y));
	sf::Vector2f viewCenter(view.GetCenter());

	// Flipped
	viewAABB.SetCenter(Vec2f(viewCenter.x, viewSize.y - viewCenter.y));
}

void LightSystem::CameraSetup()
{
	glLoadIdentity();
	glTranslatef(-viewAABB.lowerBound.x, -viewAABB.lowerBound.y, 0.0f);
}

void LightSystem::MaskShadow(Light* light, ConvexHull* convexHull, bool minPoly, float depth)
{
	// ----------------------------- Determine the Shadow Boundaries -----------------------------

	Vec2f lCenter(light->center);
	float lRadius = light->radius;

	Vec2f hCenter(convexHull->GetWorldCenter());

	const int numVertices = convexHull->vertices.size();

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

		if(centerToBoundry.dot(lightNormal) < 0)
			lightNormal *= -1;

		lightNormal = lightNormal.normalize() * light->size;

		Vec2f L((lCenter - lightNormal) - middle);
                
		if (convexHull->normals[i].dot(L) > 0)
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

	if(centerToBoundry.dot(lightNormal) < 0)
		lightNormal *= -1;

	lightNormal = lightNormal.normalize() * light->size;

	ShadowFin firstFin;

	firstFin.rootPos = firstBoundryPoint;
	firstFin.umbra = firstBoundryPoint - (lCenter + lightNormal);
	firstFin.umbra = firstFin.umbra.normalize() * lRadius;

	firstFin.penumbra = firstBoundryPoint - (lCenter - lightNormal);
	firstFin.penumbra = firstFin.penumbra.normalize() * lRadius;

	ShadowFin secondFin;

	Vec2f secondBoundryPoint = convexHull->GetWorldVertex(secondBoundryIndex);

	lightNormal.x = -(lCenter.y - secondBoundryPoint.y);
	lightNormal.y = lCenter.x - secondBoundryPoint.x;

	centerToBoundry = secondBoundryPoint - hCenter;

	if(centerToBoundry.dot(lightNormal) < 0)
		lightNormal *= -1;

	lightNormal = lightNormal.normalize() * light->size;

	secondFin.rootPos = secondBoundryPoint;
	secondFin.umbra = secondBoundryPoint - (lCenter + lightNormal);
	secondFin.umbra = secondFin.umbra.normalize() * lRadius;
	
	secondFin.penumbra = secondBoundryPoint - (lCenter - lightNormal);
	secondFin.penumbra = secondFin.penumbra.normalize() * lRadius;

	// Store generated fins to render later
	// First two, will always be there
	finsToRender_firstBoundary.push_back(firstFin);
	finsToRender_secondBoundary.push_back(secondFin);

	// Need to get address of fin in array instead of firstFin/secondFin since they are copies, and we want it to be modified directly
	// Can avoid by not creating firstFin and secondFin and instead using finsToRender[0] and finsToRender[1]
	// Also, move the boundary points for rendering depending on the number of hulls added
	firstBoundryIndex = Wrap(firstBoundryIndex + AddExtraFins(*convexHull, finsToRender_firstBoundary, *light, firstBoundryIndex, false), numVertices);

	// Umbra vectors, from last fin added
	Vec2f mainUmbraRoot1 = finsToRender_firstBoundary.back().rootPos;
	Vec2f mainUmbraVec1 = finsToRender_firstBoundary.back().umbra;

	secondBoundryIndex = Wrap(secondBoundryIndex - AddExtraFins(*convexHull, finsToRender_secondBoundary, *light, secondBoundryIndex, true), numVertices);

	// Umbra vectors, from last fin added
	Vec2f mainUmbraRoot2 = finsToRender_secondBoundary.back().rootPos;
	Vec2f mainUmbraVec2 = finsToRender_secondBoundary.back().umbra;

	// ----------------------------- Drawing the umbra -----------------------------

	if(minPoly)
	{
		Vec2f throughCenter((hCenter - lCenter).normalize() * lRadius);

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
			Vec2f endVert((startVert - light->center).normalize() * light->radius + startVert);

			// 2 points for ray in strip
			glVertex3f(startVert.x, startVert.y, depth);
			glVertex3f(endVert.x, endVert.y, depth);
		}

		glVertex3f(mainUmbraRoot2.x, mainUmbraRoot2.y, depth);
		glVertex3f(mainUmbraRoot2.x + mainUmbraVec2.x, mainUmbraRoot2.y + mainUmbraVec2.y, depth);

		glEnd();
	}
}

int LightSystem::AddExtraFins(const ConvexHull &hull, std::vector<ShadowFin> &fins, const Light &light, int boundryIndex, bool wrapCW)
{
	ShadowFin* pFin = &fins.back();

	Vec2f hCenter(hull.GetWorldCenter());

	int secondEdgeIndex;
	int numVertices = static_cast<signed>(hull.vertices.size());

	int i;

	for(i = 0; i < maxFins; i++)
	{	
		if(wrapCW)
			secondEdgeIndex = Wrap(boundryIndex - 1, numVertices);
		else
			secondEdgeIndex = Wrap(boundryIndex + 1, numVertices);

		Vec2f edgeVec((hull.vertices[secondEdgeIndex].position - hull.vertices[boundryIndex].position).normalize());

		Vec2f penNorm(pFin->penumbra.normalize());

		float angle1 = acosf(penNorm.dot(edgeVec));
		float angle2 = acosf(penNorm.dot(pFin->umbra.normalize()));

		if(angle1 >= angle2)
			break; // No intersection, break

		// Change existing fin to attatch to side of hull
		pFin->umbra = edgeVec * light.radius;

		// Calculate a lower fin instensity based on ratio of angles (0 if angles are same, so disappears then
		pFin->umbraBrightness = 1.0f - angle1 / angle2;

		// Add the extra fin
		Vec2f secondBoundryPoint(hull.GetWorldVertex(secondEdgeIndex));

		Vec2f lightNormal(-(light.center.y - secondBoundryPoint.y), light.center.x - secondBoundryPoint.x);

		Vec2f centerToBoundry(secondBoundryPoint - hCenter);

		if(centerToBoundry.dot(lightNormal) < 0)
			lightNormal *= -1;

		lightNormal = lightNormal.normalize() * light.size;

		ShadowFin newFin;

		newFin.rootPos = secondBoundryPoint;
		newFin.umbra = secondBoundryPoint - (light.center + lightNormal);
		newFin.umbra = newFin.umbra.normalize() * light.radius;
		newFin.penumbra = pFin->umbra;

		newFin.penumbraBrightness = pFin->umbraBrightness;

		fins.push_back(newFin);

		pFin = &fins.back();

		boundryIndex = secondEdgeIndex;
	}

	return i;
}

void LightSystem::SetUp(const AABB &region)
{
	// Create the quad trees
	lightTree.reset(new QuadTree(region));
	hullTree.reset(new QuadTree(region));
	emissiveTree.reset(new QuadTree(region));

	// Base RT size off of window resolution
	sf::Vector2u viewSizeui(pWin->GetWidth(), pWin->GetHeight());

	renderTexture.Create(viewSizeui.x, viewSizeui.y, false);
	renderTexture.SetSmooth(true);

	bloomTexture.Create(viewSizeui.x, viewSizeui.y, false);
	bloomTexture.SetSmooth(true);

	lightTemp.Create(viewSizeui.x, viewSizeui.y, false);
	lightTemp.SetSmooth(true);
}

void LightSystem::AddLight(Light* newLight)
{
	newLight->pWin = pWin;
	newLight->pLightSystem = this;
	lights.insert(newLight);
	lightTree->AddOccupant(newLight);
}

void LightSystem::AddConvexHull(ConvexHull* newConvexHull)
{
	convexHulls.insert(newConvexHull);
	hullTree->AddOccupant(newConvexHull);
}

void LightSystem::AddEmissiveLight(EmissiveLight* newEmissiveLight)
{
	emissiveLights.insert(newEmissiveLight);
	emissiveTree->AddOccupant(newEmissiveLight);
}

void LightSystem::RemoveLight(Light* pLight)
{
	std::unordered_set<Light*>::iterator it = lights.find(pLight);

	assert(it != lights.end());

	(*it)->RemoveFromTree();

	lights.erase(it);

	delete pLight;
}

void LightSystem::RemoveConvexHull(ConvexHull* pHull)
{
	std::unordered_set<ConvexHull*>::iterator it = convexHulls.find(pHull);

	assert(it != convexHulls.end());

	(*it)->RemoveFromTree();

	convexHulls.erase(it);

	delete pHull;
}

void LightSystem::RemoveEmissiveLight(EmissiveLight* pEmissiveLight)
{
	std::unordered_set<EmissiveLight*>::iterator it = emissiveLights.find(pEmissiveLight);

	assert(it != emissiveLights.end());

	(*it)->RemoveFromTree();

	emissiveLights.erase(it);

	delete pEmissiveLight;
}

void LightSystem::ClearLights()
{
	// Delete contents
	for(std::unordered_set<Light*>::iterator it = lights.begin(); it != lights.end(); it++)
		delete *it;

	lights.clear();

	if(lightTree.get() != NULL)
		lightTree->ClearTree(AABB(Vec2f(-50.0f, -50.0f), Vec2f(-50.0f, -50.0f)));
}

void LightSystem::ClearConvexHulls()
{
	// Delete contents
	for(std::unordered_set<ConvexHull*>::iterator it = convexHulls.begin(); it != convexHulls.end(); it++)
		delete *it;

	convexHulls.clear();

	if(hullTree.get() != NULL)
		hullTree->ClearTree(AABB(Vec2f(-50.0f, -50.0f), Vec2f(-50.0f, -50.0f)));
}

void LightSystem::ClearEmissiveLights()
{
	// Delete contents
	for(std::unordered_set<EmissiveLight*>::iterator it = emissiveLights.begin(); it != emissiveLights.end(); it++)
		delete *it;

	emissiveLights.clear();

	if(emissiveTree.get() != NULL)
		emissiveTree->ClearTree(AABB(Vec2f(-50.0f, -50.0f), Vec2f(-50.0f, -50.0f)));
}

void LightSystem::SwitchLightTemp()
{
	if(currentRenderTexture != cur_lightTemp)
	{
		lightTemp.SetActive();

		//if(currentRenderTexture == cur_lightStatic)
			SwitchWindowProjection();

		currentRenderTexture = cur_lightTemp;
	}
}

void LightSystem::SwitchMain()
{
	if(currentRenderTexture != cur_main)
	{
		renderTexture.SetActive();

		if(currentRenderTexture == cur_lightStatic)
			SwitchWindowProjection();

		currentRenderTexture = cur_main;
	}
}

void LightSystem::SwitchBloom()
{
	if(currentRenderTexture != cur_bloom)
	{
		bloomTexture.SetActive();

		if(currentRenderTexture == cur_lightStatic)
			SwitchWindowProjection();

		currentRenderTexture = cur_bloom;
	}
}

void LightSystem::SwitchWindow()
{
	pWin->ResetGLStates();
}

void LightSystem::SwitchWindowProjection()
{
	Vec2f viewSize(viewAABB.GetDims());
	sf::Vector2u viewSizeui(static_cast<unsigned int>(viewSize.x), static_cast<unsigned int>(viewSize.y));

	glViewport(0, 0, viewSizeui.x, viewSizeui.y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Upside-down, because SFML is pro like that
	glOrtho(0, viewSize.x, 0, viewSize.y, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
}

void LightSystem::RenderLights()
{
	// So will switch to main render textures from SFML projection
	currentRenderTexture = cur_lightStatic;

	Vec2f viewCenter(viewAABB.GetCenter());
	Vec2f viewSize(viewAABB.GetDims());

	if(useBloom)
	{
		// Clear the bloom texture
		SwitchBloom();
		glLoadIdentity();

		glDisable(GL_TEXTURE_2D);

		bloomTexture.Clear(sf::Color::Transparent);
	
		glColor4f(0.0f, 0.0f, 0.0f, 0.0f);

		glBlendFunc(GL_ONE, GL_ZERO);

		// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts
		glBegin(GL_QUADS);
			glVertex3f(0.0f, 0.0f, 0.0f);
			glVertex3f(viewSize.x, 0.0f, 0.0f);
			glVertex3f(viewSize.x, viewSize.y, 0.0f);
			glVertex3f(0.0f, viewSize.y, 0.0f);
		glEnd();

		glEnable(GL_TEXTURE_2D);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	SwitchMain();
	glLoadIdentity();

	glDisable(GL_TEXTURE_2D);

	renderTexture.Clear(ambientColor);
	
	glColor4b(ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);

	glBlendFunc(GL_ONE, GL_ZERO);

	// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts
	glBegin(GL_QUADS);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(viewSize.x, 0.0f, 0.0f);
		glVertex3f(viewSize.x, viewSize.y, 0.0f);
		glVertex3f(0.0f, viewSize.y, 0.0f);
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnable(GL_TEXTURE_2D);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Get visible lights
	std::vector<qdt::QuadTreeOccupant*> visibleLights;
	lightTree->Query(viewAABB, visibleLights);

	// Add lights from pre build list if there are any
	if(!lightsToPreBuild.empty())
	{
		if(prebuildTimer < 2)
		{
			prebuildTimer++;

			const unsigned int numLightsToPreBuild = lightsToPreBuild.size();

			for(unsigned int i = 0; i < numLightsToPreBuild; i++)
			{
				lightsToPreBuild[i]->updateRequired = true;
				visibleLights.push_back(lightsToPreBuild[i]);
			}
		}
		else
			lightsToPreBuild.clear();
	}

	const unsigned int numVisibleLights = visibleLights.size();

	for(unsigned int l = 0; l < numVisibleLights; l++)
	{
		Light* pLight = static_cast<Light*>(visibleLights[l]);

		// Skip invisible lights
		if(pLight->intensity == 0.0f)
			continue;

		bool updateRequired = false;

		if(pLight->AlwaysUpdate())
			updateRequired = true;
		else if(pLight->updateRequired)
			updateRequired = true;

		// Get hulls that the light affects
		std::vector<QuadTreeOccupant*> regionHulls;
		hullTree->Query(*pLight->GetAABB(), regionHulls);

		const unsigned int numHulls = regionHulls.size();

		if(!updateRequired)
		{
			// See of any of the hulls need updating
			for(unsigned int h = 0; h < numHulls; h++)
			{
				ConvexHull* pHull = static_cast<ConvexHull*>(regionHulls[h]);

				if(pHull->updateRequired)
				{
					pHull->updateRequired = false;
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

				glLoadIdentity();
				
				lightTemp.Clear(sf::Color::Transparent);

				// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts. MUST clear with full color, with alpha!
				glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

				float lightTempWidth = static_cast<float>(lightTemp.GetWidth());
				float lightTempHeight = static_cast<float>(lightTemp.GetHeight());

				glBlendFunc(GL_ONE, GL_ZERO);

				glBegin(GL_QUADS);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(lightTempWidth, 0.0f, 0.0f);
					glVertex3f(lightTempWidth, lightTempHeight, 0.0f);
					glVertex3f(0.0f, lightTempHeight, 0.0f);
				glEnd();

				CameraSetup();

				// Reset color
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			}
			else
			{
				pLight->SwitchStaticTexture();
				currentRenderTexture = cur_lightStatic;

				pLight->pStaticTexture->Clear(sf::Color::Transparent);
				
				// Clear with quad, since glClear is not working for some reason... it results in very ugly artifacts. MUST clear with full color, with alpha!
				glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

				float staticRenTexWidth = static_cast<float>(pLight->pStaticTexture->GetWidth());
				float staticRenTexHeight =  static_cast<float>(pLight->pStaticTexture->GetHeight());

				glBlendFunc(GL_ONE, GL_ZERO);

				glBegin(GL_QUADS);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(staticRenTexWidth, 0.0f, 0.0f);
					glVertex3f(staticRenTexWidth, staticRenTexHeight, 0.0f);
					glVertex3f(0.0f, staticRenTexHeight, 0.0f);
				glEnd();

				// Reset color
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

				staticTextureOffset = pLight->aabb.GetDims() / 2.0f;

				glLoadIdentity();
				glTranslatef(-pLight->aabb.lowerBound.x, -pLight->aabb.lowerBound.y, 0.0f);
			}

			if(pLight->shaderAttenuation)
			{
				lightAttenuationShader.Bind();

				if(pLight->AlwaysUpdate())
					lightAttenuationShader.SetParameter("lightPos", pLight->center.x - viewAABB.lowerBound.x, pLight->center.y - viewAABB.lowerBound.y);
				else
					lightAttenuationShader.SetParameter("lightPos", pLight->center.x - pLight->aabb.lowerBound.x, pLight->center.y - pLight->aabb.lowerBound.y);

				lightAttenuationShader.SetParameter("lightColor", pLight->color.r, pLight->color.g, pLight->color.b);
				lightAttenuationShader.SetParameter("radius", pLight->radius);
				lightAttenuationShader.SetParameter("bleed", pLight->bleed);
				lightAttenuationShader.SetParameter("linearizeFactor", pLight->linearizeFactor);

				// Render the current light
				pLight->RenderLightSolidPortion(1.0f);

				lightAttenuationShader.Unbind();
			}
			else
				// Render the current light
				pLight->RenderLightSolidPortion(1.0f);

			// Mask off lights
			glBlendFunc(GL_ONE, GL_ZERO);

			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);

			// Color black
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

			if(checkForHullIntersect)
				for(unsigned int h = 0; h < numHulls; h++)
				{
					ConvexHull* pHull = static_cast<ConvexHull*>(regionHulls[h]);

					Vec2f hullToLight(pLight->center - pHull->GetWorldCenter());
					hullToLight = hullToLight.normalize() * pLight->size;

					if(!pHull->PointInsideHull(pLight->center - hullToLight))
						MaskShadow(pLight, pHull, !pHull->renderLightOverHull, 2.0f);
				}
			else
				for(unsigned int h = 0; h < numHulls; h++)
				{
					ConvexHull* pHull = static_cast<ConvexHull*>(regionHulls[h]);

					MaskShadow(pLight, pHull, !pHull->renderLightOverHull, 2.0f);
				}

			// Render the hulls only for the hulls that had
			// there shadows rendered earlier (not out of bounds)
			for(unsigned int h = 0; h < numHulls; h++)
				static_cast<ConvexHull*>(regionHulls[h])->RenderHull(2.0f);

			// Color reset
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			glEnable(GL_BLEND);

			// Render shadow fins, multiply alpha
			glEnable(GL_TEXTURE_2D);

			softShadowTexture.Bind();

			glBlendFunc(GL_DST_COLOR, GL_ZERO);

			for(unsigned int f = 0, numFins = finsToRender_firstBoundary.size(); f < numFins; f++)
				finsToRender_firstBoundary[f].Render(0.0f);

			for(unsigned int f = 0, numFins = finsToRender_secondBoundary.size(); f < numFins; f++)
				finsToRender_secondBoundary[f].Render(0.0f);

			// Soft light angle fins (additional masking)
			pLight->RenderLightSoftPortion(0.0f);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

			// Now render that intermediate render Texture to the main render Texture
			if(pLight->AlwaysUpdate())
			{
				lightTemp.Display();

				SwitchMain();
				glLoadIdentity();

				lightTemp.GetTexture().Bind();

				glBlendFunc(GL_ONE, GL_ONE);

				// Texture is upside-down for some reason, so draw flipped
				glBegin(GL_QUADS);
					glTexCoord2i(0, 1); glVertex3f(0.0f, 0.0f, 0.0f);
					glTexCoord2i(1, 1); glVertex3f(viewSize.x, 0.0f, 0.0f);
					glTexCoord2i(1, 0); glVertex3f(viewSize.x, viewSize.y, 0.0f);
					glTexCoord2i(0, 0); glVertex3f(0.0f, viewSize.y, 0.0f);
				glEnd();

				// Bloom render
				if(useBloom && pLight->intensity > 1.0f)
				{
					SwitchBloom();
					glLoadIdentity();

					lightTemp.GetTexture().Bind();

					glBlendFunc(GL_ONE, GL_ONE);
	
					// Bloom amount
					glColor4f(1.0f, 1.0f, 1.0f, pLight->intensity - 1.0f);

					// Texture is upside-down for some reason, so draw flipped
					glBegin(GL_QUADS);
						glTexCoord2i(0, 1); glVertex3f(0.0f, 0.0f, 0.0f);
						glTexCoord2i(1, 1); glVertex3f(viewSize.x, 0.0f, 0.0f);
						glTexCoord2i(1, 0); glVertex3f(viewSize.x, viewSize.y, 0.0f);
						glTexCoord2i(0, 0); glVertex3f(0.0f, viewSize.y, 0.0f);
					glEnd();
				}
			}
			else
			{
				pLight->pStaticTexture->Display();

				SwitchMain();
				CameraSetup();

				pLight->pStaticTexture->GetTexture().Bind();

				glTranslatef(pLight->center.x - staticTextureOffset.x, pLight->center.y - staticTextureOffset.y, 0.0f);

				glBlendFunc(GL_ONE, GL_ONE);

				const float lightStaticTextureWidth = static_cast<float>(pLight->pStaticTexture->GetWidth());
				const float lightStaticTextureHeight = static_cast<float>(pLight->pStaticTexture->GetHeight());

				glBegin(GL_QUADS);
					glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, 0.0f);
					glTexCoord2i(1, 0); glVertex3f(lightStaticTextureWidth, 0.0f, 0.0f);
					glTexCoord2i(1, 1); glVertex3f(lightStaticTextureWidth, lightStaticTextureHeight, 0.0f);
					glTexCoord2i(0, 1); glVertex3f(0.0f, lightStaticTextureHeight, 0.0f);
				glEnd();

				// Bloom render
				if(useBloom && pLight->intensity > 1.0f)
				{
					SwitchBloom();
					CameraSetup();

					glTranslatef(pLight->center.x - staticTextureOffset.x, pLight->center.y - staticTextureOffset.y, 0.0f);

					pLight->pStaticTexture->GetTexture().Bind();

					glBlendFunc(GL_ONE, GL_ONE);
	
					// Bloom amount
					glColor4f(1.0f, 1.0f, 1.0f, pLight->intensity - 1.0f);

					glBegin(GL_QUADS);
						glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, 0.0f);
						glTexCoord2i(1, 0); glVertex3f(lightStaticTextureWidth, 0.0f, 0.0f);
						glTexCoord2i(1, 1); glVertex3f(lightStaticTextureWidth, lightStaticTextureHeight, 0.0f);
						glTexCoord2i(0, 1); glVertex3f(0.0f, lightStaticTextureHeight, 0.0f);
					glEnd();
				}
			}

			pLight->updateRequired = false;
		}
		else
		{
			// Render existing texture
			pLight->pStaticTexture->GetTexture().Bind();

			Vec2f staticTextureOffset = pLight->center - pLight->aabb.lowerBound;

			CameraSetup();
			glTranslatef(pLight->center.x, pLight->center.y, 0.0f);

			glBlendFunc(GL_ONE, GL_ONE);

			const float lightStaticTextureWidth = static_cast<float>(pLight->pStaticTexture->GetWidth());
			const float lightStaticTextureHeight = static_cast<float>(pLight->pStaticTexture->GetHeight());

			glBegin(GL_QUADS);
				glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, 0.0f);
				glTexCoord2i(1, 0); glVertex3f(lightStaticTextureWidth, 0.0f, 0.0f);
				glTexCoord2i(1, 1); glVertex3f(lightStaticTextureWidth, lightStaticTextureHeight, 0.0f);
				glTexCoord2i(0, 1); glVertex3f(0.0f, lightStaticTextureHeight, 0.0f);
			glEnd();

			// Bloom render
			if(useBloom && pLight->intensity > 1.0f)
			{
				SwitchBloom();
				CameraSetup();

				glTranslatef(pLight->center.x - staticTextureOffset.x, pLight->center.y - staticTextureOffset.y, 0.0f);

				pLight->pStaticTexture->GetTexture().Bind();

				glBlendFunc(GL_ONE, GL_ONE);
	
				// Bloom amount
				glColor4f(1.0f, 1.0f, 1.0f, pLight->intensity - 1.0f);

				glBegin(GL_QUADS);
					glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, 0.0f);
					glTexCoord2i(1, 0); glVertex3f(lightStaticTextureWidth, 0.0f, 0.0f);
					glTexCoord2i(1, 1); glVertex3f(lightStaticTextureWidth, lightStaticTextureHeight, 0.0f);
					glTexCoord2i(0, 1); glVertex3f(0.0f, lightStaticTextureHeight, 0.0f);
				glEnd();
			}
		}

		regionHulls.clear();
		finsToRender_firstBoundary.clear();
		finsToRender_secondBoundary.clear();
	}

	// Emissive lights
	std::vector<qdt::QuadTreeOccupant*> visibleEmissiveLights;
	emissiveTree->Query(viewAABB, visibleEmissiveLights);

	const unsigned int numEmissiveLights = visibleEmissiveLights.size();

	for(unsigned int i = 0; i < numEmissiveLights; i++)
	{
		EmissiveLight* pEmissive = static_cast<EmissiveLight*>(visibleEmissiveLights[i]);

		if(useBloom && pEmissive->intensity > 1.0f)
		{
			SwitchBloom();
			CameraSetup();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			pEmissive->Render();
		}
		else
		{
			SwitchMain();
			CameraSetup();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			pEmissive->Render();
		}
	}

	renderTexture.Display();

	SwitchWindow();
}

void LightSystem::BuildLight(Light* pLight)
{
	lightsToPreBuild.push_back(pLight);
}

void LightSystem::RenderLightTexture(float renderDepth)
{
	Vec2f viewSize(viewAABB.GetDims());

	// Translate by negative camera coordinates. glLoadIdentity will not work, probably
	// because SFML stores view transformations in the projection matrix
	glTranslatef(viewAABB.GetLowerBound().x, -viewAABB.GetLowerBound().y, 0.0f);

	renderTexture.GetTexture().Bind();

	// Set up color function to multiply the existing color with the render texture color
	glBlendFunc(GL_DST_COLOR, GL_ZERO); // Seperate allows you to set color and alpha functions seperately

	glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, renderDepth);
		glTexCoord2i(1, 0); glVertex3f(viewSize.x, 0.0f, renderDepth);
		glTexCoord2i(1, 1); glVertex3f(viewSize.x, viewSize.y, renderDepth);
		glTexCoord2i(0, 1); glVertex3f(0.0f, viewSize.y, renderDepth);
	glEnd();

	if(useBloom)
	{
		bloomTexture.GetTexture().Bind();

		glBlendFunc(GL_ONE, GL_ONE);

		glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, renderDepth);
			glTexCoord2i(1, 0); glVertex3f(viewSize.x, 0.0f, renderDepth);
			glTexCoord2i(1, 1); glVertex3f(viewSize.x, viewSize.y, renderDepth);
			glTexCoord2i(0, 1); glVertex3f(0.0f, viewSize.y, renderDepth);
		glEnd();
	}

	// Reset blend function
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	pWin->ResetGLStates();
}

void LightSystem::DebugRender()
{
	// Set to a more useful-for-OpenGL projection
	Vec2f viewSize(viewAABB.GetDims());

	glLoadIdentity();

	CameraSetup();

	// Render all trees
	lightTree.get()->DebugRender();
	emissiveTree.get()->DebugRender();
	hullTree.get()->DebugRender();

	glLoadIdentity();

	// Reset to SFML's projection
	pWin->ResetGLStates();
}