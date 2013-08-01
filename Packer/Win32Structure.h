#pragma once

#include <cstdint>
//Undocumented win32 structures

typedef struct _UNICODE_STRING {
	uint16_t Length;
	uint16_t MaximumLength;
	wchar_t *  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef union _LARGE_INTEGER {
	struct {
		uint32_t LowPart;
		uint32_t HighPart;
	};
	struct {
		uint32_t LowPart;
		uint32_t HighPart;
	} u;
	uint64_t QuadPart;
} LARGE_INTEGER;

typedef struct _LIST_ENTRY {
	struct _LIST_ENTRY *Flink;
	struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _LDR_MODULE {

	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	void * BaseAddress;
	void * EntryPoint;
	uint32_t SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	uint32_t Flags;
	int16_t LoadCount;
	int16_t TlsIndex;
	LIST_ENTRY HashTableEntry;
	uint32_t TimeDateStamp;

} LDR_MODULE, *PLDR_MODULE;

typedef struct _PEB_LDR_DATA {

	uint32_t Length;
	int8_t Initialized;
	void * SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;

} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_DRIVE_LETTER_CURDIR {

	uint16_t Flags;
	uint16_t Length;
	uint32_t TimeStamp;
	UNICODE_STRING DosPath;

} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {

	uint32_t MaximumLength;
	uint32_t Length;
	uint32_t Flags;
	uint32_t DebugFlags;
	void * ConsoleHandle;
	uint32_t ConsoleFlags;
	void * StdInputHandle;
	void * StdOutputHandle;
	void * StdErrorHandle;
	UNICODE_STRING CurrentDirectoryPath;
	void * CurrentDirectoryHandle;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	void * Environment;
	uint32_t StartingPositionLeft;
	uint32_t StartingPositionTop;
	uint32_t Width;
	uint32_t Height;
	uint32_t CharWidth;
	uint32_t CharHeight;
	uint32_t ConsoleTextAttributes;
	uint32_t WindowFlags;
	uint32_t ShowWindowFlags;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING DesktopName;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeData;
	RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];


} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef void (*PPEBLOCKROUTINE)(
	void * PebLock
	);

typedef struct _api_set_host {
	uint32_t           ImportModuleName;
	uint16_t            ImportModuleNameLength;
	uint32_t           HostModuleName;
	uint16_t            HostModuleNameLength;
} API_SET_HOST;

typedef struct _api_set_host_descriptor {
	uint32_t           NumberOfHosts;
	API_SET_HOST    Hosts[1];
} API_SET_HOST_DESCRIPTOR;

typedef struct _api_set_entry {
	uint32_t           Name;
	uint16_t            NameLength;
	uint32_t           HostDescriptor;
} API_SET_ENTRY;

typedef struct _api_set_header {
	uint32_t           unknown1;
	uint32_t           NumberOfEntries;
	API_SET_ENTRY   Entries[1];
} API_SET_HEADER;	

typedef struct _PEB {

	int8_t InheritedAddressSpace;
	int8_t ReadImageFileExecOptions;
	int8_t BeingDebugged;
	int8_t Spare;
	void * Mutant;
	void * ImageBaseAddress;
	PPEB_LDR_DATA LoaderData;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	void * SubSystemData;
	void * ProcessHeap;
	void * FastPebLock;
	PPEBLOCKROUTINE FastPebLockRoutine;
	PPEBLOCKROUTINE FastPebUnlockRoutine;
	uint32_t EnvironmentUpdateCount;
	void * KernelCallbackTable;
	void * EventLogSection;
	void * EventLog;
	API_SET_HEADER *ApiSet;
	uint32_t TlsExpansionCounter;
	void * TlsBitmap;
	uint32_t TlsBitmapBits[0x2];
	void * ReadOnlySharedMemoryBase;
	void * ReadOnlySharedMemoryHeap;
	void * ReadOnlyStaticServerData;
	void * AnsiCodePageData;
	void * OemCodePageData;
	void * UnicodeCaseTableData;
	uint32_t NumberOfProcessors;
	uint32_t NtGlobalFlag;
	uint8_t Spare2[0x4];
	LARGE_INTEGER CriticalSectionTimeout;
	uint32_t HeapSegmentReserve;
	uint32_t HeapSegmentCommit;
	uint32_t HeapDeCommitTotalFreeThreshold;
	uint32_t HeapDeCommitFreeBlockThreshold;
	uint32_t NumberOfHeaps;
	uint32_t MaximumNumberOfHeaps;
	void * *ProcessHeaps;
	void * GdiSharedHandleTable;
	void * ProcessStarterHelper;
	void * GdiDCAttributeList;
	void * LoaderLock;
	uint32_t OSMajorVersion;
	uint32_t OSMinorVersion;
	uint32_t OSBuildNumber;
	uint32_t OSPlatformId;
	uint32_t ImageSubSystem;
	uint32_t ImageSubSystemMajorVersion;
	uint32_t ImageSubSystemMinorVersion;
	uint32_t GdiHandleBuffer[0x22];
	uint32_t PostProcessInitRoutine;
	uint32_t TlsExpansionBitmap;
	uint8_t TlsExpansionBitmapBits[0x80];
	uint32_t SessionId;

} PEB, *PPEB;