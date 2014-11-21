#ifndef LTBL_AABB_H
#define LTBL_AABB_H

#include <LTBL/Constructs/Vec2f.h>

class AABB
{
private:
	Vec2f m_center;
	Vec2f m_halfDims;

public:
	Vec2f m_lowerBound;
	Vec2f m_upperBound;

	void CalculateHalfDims();
	void CalculateCenter();
	void CalculateBounds();

	// Constructor
	AABB();
	AABB(const Vec2f &lowerBound, const Vec2f &upperBound);

	// Accessors
	const Vec2f &GetCenter() const;

	Vec2f GetDims() const;

	const Vec2f &GetHalfDims() const;
	const Vec2f &GetLowerBound() const;
	const Vec2f &GetUpperBound() const;

	AABB GetRotatedAABB(float angleRads) const;

	// Modifiers
	void SetLowerBound(const Vec2f &newLowerBound);
	void SetUpperBound(const Vec2f &newUpperBound);
	void SetCenter(const Vec2f &newCenter);
	void IncCenter(const Vec2f &increment);

	void SetDims(const Vec2f &newDims);

	void SetHalfDims(const Vec2f &newDims);

	void SetRotatedAABB(float angleRads);

	// Utility
	bool Intersects(const AABB &other) const;
	bool Contains(const AABB &other) const;

	// Render the AABB for debugging purposes
	void DebugRender();

	friend class AABB;
};

#endif