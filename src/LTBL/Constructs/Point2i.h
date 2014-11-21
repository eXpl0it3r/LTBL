#ifndef LTBL_POINT2I_H
#define LTBL_POINT2I_H

class Point2i
{
public:
	int x, y;

	Point2i(int X, int Y);
	Point2i();
	
	bool operator==(const Point2i &other) const;
	bool operator!=(const Point2i &other) const;
};

#endif