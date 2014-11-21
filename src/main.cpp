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

#include "Light_Point.h"

#include <assert.h>

#include <SFML/Graphics.hpp>

#include <sstream>

int main(int argc, char* args[])
{ 
	sf::VideoMode vidMode;
	vidMode.Width = 800;
	vidMode.Height = 600;
	vidMode.BitsPerPixel = 32;
	assert(vidMode.IsValid());

	sf::RenderWindow win;
	win.Create(vidMode, "Let there be Light - Demo");

	sf::View view;
	view.SetSize(sf::Vector2f((float)win.GetWidth(), (float)win.GetHeight()));
	view.SetCenter(view.GetSize() / 2.0f);

	// ---------------------- Background Image ---------------------

	sf::Texture backgroundImage;

	assert(backgroundImage.LoadFromFile("data/background.png"));

	// Tiling background
	backgroundImage.SetRepeated(true);

	sf::Sprite backgroundSprite(backgroundImage);
	backgroundSprite.SetTextureRect(sf::IntRect(0, 0, vidMode.Width * 2, vidMode.Height * 2));

	// --------------------- Light System Setup ---------------------

	ltbl::LightSystem ls(AABB(Vec2f(0.0f, 0.0f), Vec2f(static_cast<float>(vidMode.Width), static_cast<float>(vidMode.Height))), &win, "data/lightFin.png", "data/shaders/lightAttenuationShader.frag");

	// Create a light
	ltbl::Light_Point* testLight = new ltbl::Light_Point();
	testLight->intensity = 2.0f;
	testLight->center = Vec2f(200.0f, 200.0f);
	testLight->radius = 600.0f;
	testLight->size = 15.0f;
	testLight->spreadAngle = 2.0f * static_cast<float>(M_PI);
	testLight->softSpreadAngle = 0.0f;
	testLight->CalculateAABB();

	testLight->bleed = 0.4f;
	testLight->linearizeFactor = 0.2f;

	ls.AddLight(testLight);

	testLight->SetAlwaysUpdate(false);

	// Create a light
	ltbl::Light_Point* testLight2 = new ltbl::Light_Point();
	testLight2->center = Vec2f(200.0f, 200.0f);
	testLight2->radius = 500.0f;
	testLight2->size = 30.0f;
	testLight2->color.r = 0.5f;
	testLight2->intensity = 1.5f;
	testLight2->spreadAngle = 2.0f * static_cast<float>(M_PI);
	testLight2->softSpreadAngle = 0.0f;
	testLight2->CalculateAABB();

	ls.AddLight(testLight2);

	testLight2->SetAlwaysUpdate(false);

	// Create an emissive light
	ltbl::EmissiveLight* emissiveLight = new ltbl::EmissiveLight();

	sf::Texture text;

	if(!text.LoadFromFile("data/emissive.png"))
		abort();

	emissiveLight->SetTexture(&text);

	emissiveLight->SetRotation(45.0f);

	emissiveLight->intensity = 1.3f;

	ls.AddEmissiveLight(emissiveLight);

	emissiveLight->SetCenter(Vec2f(500.0f, 500.0f));

	// Create a hull by loading it from a file
	ltbl::ConvexHull* testHull = new ltbl::ConvexHull();

	if(!testHull->LoadShape("data/testShape.txt"))
		abort();

	// Pre-calculate certain aspects
	testHull->CalculateNormals();
	testHull->GenerateAABB();

	testHull->SetWorldCenter(Vec2f(300.0f, 300.0f));

	ls.AddConvexHull(testHull);

	// ------------------------- Game Loop --------------------------

	sf::Event eventStructure;

	bool quit = false;

	while(!quit)
	{
		while(win.PollEvent(eventStructure))
			if(eventStructure.Type == sf::Event::Closed)
			{
				quit = true;
				break;
			}

		if(sf::Keyboard::IsKeyPressed(sf::Keyboard::A))
			view.Move(sf::Vector2f(-1.0f, 0.0f));
		else if(sf::Keyboard::IsKeyPressed(sf::Keyboard::D))
			view.Move(sf::Vector2f(1.0f, 0.0f));

		if(sf::Keyboard::IsKeyPressed(sf::Keyboard::W))
			view.Move(sf::Vector2f(0.0f, -1.0f));
		else if(sf::Keyboard::IsKeyPressed(sf::Keyboard::S))
			view.Move(sf::Vector2f(0.0f, 1.0f));

		sf::Vector2i mousePos = sf::Mouse::GetPosition(win);
		//testLight2->IncCenter(ltbl::Vec2f(0.1f, 0.0f));
		// Update light
		testLight->SetCenter(Vec2f(static_cast<float>(mousePos.x), static_cast<float>(vidMode.Height) - static_cast<float>(mousePos.y)));

		win.Clear();

		win.SetView(view);
		ls.SetView(view);

		// Draw the background
		win.Draw(backgroundSprite);

		// Calculate the lights
		ls.RenderLights();

		// Draw the lights
		ls.RenderLightTexture();

		//ls.DebugRender();

		win.Display();
	}

	win.Close();
}