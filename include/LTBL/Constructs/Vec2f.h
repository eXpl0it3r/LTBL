#ifndef LTBL_VEC2F_H
#define LTBL_VEC2F_H

#include <iostream>

class Vec2f 
{
public:
	float x, y;

	Vec2f();
	Vec2f(float X, float Y);

	bool operator==(const Vec2f &other) const;
		
	Vec2f operator*(float scale) const;
	Vec2f operator/(float scale) const;
	Vec2f operator+(const Vec2f &other) const;
	Vec2f operator-(const Vec2f &other) const;
	Vec2f operator-() const;
		
	const Vec2f &operator*=(float scale);
	const Vec2f &operator/=(float scale);
	const Vec2f &operator+=(const Vec2f &other);
	const Vec2f &operator-=(const Vec2f &other);
		
	float Magnitude() const;
	float MagnitudeSquared() const;
	Vec2f Normalize() const;
	float Dot(const Vec2f &other) const;
	float Cross(const Vec2f &other) const;
};

Vec2f operator*(float scale, const Vec2f &v);
std::ostream &operator<<(std::ostream &output, const Vec2f &v);

#endif