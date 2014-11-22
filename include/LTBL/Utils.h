#ifndef LTBL_UTILS_H
#define LTBL_UTILS_H

#include <cmath>

#include <string>

const double PI = 3.14159265359;

namespace ltbl
{
	const float pif = static_cast<float>(PI);
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
