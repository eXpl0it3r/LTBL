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

LightSystem::LightSystem(const AABB &region, sf::RenderWindow* pRenderWindow)
	: ambientColor(55, 55, 55), checkForHullIntersect(true),
	prebuildTimer(0), pWin(pRenderWindow)
{
	view.SetCenter(sf::Vector2f(0.0f, 0.0f));
	view.SetSize(sf::Vector2f(static_cast<float>(pRenderWindow->GetWidth()), static_cast<float>(pRenderWindow->GetHeight())));

	// Load the soft shadows texture
	if(!softShadowTexture.LoadFromFile("data/softShadowsTexture.png"))
		abort(); // Could not find the texture, abort

	SetUp(region);
}

LightSystem::~LightSystem()
{
	// Destroy resources
	ClearLights();
	ClearConvexHulls();
	ClearEmissiveLights();
}

void LightSystem::CameraSetup()
{
	sf::Vector2f viewCenter = view.GetCenter();
	glTranslatef(-viewCenter.x, -viewCenter.y, 0.0f);
}

void LightSystem::MaskShadow(Light* light, ConvexHull* convexHull, float depth)
{
	// ----------------------------- Determine the Shadow Boundaries -----------------------------

	Vec2f lCenter = light->center;
	float lRadius = light->radius;

	Vec2f hCenter = convexHull->GetWorldCenter();

	const int numVertices = convexHull->vertices.size();

	std::vector<bool> backFacing(numVertices);

	for(int i = 0; i < numVertices; i++)
    {
		Vec2f firstVertex(convexHull->GetWorldVertex(i));
        int secondIndex = (i + 1) % numVertices;
        Vec2f secondVertex(convexHull->GetWorldVertex(secondIndex));
        Vec2f middle = (firstVertex + secondVertex) / 2.0f;

		// Use normal to take light width into account, this eliminates popping
		Vec2f lightNormal(-(lCenter.y - middle.y), lCenter.x - middle.x);

		Vec2f centerToBoundry = middle - hCenter;

		if(centerToBoundry.dot(lightNormal) < 0)
			lightNormal *= -1;

		lightNormal = lightNormal.normalize() * light->size;

		Vec2f L = (lCenter - lightNormal) - middle;
                
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

	Vec2f firstBoundryPoint = convexHull->GetWorldVertex(firstBoundryIndex);

	Vec2f lightNormal(-(lCenter.y - firstBoundryPoint.y), lCenter.x - firstBoundryPoint.x);

	Vec2f centerToBoundry = firstBoundryPoint - hCenter;

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

	lightNormal = Vec2f(-(lCenter.y - secondBoundryPoint.y), lCenter.x - secondBoundryPoint.x);
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
	finsToRender.push_back(firstFin);
	finsToRender.push_back(secondFin);

	Vec2f mainUmbraRoot1 = firstFin.rootPos;
	Vec2f mainUmbraRoot2 = secondFin.rootPos;
	Vec2f mainUmbraVec1 = firstFin.umbra;
	Vec2f mainUmbraVec2 = secondFin.umbra;

	AddExtraFins(*convexHull, &firstFin, *light, mainUmbraVec1, mainUmbraRoot1, firstBoundryIndex, false);
	AddExtraFins(*convexHull, &secondFin, *light, mainUmbraVec2, mainUmbraRoot2, secondBoundryIndex, true);
	
	// ----------------------------- Drawing the umbra -----------------------------

	Vec2f vertex1 = convexHull->GetWorldVertex(firstBoundryIndex);
	Vec2f vertex2 = convexHull->GetWorldVertex(secondBoundryIndex);

	Vec2f throughCenter = (hCenter - lCenter).normalize() * lRadius;

	// 3 rays is enough in most cases
	glBegin(GL_TRIANGLE_STRIP);

	glVertex3f(mainUmbraRoot1.x, mainUmbraRoot1.y, depth);
	glVertex3f(mainUmbraRoot1.x + mainUmbraVec1.x, mainUmbraRoot1.y + mainUmbraVec1.y, depth);
	glVertex3f(hCenter.x, hCenter.y, depth);
	glVertex3f(hCenter.x + throughCenter.x, hCenter.y + throughCenter.y, depth);
	glVertex3f(mainUmbraRoot2.x, mainUmbraRoot2.y, depth);
	glVertex3f(mainUmbraRoot2.x + mainUmbraVec2.x, mainUmbraRoot2.y + mainUmbraVec2.y, depth);

	glEnd();
}

void LightSystem::AddExtraFins(const ConvexHull &hull, ShadowFin* fin, const Light &light, Vec2f &mainUmbra, Vec2f &mainUmbraRoot, int boundryIndex, bool wrapCW)
{
	Vec2f hCenter = hull.GetWorldCenter();

	int secondEdgeIndex;
	int numVertices = static_cast<signed>(hull.vertices.size());

	for(int i = 0; i < maxFins; i++)
	{	
		if(wrapCW)
			secondEdgeIndex = Wrap(boundryIndex - 1, numVertices);
		else
			secondEdgeIndex = Wrap(boundryIndex + 1, numVertices);

		Vec2f edgeVec = Vec2f(hull.vertices[secondEdgeIndex].position - hull.vertices[boundryIndex].position).normalize();

		Vec2f penNorm(fin->penumbra.normalize());

		float angle1 = acosf(penNorm.dot(edgeVec) / (fin->penumbra.magnitude() * edgeVec.magnitude()));
		float angle2 = acosf(penNorm.dot(fin->umbra) / (fin->penumbra.magnitude() * fin->umbra.magnitude()));

		if(angle1 >= angle2)
			break; // No intersection, break

		// Change existing fin to attatch to side of hull
		fin->umbra = edgeVec * light.radius;

		// Add the extra fin
		Vec2f secondBoundryPoint = hull.GetWorldVertex(secondEdgeIndex);

		Vec2f lightNormal(-(light.center.y - secondBoundryPoint.y), light.center.x - secondBoundryPoint.x);

		Vec2f centerToBoundry = secondBoundryPoint - hCenter;

		if(centerToBoundry.dot(lightNormal) < 0)
			lightNormal *= -1;

		lightNormal = lightNormal.normalize() * light.size;

		ShadowFin newFin;

		newFin.rootPos = secondBoundryPoint;
		newFin.umbra = secondBoundryPoint - (light.center + lightNormal);
		newFin.umbra = newFin.umbra.normalize() * light.radius;
		newFin.penumbra = edgeVec.normalize() * light.radius;

		finsToRender.push_back(newFin);

		fin = &finsToRender.back();

		boundryIndex = secondEdgeIndex;

		break;
	}

	// Change the main umbra to correspond to the last fin
	mainUmbraRoot = fin->rootPos;
	mainUmbra = fin->umbra;
}

void LightSystem::SetUp(const AABB &region)
{
	// Create the quad trees
	lightTree.reset(new QuadTree(region));
	hullTree.reset(new QuadTree(region));
	emissiveTree.reset(new QuadTree(region));

	sf::Vector2f viewSize(view.GetSize());
	sf::Vector2u viewSizeui(static_cast<unsigned int>(viewSize.x), static_cast<unsigned int>(viewSize.y));

	renderTexture.Create(viewSizeui.x, viewSizeui.y, false);
	renderTexture.SetSmooth(true);

	lightTemp.Create(viewSizeui.x, viewSizeui.y, true);
	lightTemp.SetSmooth(true);

	InitGlew();

	// Check to see if advanced blending is supported
	if(!GL_ARB_imaging)
	{
		std::cout << "Advanced blending not supported on this machine!" << std::endl;
		abort();
	}
}

void LightSystem::AddLight(Light* newLight)
{
	newLight->pWin = pWin;
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
	lightTemp.SetActive();

	sf::Vector2f viewSize(view.GetSize());
	sf::Vector2u viewSizeui(static_cast<unsigned int>(viewSize.x), static_cast<unsigned int>(viewSize.y));

	glViewport(0, 0, viewSizeui.x, viewSizeui.y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Flip the projection, since SFML draws the bound textures (on the FBO) upside down
	glOrtho(0, viewSizeui.x, viewSizeui.y, 0, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1.0f);
}

void LightSystem::SwitchMain()
{
	renderTexture.SetActive();

	sf::Vector2f viewSize(view.GetSize());
	sf::Vector2u viewSizeui(static_cast<unsigned int>(viewSize.x), static_cast<unsigned int>(viewSize.y));

	glViewport(0, 0, viewSizeui.x, viewSizeui.y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, viewSizeui.x, 0, viewSizeui.y, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
}

void LightSystem::SwitchWindow()
{
	pWin->ResetGLStates();
}

void LightSystem::RenderLights()
{
	sf::Vector2f viewCenter(view.GetCenter());
	sf::Vector2f viewSize(view.GetSize());
	sf::Vector2u viewSizeui(viewSize);

	// Bind the main render texture on to which the scene is composed
	SwitchMain();

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
	AABB view(Vec2f(viewCenter.x, viewCenter.y),
		Vec2f(viewSize.x + viewCenter.x, viewSize.y + viewCenter.y));

	std::vector<QuadTreeOccupant*> visibleLights;
	lightTree->Query(view, visibleLights);

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
				CameraSetup();
				
				lightTemp.Clear();

				// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts. MUST clear with full color, with alpha!
				//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Not needed, since already this color

				float lightTempWidth = static_cast<float>(lightTemp.GetWidth());
				float lightTempHeight = static_cast<float>(lightTemp.GetHeight());

				glBlendFunc(GL_ONE, GL_ZERO);

				glBegin(GL_QUADS);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(lightTempWidth, 0.0f, 0.0f);
					glVertex3f(lightTempWidth, lightTempHeight, 0.0f);
					glVertex3f(0.0f, lightTempHeight, 0.0f);
				glEnd();
			}
			else
			{
				pLight->SwitchStaticTexture();

				pLight->pStaticTexture->Clear();
				
				// Clear with quad, since glClear is not working for some reason... if results in very ugly artifacts. MUST clear with full color, with alpha!
				//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Not needed, since already this color

				float staticRenTexWidth = static_cast<float>(pLight->pStaticTexture->GetWidth());
				float staticRenTexHeight =  static_cast<float>(pLight->pStaticTexture->GetHeight());

				glBlendFunc(GL_ONE, GL_ZERO);

				glBegin(GL_QUADS);
					glVertex3f(0.0f, 0.0f, 0.0f);
					glVertex3f(staticRenTexWidth, 0.0f, 0.0f);
					glVertex3f(staticRenTexWidth, staticRenTexHeight, 0.0f);
					glVertex3f(0.0f, staticRenTexHeight, 0.0f);
				glEnd();

				staticTextureOffset = pLight->aabb.GetDims() / 2.0f;

				glLoadIdentity();
				glTranslatef(-pLight->aabb.lowerBound.x, -pLight->aabb.lowerBound.y, 0.0f);
			}

			// About to use depth buffer...
			glClear(GL_DEPTH_BUFFER_BIT);

			glBlendFunc(GL_ONE, GL_ZERO);

			glEnable(GL_DEPTH_TEST);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);

			// Disable color and alpha buffer writes temporarily for masking
			glColorMask(false, false, false, false);

			if(checkForHullIntersect)
				for(unsigned int h = 0; h < numHulls; h++)
				{
					ConvexHull* pHull = static_cast<ConvexHull*>(regionHulls[h]);

					Vec2f hullToLight(pLight->center - pHull->GetWorldCenter());
					hullToLight = hullToLight.normalize() * pLight->size;

					if(!pHull->PointInsideHull(pLight->center - hullToLight))
						MaskShadow(pLight, pHull, 2.0f);
				}
			else
				for(unsigned int h = 0; h < numHulls; h++)
					MaskShadow(pLight, static_cast<ConvexHull*>(regionHulls[h]), 2.0f);

			// Render the hulls only for the hulls that had
			// there shadows rendered earlier (not out of bounds)
			for(unsigned int h = 0; h < numHulls; h++)
				static_cast<ConvexHull*>(regionHulls[h])->RenderHull(2.0f);

			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);

			// Re-enable color buffer and alpha buffer writes
			glColorMask(true, true, true, true);

			// Render the current light
			pLight->RenderLightSolidPortion(1.0f);

			// Color reset
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

			// Render shadow fins, multiply alpha
			glEnable(GL_TEXTURE_2D);

			softShadowTexture.Bind();

			glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

			const unsigned int numFins = finsToRender.size();

			for(unsigned int f = 0; f < numFins; f++)
				finsToRender[f].Render(1.0f);

			// Soft light angle fins
			pLight->RenderLightSoftPortion(1.0f);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

			glClear(GL_DEPTH_BUFFER_BIT);

			// Now render that intermediate render Texture to the main render Texture
			if(pLight->AlwaysUpdate())
			{
				lightTemp.Display();

				SwitchMain();

				lightTemp.GetTexture().Bind();

				glDisable(GL_DEPTH_TEST);

				// Save the camera matrix
				glPushMatrix();

				glLoadIdentity();

				glBlendFunc(GL_ONE, GL_ONE);

				glBegin(GL_QUADS);
					glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, 0.0f);
					glTexCoord2i(1, 0); glVertex3f(viewSize.x, 0.0f, 0.0f);
					glTexCoord2i(1, 1); glVertex3f(viewSize.x, viewSize.y, 0.0f);
					glTexCoord2i(0, 1); glVertex3f(0.0f, viewSize.y, 0.0f);
				glEnd();

				glPopMatrix();
			}
			else
			{
				pLight->pStaticTexture->Display();

				SwitchMain();

				pLight->pStaticTexture->GetTexture().Bind();

				glPushMatrix();

				CameraSetup();
				glTranslatef(pLight->center.x - staticTextureOffset.x, pLight->center.y - staticTextureOffset.y, 0.0f);

				glBlendFunc(GL_ONE, GL_ONE);
				glDisable(GL_DEPTH_TEST);

				const float lightStaticTextureWidth = static_cast<float>(pLight->pStaticTexture->GetWidth());
				const float lightStaticTextureHeight = static_cast<float>(pLight->pStaticTexture->GetHeight());

				glBegin(GL_QUADS);
					glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, 0.0f);
					glTexCoord2i(1, 0); glVertex3f(lightStaticTextureWidth, 0.0f, 0.0f);
					glTexCoord2i(1, 1); glVertex3f(lightStaticTextureWidth, lightStaticTextureHeight, 0.0f);
					glTexCoord2i(0, 1); glVertex3f(0.0f, lightStaticTextureHeight, 0.0f);
				glEnd();

				glPopMatrix();
			}

			pLight->updateRequired = false;
		}
		else
		{
			// Render existing texture
			pLight->pStaticTexture->GetTexture().Bind();

			Vec2f staticTextureOffset = pLight->center - pLight->aabb.lowerBound;

			glPushMatrix();

			CameraSetup();
			glTranslatef(pLight->center.x - staticTextureOffset.x, pLight->center.y - staticTextureOffset.y, 0.0f);

			glBlendFunc(GL_ONE, GL_ONE);
			glDisable(GL_DEPTH_TEST);

			const float lightStaticTextureWidth = static_cast<float>(pLight->pStaticTexture->GetWidth());
			const float lightStaticTextureHeight = static_cast<float>(pLight->pStaticTexture->GetHeight());

			glBegin(GL_QUADS);
				glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, 0.0f);
				glTexCoord2i(1, 0); glVertex3f(lightStaticTextureWidth, 0.0f, 0.0f);
				glTexCoord2i(1, 1); glVertex3f(lightStaticTextureWidth, lightStaticTextureHeight, 0.0f);
				glTexCoord2i(0, 1); glVertex3f(0.0f, lightStaticTextureHeight, 0.0f);
			glEnd();

			glPopMatrix();
		}

		regionHulls.clear();
		finsToRender.clear();
	}

	glLoadIdentity();
	CameraSetup();

	// Emissive lights
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	std::vector<QuadTreeOccupant*> visibleEmissiveLights;

	emissiveTree->Query(view, visibleEmissiveLights);

	const unsigned int numEmissiveLights = visibleEmissiveLights.size();

	for(unsigned int i = 0; i < numEmissiveLights; i++)
		static_cast<EmissiveLight*>(visibleEmissiveLights[i])->Render();

	renderTexture.Display();

	SwitchWindow();
}

