#pragma once

#include <cstdint>
#include "List.h"

struct _PEB;
typedef struct _PEB PEB;
struct _api_set_header;
typedef struct _api_set_header API_SET_HEADER;
class PEFormat;

struct Win32LoadedImage
{
	const wchar_t *fileName;
	uint8_t *baseAddress;
};

class Win32NativeHelper
{
private:
	bool init_;
	size_t ntdllBase_;
	PEB *myPEB_;

	size_t rtlAllocateHeap_;
	size_t rtlFreeHeap_;
	size_t ntAllocateVirtualMemory_;
	size_t ntProtectVirtualMemory_;
	size_t ntCreateFile_;
	size_t ntClose_;
	size_t ntCreateSection_;
	size_t ntMapViewOfSection_;
	size_t ntUnmapViewOfSection_;
	size_t ntQueryFullAttributesFile_;

	void init();
	void initNtdllImport(const PEFormat &ntdll);
public:
	API_SET_HEADER *getApiSet();
	void *allocateHeap(size_t dwBytes);
	bool freeHeap(void *ptr);
	void *allocateVirtual(size_t RegionSize, size_t AllocationType, size_t Protect);
	void protectVirtual(void *BaseAddress, size_t NumberOfBytes, size_t NewAccessProtection, size_t *OldAccessProtection);
	void *createFile(uint32_t DesiredAccess, const wchar_t *Filename, size_t FilenameLength, size_t FileAttributes, size_t ShareAccess, size_t CreateDisposition, size_t CreateOptions);
	void closeHandle(void *handle);
	void *createSection(void *file, uint32_t flProtect, uint64_t sectionSize, wchar_t *lpName, size_t NameLength);
	void *mapViewOfSection(void *section, uint32_t dwDesiredAccess, uint64_t offset, size_t dwNumberOfBytesToMap, void *lpBaseAddress);
	void unmapViewOfSection(void *lpBaseAddress);
	uint32_t getFileAttributes(const wchar_t *filePath, size_t filePathLen);
	wchar_t *getCommandLine();
	wchar_t *getCurrentDirectory();
	wchar_t *getEnvironments();
	bool isInitialized();
	PEB *getPEB();
	List<Win32LoadedImage> getLoadedImages();

	static Win32NativeHelper *get();
};

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

#define FILE_SHARE_READ                 0x00000001  
#define FILE_SHARE_WRITE                0x00000002  
#define FILE_SHARE_DELETE               0x00000004  

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

#define PAGE_NOACCESS          0x01     
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     
#define PAGE_WRITECOPY         0x08     
#define PAGE_EXECUTE           0x10     
#define PAGE_EXECUTE_READ      0x20     
#define PAGE_EXECUTE_READWRITE 0x40     
#define PAGE_EXECUTE_WRITECOPY 0x80   

#define INVALID_HANDLE_VALUE reinterpret_cast<void *>(-1)

#define SECTION_QUERY                0x0001
#define SECTION_MAP_WRITE            0x0002
#define SECTION_MAP_READ             0x0004
#define SECTION_MAP_EXECUTE          0x0008
#define SECTION_EXTEND_SIZE          0x0010
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020 // not included in SECTION_ALL_ACCESS

#define SECTION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY|\
	SECTION_MAP_WRITE |      \
	SECTION_MAP_READ |       \
	SECTION_MAP_EXECUTE |    \
	SECTION_EXTEND_SIZE)

#define SESSION_QUERY_ACCESS  0x0001
#define SESSION_MODIFY_ACCESS 0x0002

#define SESSION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED |  \
	SESSION_QUERY_ACCESS |             \
	SESSION_MODIFY_ACCESS)

#define FILE_MAP_WRITE      SECTION_MAP_WRITE
#define FILE_MAP_READ       SECTION_MAP_READ
#define FILE_MAP_ALL_ACCESS SECTION_ALL_ACCESS
#define FILE_MAP_EXECUTE    SECTION_MAP_EXECUTE_EXPLICIT    // not included in FILE_MAP_ALL_ACCESS
#define FILE_MAP_COPY       0x00000001
#define FILE_MAP_RESERVE    0x80000000

#define INVALID_FILE_ATTRIBUTES ((uint32_t)-1)
#define SEC_FILE           0x800000     
#define SEC_IMAGE         0x1000000     
#define SEC_PROTECTED_IMAGE  0x2000000  
#define SEC_RESERVE       0x4000000     
#define SEC_COMMIT        0x8000000     
#define SEC_NOCACHE      0x10000000     
#define SEC_WRITECOMBINE 0x40000000     
#define SEC_LARGE_PAGES  0x80000000     