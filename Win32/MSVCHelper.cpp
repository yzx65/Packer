#include "../Util/Util.h"

extern "C" void *__cdecl memset(void *, int, size_t);
#pragma intrinsic(memset)
extern "C" void *__cdecl memcpy(void *dst, const void *src, size_t size);
#pragma intrinsic(memcpy)
extern "C" void *__cdecl memmove(void *dst, const void *src, size_t size);

extern "C"
{
	int _purecall()
	{
		return 0;
	}

#pragma function(memset)
	void *__cdecl memset(void *dst, int val, size_t size)
	{
		setMemory(dst, val, size);
		return dst;
	}

#pragma function(memcpy)
	void *__cdecl memcpy(void *dst, const void *src, size_t size)
	{
		copyMemory(dst, src, size);
		return dst;
	}

	void *__cdecl memmove(void *dst, const void *src, size_t size)
	{
		moveMemory(dst, src, size);
		return dst;
	}
}
