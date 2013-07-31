#pragma once

#include <cstdint>

struct _PEB;
typedef struct _PEB PEB;
struct _api_set_header;
typedef struct _api_set_header API_SET_HEADER;
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
	API_SET_HEADER *getApiSet();
	void *allocateHeap(size_t dwBytes);
	bool freeHeap(void *ptr);
	wchar_t *getCommandLine();
	size_t getNtdll();

	static Win32NativeHelper *get();
};
