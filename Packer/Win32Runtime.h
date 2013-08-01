#pragma once

#include <cstdint>

#define MEM_COMMIT                  0x1000      
#define MEM_RESERVE                 0x2000      
#define MEM_DECOMMIT                0x4000      
#define MEM_RELEASE                 0x8000      
#define PAGE_NOACCESS          0x01     
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     
#define PAGE_WRITECOPY         0x08     
#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80   

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
	size_t ntAllocateVirtualMemory_;
	size_t ntProtectVirtualMemory_;

	void init();
	void initNtdllImport(size_t exportDirectoryAddress);
public:
	API_SET_HEADER *getApiSet();
	void *allocateHeap(size_t dwBytes);
	bool freeHeap(void *ptr);
	void *allocateVirtual(size_t RegionSize, size_t AllocationType, size_t Protect);
	void protectVirtual(void *BaseAddress, size_t NumberOfBytes, size_t NewAccessProtection, size_t *OldAccessProtection);
	wchar_t *getCommandLine();
	size_t getNtdll();

	static Win32NativeHelper *get();
};
