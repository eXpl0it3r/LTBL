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

#include "SFML_OpenGL.h"

#include <iostream>

namespace ltbl
{
	bool glewInitialized = false;

	void InitGlew()
	{
		if(glewInitialized)
			return;

		GLenum err = glewInit();

		if(GLEW_OK != err)
		{
			std::cout << "Could not initialize GLEW! Aborting..." << std::endl;
			abort();
		}

		glewInitialized = true;
	}

	void DrawQuad(sf::Texture &Texture)
	{
		float halfWidth = Texture.GetWidth() / 2.0f;
		float halfHeight = Texture.GetHeight() / 2.0f;

		// Bind the texture
		Texture.Bind();

		// Have to render upside-down because SFML loads the Textures upside-down
		glBegin(GL_QUADS);
			glTexCoord2i(0, 0); glVertex3f(-halfWidth, -halfHeight, 0.0f);
			glTexCoord2i(1, 0); glVertex3f(halfWidth, -halfHeight, 0.0f);
			glTexCoord2i(1, 1); glVertex3f(halfWidth, halfHeight, 0.0f);
			glTexCoord2i(0, 1); glVertex3f(-halfWidth, halfHeight, 0.0f);
		glEnd();
	}
}