#include <LTBL/Constructs/AABB.h>

#include <SFML/OpenGL.hpp>

AABB::AABB()
	: m_lowerBound(0.0f, 0.0f), m_upperBound(1.0f, 1.0f),
	m_center(0.5f, 0.5f), m_halfDims(0.5f, 0.5f)
{
}

AABB::AABB(const Vec2f &lowerBound, const Vec2f &upperBound)
	: m_lowerBound(lowerBound), m_upperBound(upperBound)
{
	CalculateHalfDims();
	CalculateCenter();
}

void AABB::CalculateHalfDims()
{
	m_halfDims = (m_upperBound - m_lowerBound) / 2.0f;
}

void AABB::CalculateCenter()
{
	m_center = m_lowerBound + m_halfDims;
}

void AABB::CalculateBounds()
{
	m_lowerBound = m_center - m_halfDims;
	m_upperBound = m_center + m_halfDims;
}

const Vec2f &AABB::GetCenter() const
{
	return m_center;
}

Vec2f AABB::GetDims() const
{
	return m_upperBound - m_lowerBound;
}

const Vec2f &AABB::GetHalfDims() const
{
	return m_halfDims;
}

const Vec2f &AABB::GetLowerBound() const
{
	return m_lowerBound;
}

const Vec2f &AABB::GetUpperBound() const
{
	return m_upperBound;
}

AABB AABB::GetRotatedAABB(float angleRads) const
{
	// Get new dimensions
	float cosOfAngle = cosf(angleRads);
	float sinOfAngle = sinf(angleRads);

	Vec2f newHalfDims(m_halfDims.y * sinOfAngle + m_halfDims.x * cosOfAngle, m_halfDims.x * sinOfAngle + m_halfDims.y * cosOfAngle);

	return AABB(m_center - newHalfDims, m_center + newHalfDims);
}

void AABB::SetCenter(const Vec2f &newCenter)
{
	m_center = newCenter;
	
	CalculateBounds();
}

void AABB::IncCenter(const Vec2f &increment)
{
	m_center += increment;
	
	CalculateBounds();
}

void AABB::SetDims(const Vec2f &newDims)
{
	SetHalfDims(newDims / 2.0f);
}

void AABB::SetHalfDims(const Vec2f &newDims)
{
	m_halfDims = newDims;

	CalculateBounds();
}

void AABB::SetRotatedAABB(float angleRads)
{
	float cosOfAngle = cosf(angleRads);
	float sinOfAngle = sinf(angleRads);

	m_halfDims.x = m_halfDims.y * sinOfAngle + m_halfDims.x * cosOfAngle;
	m_halfDims.y = m_halfDims.x * sinOfAngle + m_halfDims.y * cosOfAngle;
}

bool AABB::Intersects(const AABB &other) const
{
	if(m_upperBound.x < other.m_lowerBound.x)
		return false;

	if(m_upperBound.y < other.m_lowerBound.y)
		return false;

	if(m_lowerBound.x > other.m_upperBound.x)
		return false;

	if(m_lowerBound.y > other.m_upperBound.y)
		return false;

	return true;
}

bool AABB::Contains(const AABB &other) const
{
	if(other.m_lowerBound.x >= m_lowerBound.x &&
		other.m_upperBound.x <= m_upperBound.x &&
		other.m_lowerBound.y >= m_lowerBound.y &&
		other.m_upperBound.y <= m_upperBound.y)
		return true;

	return false;
}

void AABB::DebugRender()
{
	// Render the AABB with lines
	glBegin(GL_LINES);
	
	// Bottom
	glVertex2f(m_lowerBound.x, m_lowerBound.y);
	glVertex2f(m_upperBound.x, m_lowerBound.y);

	// Right
	glVertex2f(m_upperBound.x, m_lowerBound.y);
	glVertex2f(m_upperBound.x, m_upperBound.y);

	// Top
	glVertex2f(m_upperBound.x, m_upperBound.y);
	glVertex2f(m_lowerBound.x, m_upperBound.y);

	// Left
	glVertex2f(m_lowerBound.x, m_upperBound.y);
	glVertex2f(m_lowerBound.x, m_lowerBound.y);

	glEnd();
}