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

#include <LTBL/Light/Light_Point.h>
#include <LTBL/Light/ShadowFin.h>

#include <LTBL/Utils.h>

#include <assert.h>

namespace ltbl
{
	Light_Point::Light_Point()
		: m_directionAngle(0.0f), m_spreadAngle(pifTimes2),
		m_softSpreadAngle(0.0f), m_lightSubdivisionSize(pif / 24.0f)
	{
	}

	Light_Point::~Light_Point()
	{
	}

	void Light_Point::RenderLightSolidPortion()
	{
		float renderIntensity = m_intensity;

		// Clamp the render intensity
		if(renderIntensity > 1.0f)
			renderIntensity = 1.0f;

		assert(renderIntensity >= 0.0f);

		float r = m_color.r * m_intensity;
		float g = m_color.g * m_intensity;
		float b = m_color.b * m_intensity;

		glBegin(GL_TRIANGLE_FAN);

		glVertex2f(m_center.x, m_center.y);
      
		// Set the edge color for rest of shape
		int numSubdivisions = static_cast<int>(m_spreadAngle / m_lightSubdivisionSize);
		float startAngle = m_directionAngle - m_spreadAngle / 2.0f;
      
		for(int currentSubDivision = 0; currentSubDivision <= numSubdivisions; currentSubDivision++)
		{
			float angle = startAngle + currentSubDivision * m_lightSubdivisionSize;
			glVertex2f(m_radius * cosf(angle) + m_center.x, m_radius * sinf(angle) + m_center.y);  
		}

		glEnd();
	}

	void Light_Point::RenderLightSoftPortion()
	{
		// If light goes all the way around do not render fins
		if(m_spreadAngle == pifTimes2 || m_softSpreadAngle == 0.0f)
			return;

		// Create to shadow fins to mask off a portion of the light
		ShadowFin fin1;

		float umbraAngle1 = m_directionAngle - m_spreadAngle / 2.0f;
		float penumbraAngle1 = umbraAngle1 + m_softSpreadAngle;
		fin1.m_penumbra = Vec2f(m_radius * cosf(penumbraAngle1), m_radius * sinf(penumbraAngle1));
		fin1.m_umbra = Vec2f(m_radius * cosf(umbraAngle1), m_radius * sinf(umbraAngle1));
		fin1.m_rootPos = m_center;

		fin1.Render(1.0f);

		ShadowFin fin2;

		float umbraAngle2 = m_directionAngle + m_spreadAngle / 2.0f;
		float penumbraAngle2 = umbraAngle2 - m_softSpreadAngle;
		fin2.m_penumbra = Vec2f(m_radius * cosf(penumbraAngle2), m_radius * sinf(penumbraAngle2));
		fin2.m_umbra = Vec2f(m_radius * cosf(umbraAngle2), m_radius * sinf(umbraAngle2));
		fin2.m_rootPos = m_center;
	
		fin2.Render(1.0f);
	}

	void Light_Point::CalculateAABB()
	{
		if(m_spreadAngle == pifTimes2 || m_spreadAngle == 0.0f) // 1 real value, 1 flag
		{
			Vec2f diff(m_radius, m_radius);
			m_aabb.m_lowerBound = m_center - diff;
			m_aabb.m_upperBound = m_center + diff;
		}
		else // Not a true frustum test, but puts the frustum in an aabb, better than nothing
		{
			m_aabb.m_lowerBound = m_center;
			m_aabb.m_upperBound = m_center;

			float halfSpread = m_spreadAngle / 2.0f;
			float angle1 = m_directionAngle + halfSpread;
			float angle2 = m_directionAngle - halfSpread;

			Vec2f frustVec1 = Vec2f(cosf(angle1) * m_radius, sinf(angle1) * m_radius);
			Vec2f frustVec2 = Vec2f(cosf(angle2) * m_radius, sinf(angle2) * m_radius);

			float dist = (frustVec1 + (frustVec2 - frustVec1) / 2).Magnitude();

			float lengthMultiplier = m_radius / dist;

			frustVec1 *= lengthMultiplier;
			frustVec2 *= lengthMultiplier;

			Vec2f frustumPos1 = m_center + frustVec1;
			Vec2f frustumPos2 = m_center + frustVec2;

			// Expand lower bound
			if(frustumPos1.x < m_aabb.m_lowerBound.x)
				m_aabb.m_lowerBound.x = frustumPos1.x;

			if(frustumPos1.y < m_aabb.m_lowerBound.y)
				m_aabb.m_lowerBound.y = frustumPos1.y;

			if(frustumPos2.x < m_aabb.m_lowerBound.x)
				m_aabb.m_lowerBound.x = frustumPos2.x;

			if(frustumPos2.y < m_aabb.m_lowerBound.y)
				m_aabb.m_lowerBound.y = frustumPos2.y;

			// Expand upper bound
			if(frustumPos1.x > m_aabb.m_upperBound.x)
				m_aabb.m_upperBound.x = frustumPos1.x;

			if(frustumPos1.y > m_aabb.m_upperBound.y)
				m_aabb.m_upperBound.y = frustumPos1.y;

			if(frustumPos2.x > m_aabb.m_upperBound.x)
				m_aabb.m_upperBound.x = frustumPos2.x;

			if(frustumPos2.y > m_aabb.m_upperBound.y)
				m_aabb.m_upperBound.y = frustumPos2.y;
		}

		m_aabb.CalculateHalfDims();
		m_aabb.CalculateCenter();
	}

	void Light_Point::SetDirectionAngle(float directionAngle)
	{
		assert(AlwaysUpdate());

		m_directionAngle = directionAngle;

		CalculateAABB();
		TreeUpdate();
	}

	void Light_Point::IncDirectionAngle(float increment)
	{
		assert(AlwaysUpdate());

		m_directionAngle += increment;

		CalculateAABB();
		TreeUpdate();
	}

	float Light_Point::GetDirectionAngle()
	{
		return m_directionAngle;
	}

	void Light_Point::SetSpreadAngle(float spreadAngle)
	{
		assert(AlwaysUpdate());

		m_spreadAngle = spreadAngle;

		CalculateAABB();
		TreeUpdate();
	}

	void Light_Point::IncSpreadAngle(float increment)
	{
		assert(AlwaysUpdate());

		m_spreadAngle += increment;

		CalculateAABB();
		TreeUpdate();
	}

	float Light_Point::GetSpreadAngle()
	{
		return m_spreadAngle;
	}
}