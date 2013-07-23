#include "Vector.h"

#include <cstdint>

template<typename CharacterType=char>
class String : public Vector<CharacterType>
{
private:
	static size_t length_(const char *str)
	{
		size_t result = 0;
		while(*str ++)
			result ++;
		return result;
	}
public:
	String(const char *string)
	{
		size_t length = length_(string);
		resize(length + 1);
		assign(const_cast<char>(string), length + 1);
	}
};
