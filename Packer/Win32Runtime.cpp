#include "Win32Runtime.h"

#include <cstdlib>

//TODO
void* operator new(size_t num)
{
	return malloc(num);
}

void operator delete(void *ptr)
{
	free(ptr);
}

void operator delete[](void *ptr)
{
	free(ptr);
}
