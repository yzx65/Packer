#pragma once

#include <cstdint>

struct _PEB;
typedef struct _PEB PEB;
class Win32NativeHelper
{
private:
	bool init_;
	size_t ntdllBase_;
	PEB *myPEB_;

	void *heap_;
	size_t rtlAllocateHeap_;
	size_t rtlFreeHeap_;

	void init();
	void initNtdllImport(size_t exportDirectoryAddress);
public:
	void *allocateHeap(size_t dwBytes);
	bool freeHeap(void *ptr);
	wchar_t *getCommandLine();

	static Win32NativeHelper *get();
};
