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

#include "Constructs.h"

#include "SFML_OpenGL.h"

using namespace ltbl;

Color3f::Color3f()
	: r(0.0f), g(0.0f), b(0.0f)
{
}

Color3f::Color3f(float R, float G, float B)
	: r(R), g(G), b(B)
{
}

Point2i::Point2i()
{
}

Point2i::Point2i(int X, int Y)
	: x(X), y(Y)
{
}

bool Point2i::operator==(const Point2i &other) const
{
	if(x == other.x && y == other.y)
		return true;

	return false;
}

Vec2f::Vec2f() 
{
}

Vec2f::Vec2f(float X, float Y)
	: x(X), y(Y)
{
}

bool Vec2f::operator==(const Vec2f &other) const 
{
	if(x == other.x && y == other.y)
		return true;

	return false;
}

Vec2f Vec2f::operator*(float scale) const 
{
	return Vec2f(x * scale, y * scale);
}

Vec2f Vec2f::operator/(float scale) const 
{
	return Vec2f(x / scale, y / scale);
}


Vec2f Vec2f::operator+(const Vec2f &other) const 
{
	return Vec2f(x + other.x, y + other.y);
}

Vec2f Vec2f::operator-(const Vec2f &other) const 
{
	return Vec2f(x - other.x, y - other.y);
}

Vec2f Vec2f::operator-() const
{
	return Vec2f(-x, -y);
}

const Vec2f &Vec2f::operator*=(float scale) 
{
	x *= scale;
	y *= scale;

	return *this;
}

const Vec2f &Vec2f::operator/=(float scale) 
{
	x /= scale;
	y /= scale;

	return *this;
}

const Vec2f &Vec2f::operator+=(const Vec2f &other) 
{
	x += other.x;
	y += other.y;

	return *this;
}

const Vec2f &Vec2f::operator-=(const Vec2f &other) 
{
	x -= other.x;
	y -= other.y;

	return *this;
}

float Vec2f::magnitude() const 
{
	return sqrt(x * x + y * y);
}

float Vec2f::magnitudeSquared() const 
{
	return x * x + y * y;
}

Vec2f Vec2f::normalize() const 
{
	float m = sqrt(x * x + y * y);
	return Vec2f(x / m, y / m);
}

float Vec2f::dot(const Vec2f &other) const 
{
	return x * other.x + y * other.y;
}

float Vec2f::cross(const Vec2f &other) const 
{
	return x * other.y - y * other.x;
}

Vec2f operator*(float scale, const Vec2f &v) 
{
	return v * scale;
}

std::ostream &operator<<(std::ostream &output, const Vec2f &v)
{
	std::cout << '(' << v.x << ", " << v.y << ')';
	return output;
}

AABB::AABB()
	: lowerBound(0.0f, 0.0f), upperBound(1.0f, 1.0f),
	center(0.5f, 0.5f), halfDims(0.5f, 0.5f)
{
}

AABB::AABB(const Vec2f &bottomLeft, const Vec2f &topRight)
	: lowerBound(bottomLeft), upperBound(topRight)
{
	CalculateHalfDims();
	CalculateCenter();
}

void AABB::CalculateHalfDims()
{
	halfDims = (upperBound - lowerBound) / 2.0f;
}

void AABB::CalculateCenter()
{
	center = lowerBound + halfDims;
}

void AABB::CalculateBounds()
{
	lowerBound = center - halfDims;
	upperBound = center + halfDims;
}

const Vec2f &AABB::GetCenter() const
{
	return center;
}

Vec2f AABB::GetDims() const
{
	return upperBound - lowerBound;
}

const Vec2f &AABB::GetHalfDims() const
{
	return halfDims;
}

const Vec2f &AABB::GetLowerBound() const
{
	return lowerBound;
}

const Vec2f &AABB::GetUpperBound() const
{
	return upperBound;
}

AABB AABB::GetRotatedAABB(float angleRads) const
{
	// Get new dimensions
	float cosOfAngle = cosf(angleRads);
	float sinOfAngle = sinf(angleRads);

	Vec2f newHalfDims(halfDims.y * sinOfAngle + halfDims.x * cosOfAngle, halfDims.x * sinOfAngle + halfDims.y * cosOfAngle);

	return AABB(center - newHalfDims, center + newHalfDims);
}

void AABB::SetCenter(const Vec2f &newCenter)
{
	center = newCenter;
	
	CalculateBounds();
}

void AABB::IncCenter(const Vec2f &increment)
{
	center += increment;
	
	CalculateBounds();
}

void AABB::SetDims(const Vec2f &newDims)
{
	SetHalfDims(newDims / 2.0f);
}

void AABB::SetHalfDims(const Vec2f &newDims)
{
	halfDims = newDims;

	CalculateBounds();
}

void AABB::SetRotatedAABB(float angleRads)
{
	float cosOfAngle = cosf(angleRads);
	float sinOfAngle = sinf(angleRads);

	halfDims.x = halfDims.y * sinOfAngle + halfDims.x * cosOfAngle;
	halfDims.y = halfDims.x * sinOfAngle + halfDims.y * cosOfAngle;
}

bool AABB::Intersects(const AABB &other) const
{
	if(upperBound.x < other.lowerBound.x)
		return false;

	if(upperBound.y < other.lowerBound.y)
		return false;

	if(lowerBound.x > other.upperBound.x)
		return false;

	if(lowerBound.y > other.upperBound.y)
		return false;

	return true;
}

bool AABB::Contains(const AABB &other) const
{
	if(other.lowerBound.x >= lowerBound.x &&
		other.upperBound.x <= upperBound.x &&
		other.lowerBound.y >= lowerBound.y &&
		other.upperBound.y <= upperBound.y)
		return true;

	return false;
}

void AABB::DebugRender()
{
	// Render the AABB with lines
	glBegin(GL_LINES);
	
	// Bottom
	glVertex3f(lowerBound.x, lowerBound.y, 0.0f);
	glVertex3f(upperBound.x, lowerBound.y, 0.0f);

	// Right
	glVertex3f(upperBound.x, lowerBound.y, 0.0f);
	glVertex3f(upperBound.x, upperBound.y, 0.0f);

	// Top
	glVertex3f(upperBound.x, upperBound.y, 0.0f);
	glVertex3f(lowerBound.x, upperBound.y, 0.0f);

	// Left
	glVertex3f(lowerBound.x, upperBound.y, 0.0f);
	glVertex3f(lowerBound.x, lowerBound.y, 0.0f);

	glEnd();
}