void LightSystem::BuildLight(Light* pLight)
{
	lightsToPreBuild.push_back(pLight);
}

void LightSystem::RenderLightTexture(float renderDepth)
{
	sf::Vector2f viewSize(view.GetSize());
	sf::Vector2u viewSizeui(static_cast<unsigned int>(viewSize.x), static_cast<unsigned int>(viewSize.y));

	renderTexture.GetTexture().Bind();

	// Set up color function to multiply the existing color with the render texture color
	glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Seperate allows you to set color and alpha functions seperately

	glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex3f(0.0f, 0.0f, renderDepth);
		glTexCoord2i(1, 0); glVertex3f(viewSize.x, 0.0f, renderDepth);
		glTexCoord2i(1, 1); glVertex3f(viewSize.x, viewSize.y, renderDepth);
		glTexCoord2i(0, 1); glVertex3f(0.0f, viewSize.y, renderDepth);
	glEnd();

	// Reset blend function
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void LightSystem::DebugRender()
{
	// Set to a more useful-for-OpenGL projection
	sf::Vector2f viewSize(view.GetSize());
	sf::Vector2u viewSizeui(static_cast<unsigned int>(viewSize.x), static_cast<unsigned int>(viewSize.y));

	glViewport(0, 0, viewSizeui.x, viewSizeui.y);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, viewSizeui.x, 0, viewSizeui.y, -100.0f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
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