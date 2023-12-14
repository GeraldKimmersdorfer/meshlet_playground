#include "Property.h"

#include <glm/gtx/string_cast.hpp>

std::string replace(std::string s, char c1, char c2)
{
	int l = s.length();

	// loop to traverse in the string
	for (int i = 0; i < l; i++)
	{
		// check for c1 and replace
		if (s[i] == c1)
			s[i] = c2;
	}
	return s;
}


