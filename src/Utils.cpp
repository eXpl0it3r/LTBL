#include <LTBL/Utils.h>

#include <cstdlib>
#include <sstream>

namespace ltbl
{
	float GetFloatVal(std::string strConvert)
	{
		return static_cast<float>(std::atof(strConvert.c_str()));
	}
}
