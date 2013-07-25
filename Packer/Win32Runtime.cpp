#include "Win32Runtime.h"

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
