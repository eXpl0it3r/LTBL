#ifndef LTBL_COLOR3F_H
#define LTBL_COLOR3F_H

class Color3f
{
public:
	float r, g, b;

	Color3f();
	Color3f(float R, float G, float B);

	void GL_Set();
};

#endif