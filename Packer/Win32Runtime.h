#pragma once

//Runtime library replacement

void* operator new(size_t num);
void operator delete(void *ptr);
void operator delete[](void *ptr);
