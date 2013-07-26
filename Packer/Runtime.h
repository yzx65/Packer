#pragma once

void* operator new(size_t num);
void* operator new[](size_t num);
void operator delete(void *ptr);
void operator delete[](void *ptr);

#ifdef _MSC_VER
#include <type_traits> //for std::move
#endif