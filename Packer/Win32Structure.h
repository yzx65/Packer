#pragma once

#include <cstdint>
//win32 structures

typedef struct _UNICODE_STRING {
	uint16_t Length;
	uint16_t MaximumLength;
	wchar_t *  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _ANSI_STRING {
	uint16_t Length;
	uint16_t MaximumLength;
	char *  Buffer;
} ANSI_STRING;
typedef ANSI_STRING *PANSI_STRING;
typedef const ANSI_STRING *PCANSI_STRING;

typedef struct _OBJECT_ATTRIBUTES {
	uint32_t           Length;
	void *          RootDirectory;
	PUNICODE_STRING ObjectName;
	uint32_t           Attributes;
	void *           SecurityDescriptor;
	void *           SecurityQualityOfService;
}  OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _IO_STATUS_BLOCK {
	union {
		uint32_t Status;
		void *    Pointer;
	};
	uint32_t *Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

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
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	uint32_t         FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

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

#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)

#define STANDARD_RIGHTS_ALL              (0x001F0000L)

#define SPECIFIC_RIGHTS_ALL              (0x0000FFFFL)


#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory

#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define FILE_ADD_FILE             ( 0x0002 )    // directory

#define FILE_APPEND_DATA          ( 0x0004 )    // file
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe


#define FILE_READ_EA              ( 0x0008 )    // file & directory

#define FILE_WRITE_EA             ( 0x0010 )    // file & directory

#define FILE_EXECUTE              ( 0x0020 )    // file
#define FILE_TRAVERSE             ( 0x0020 )    // directory

#define FILE_DELETE_CHILD         ( 0x0040 )    // directory

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all

#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
	FILE_READ_DATA           |\
	FILE_READ_ATTRIBUTES     |\
	FILE_READ_EA             |\
	SYNCHRONIZE)


#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
	FILE_WRITE_DATA          |\
	FILE_WRITE_ATTRIBUTES    |\
	FILE_WRITE_EA            |\
	FILE_APPEND_DATA         |\
	SYNCHRONIZE)


#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
	FILE_READ_ATTRIBUTES     |\
	FILE_EXECUTE             |\
	SYNCHRONIZE)

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L


#define EXCEPTION_MAXIMUM_PARAMETERS 15 // maximum number of exception parameters
typedef struct _EXCEPTION_RECORD {
	uint32_t    ExceptionCode;
	uint32_t ExceptionFlags;
	struct _EXCEPTION_RECORD *ExceptionRecord;
	void *ExceptionAddress;
	uint32_t NumberParameters;
	size_t *ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD;


#define SIZE_OF_80387_REGISTERS      80
typedef struct _FLOATING_SAVE_AREA {
	uint32_t   ControlWord;
	uint32_t   StatusWord;
	uint32_t   TagWord;
	uint32_t   ErrorOffset;
	uint32_t   ErrorSelector;
	uint32_t   DataOffset;
	uint32_t   DataSelector;
	uint8_t    RegisterArea[SIZE_OF_80387_REGISTERS];
	uint32_t   Cr0NpxState;
} FLOATING_SAVE_AREA;


#define MAXIMUM_SUPPORTED_EXTENSION     512
typedef struct _CONTEXT {

	//
	// The flags values within this flag control the contents of
	// a CONTEXT record.
	//
	// If the context record is used as an input parameter, then
	// for each portion of the context record controlled by a flag
	// whose value is set, it is assumed that that portion of the
	// context record contains valid context. If the context record
	// is being used to modify a threads context, then only that
	// portion of the threads context will be modified.
	//
	// If the context record is used as an IN OUT parameter to capture
	// the context of a thread, then only those portions of the thread's
	// context corresponding to set flags will be returned.
	//
	// The context record is never used as an OUT only parameter.
	//

	uint32_t ContextFlags;

	//
	// This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
	// set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
	// included in CONTEXT_FULL.
	//

	uint32_t   Dr0;
	uint32_t   Dr1;
	uint32_t   Dr2;
	uint32_t   Dr3;
	uint32_t   Dr6;
	uint32_t   Dr7;

	//
	// This section is specified/returned if the
	// ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
	//

	FLOATING_SAVE_AREA FloatSave;

	//
	// This section is specified/returned if the
	// ContextFlags word contians the flag CONTEXT_SEGMENTS.
	//

	uint32_t   SegGs;
	uint32_t   SegFs;
	uint32_t   SegEs;
	uint32_t   SegDs;

	//
	// This section is specified/returned if the
	// ContextFlags word contians the flag CONTEXT_INTEGER.
	//

	uint32_t   Edi;
	uint32_t   Esi;
	uint32_t   Ebx;
	uint32_t   Edx;
	uint32_t   Ecx;
	uint32_t   Eax;

	//
	// This section is specified/returned if the
	// ContextFlags word contians the flag CONTEXT_CONTROL.
	//

	uint32_t   Ebp;
	uint32_t   Eip;
	uint32_t   SegCs;              // MUST BE SANITIZED
	uint32_t   EFlags;             // MUST BE SANITIZED
	uint32_t   Esp;
	uint32_t   SegSs;

	//
	// This section is specified/returned if the ContextFlags word
	// contains the flag CONTEXT_EXTENDED_REGISTERS.
	// The format and contexts are processor specific
	//

	uint8_t    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} CONTEXT;


typedef enum _EXCEPTION_DISPOSITION {
	ExceptionContinueExecution,
	ExceptionContinueSearch,
	ExceptionNestedException,
	ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;
