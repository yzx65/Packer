#pragma once

#include <cstdint>

class Win32NativeHelper
{
private:
	bool init_;
	size_t ntdllBase_;

	void *heap_;
	size_t rtlAllocateHeap_;
	size_t rtlFreeHeap_;

	void init();
	void initNtdllImport(size_t exportDirectoryAddress);
public:
	void *allocateHeap(size_t dwBytes);
	bool freeHeap(void *ptr);

	static Win32NativeHelper *get();
};
