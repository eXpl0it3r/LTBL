#include <LTBL/Constructs/Color3f.h>

#include <SFML/OpenGL.hpp>

Color3f::Color3f()
{
}

Color3f::Color3f(float R, float G, float B)
	: r(R), g(G), b(B)
{
}

void Color3f::GL_Set()
{
	glColor3f(r, g, b);
}
