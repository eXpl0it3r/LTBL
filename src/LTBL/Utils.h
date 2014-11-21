#ifndef LTBL_UTILS_H
#define LTBL_UTILS_H

#define _USE_MATH_DEFINES
#include <math.h>

#include <string>

namespace ltbl
{
	const float pif = static_cast<float>(M_PI);
	const float pifTimes2 = pif * 2.0f;

	template<class T> T Wrap(T val, T size)
	{
		if(val < 0)
			return val + size;

		if(val >= size)
			return val - size;

		return val;
	}

	float GetFloatVal(std::string strConvert);
}

#endif