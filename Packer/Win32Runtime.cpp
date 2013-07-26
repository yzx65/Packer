#include "Win32Runtime.h"

#include <cstdint>
#include <windows.h>

//TODO
void* operator new(size_t num)
{
	return HeapAlloc(GetProcessHeap(), 0, num);
}

void* operator new[](size_t num)
{
	return HeapAlloc(GetProcessHeap(), 0, num);
}

void operator delete(void *ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

void operator delete[](void *ptr)
{
	HeapFree(GetProcessHeap(), 0, ptr);
}

extern "C"
{
	int _purecall()
	{
		return 0;
	}

	void *memset(void *dst, int val, size_t size)
	{
		for(size_t i = 0; i < size; i ++)
			*(reinterpret_cast<uint8_t *>(dst) + i) = static_cast<uint8_t>(val);
		return dst;
	}
}
