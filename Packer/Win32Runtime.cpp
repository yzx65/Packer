#include "Win32Runtime.h"

#include <cstdint>

#include "PEHeader.h"
#include "Win32Structure.h"
#include "PEFormat.h"
#include "String.h"
#include "Util.h"

#include <intrin.h>

Win32NativeHelper g_helper;

//utilites
#define NtCurrentProcess() reinterpret_cast<void *>(-1)

uint8_t tempHeap[655350]; //temporary heap during initialization.
uint32_t tempHeapPtr = 0;

void *heapAlloc(size_t size)
{
	if(!g_helper.isInitialized())
	{
		void *result = tempHeap + tempHeapPtr;
		tempHeapPtr += size;
		return result;
	}
	return Win32NativeHelper::get()->allocateHeap(size);
}

void heapFree(void *ptr)
{
	if(!g_helper.isInitialized())
		return;
	Win32NativeHelper::get()->freeHeap(ptr);
}

void copyUnicodeString(UNICODE_STRING *dest, UNICODE_STRING *src)
{
	dest->Length = src->Length;
	dest->MaximumLength = src->Length;
	if(!src->Buffer)
		dest->Buffer = nullptr;
	else
		copyMemory(dest->Buffer, src->Buffer, sizeof(wchar_t) * src->Length + 2);
}

void initializeUnicodeString(UNICODE_STRING *string, wchar_t *data, size_t dataSize, size_t length = 0)
{
	if(length == 0)
		length = dataSize;
	string->Buffer = reinterpret_cast<wchar_t *>(heapAlloc(length * 2 + 2));
	string->MaximumLength = length * 2;
	string->Length = dataSize * 2;

	for(size_t i = 0; i < dataSize; i ++)
		string->Buffer[i] = data[i];
	string->Buffer[dataSize] = 0;
}

void freeUnicodeString(UNICODE_STRING *string)
{
	Win32NativeHelper::get()->freeHeap(string->Buffer);
}

void addPrefixToPath(UNICODE_STRING *string, const wchar_t *path, size_t pathSize)
{
	initializeUnicodeString(string, L"\\??\\", 4, pathSize + 4);
	for(size_t i = 4; i < pathSize + 4; i ++)
		string->Buffer[i] = path[i - 4];
	string->Buffer[pathSize + 4] = 0;
	string->Length = pathSize * 2 + 8;
}

Win32NativeHelper *Win32NativeHelper::get()
{
	if(g_helper.init_ == false)
		g_helper.init();
	return &g_helper;
}

void Win32NativeHelper::initNtdllImport(const PEFormat &ntdll)
{
	auto &exportList = ntdll.getExports();
	Vector<ExportFunction> exports(exportList.begin(), exportList.end());
	auto findItem = [&](const char *name) -> size_t
	{
		return static_cast<size_t>(binarySearch(exports.begin(), exports.end(), [&](const ExportFunction *a) -> int { return a->name.icompare(name); })->address + ntdllBase_);
	};

	rtlCreateHeap_ = findItem("RtlCreateHeap");
	rtlDestroyHeap_ = findItem("RtlDestroyHeap");
	rtlAllocateHeap_ = findItem("RtlAllocateHeap");
	rtlFreeHeap_ = findItem("RtlFreeHeap");
	rtlSizeHeap_ = findItem("RtlSizeHeap");
	ntAllocateVirtualMemory_ = findItem("NtAllocateVirtualMemory");
	ntProtectVirtualMemory_ = findItem("NtProtectVirtualMemory");
	ntFreeVirtualMemory_ = findItem("NtFreeVirtualMemory");
	ntCreateFile_ = findItem("NtCreateFile");
	ntClose_ = findItem("NtClose");
	ntCreateSection_ = findItem("NtCreateSection");
	ntMapViewOfSection_ = findItem("NtMapViewOfSection");
	ntUnmapViewOfSection_ = findItem("NtUnmapViewOfSection");
	ntQueryFullAttributesFile_ = findItem("NtQueryFullAttributesFile");
}

void Win32NativeHelper::init()
{
	uint32_t pebAddress;
#ifndef __WIN64 
	pebAddress = __readfsdword(0x30);
#elif defined(__WIN32)
	pebAddress = __readgsqword(0x60);
#endif
	myPEB_ = reinterpret_cast<PEB *>(pebAddress);

	LDR_MODULE *module = reinterpret_cast<LDR_MODULE *>(myPEB_->LoaderData->InLoadOrderModuleList.Flink->Flink);
	ntdllBase_ = reinterpret_cast<size_t>(module->BaseAddress);

	//get exports
	PEFormat format;
	format.load(MakeShared<MemoryDataSource>(reinterpret_cast<uint8_t *>(module->BaseAddress)), true);

	initNtdllImport(format);
	initModuleList();
	initHeap();
	init_ = true;
}

void Win32NativeHelper::initModuleList()
{
	loadedImages_ = new List<Win32LoadedImage>();
	LDR_MODULE *node = reinterpret_cast<LDR_MODULE *>(getPEB()->LoaderData->InLoadOrderModuleList.Flink->Flink);
	while(node->BaseAddress)
	{
		Win32LoadedImage image;
		image.baseAddress = reinterpret_cast<uint8_t *>(node->BaseAddress);
		image.fileName = node->BaseDllName.Buffer;
		loadedImages_->push_back(image);
		node = reinterpret_cast<LDR_MODULE *>(node->InLoadOrderModuleList.Flink);
	}
}

void Win32NativeHelper::initHeap()
{
	//Initialize heap not to contain address 0x00400000, which is base address of most executable.
	RTL_USER_PROCESS_PARAMETERS pp;
	UNICODE_STRING tempUS[8];
	wchar_t *buffers = reinterpret_cast<wchar_t *>(allocateVirtual(0, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	wchar_t *environment;
	size_t environmentLength;

	for(int i = 0; i < 8; i ++)
		tempUS[i].Buffer = &buffers[i * 255];

	copyMemory(reinterpret_cast<uint8_t *>(&pp), reinterpret_cast<uint8_t *>(myPEB_->ProcessParameters), sizeof(RTL_USER_PROCESS_PARAMETERS));
	copyUnicodeString(&tempUS[0], &pp.CurrentDirectoryPath);
	copyUnicodeString(&tempUS[1], &pp.DllPath);
	copyUnicodeString(&tempUS[2], &pp.ImagePathName);
	copyUnicodeString(&tempUS[3], &pp.CommandLine);
	copyUnicodeString(&tempUS[4], &pp.WindowTitle);
	copyUnicodeString(&tempUS[5], &pp.DesktopName);
	copyUnicodeString(&tempUS[6], &pp.ShellInfo);
	copyUnicodeString(&tempUS[7], &pp.RuntimeData);

	environmentLength = sizeHeap(pp.Environment);
	environment = reinterpret_cast<wchar_t *>(allocateVirtual(0, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	copyMemory(environment, pp.Environment, environmentLength);

	void *oldHeap = myPEB_->ProcessHeap;
	myPEB_->ProcessHeap = nullptr;
	myPEB_->ProcessHeaps[0] = nullptr;
	destroyHeap(oldHeap);

	allocateVirtual(0x00400000, 0x01000000, MEM_RESERVE, PAGE_EXECUTE_READWRITE); //reserve

	myPEB_->ProcessHeap = createHeap(0);
	myPEB_->ProcessHeaps[0] = myPEB_->ProcessHeap;
	myPEB_->ProcessParameters = reinterpret_cast<RTL_USER_PROCESS_PARAMETERS *>(heapAlloc(sizeof(RTL_USER_PROCESS_PARAMETERS)));

	initializeUnicodeString(&pp.CurrentDirectoryPath, tempUS[0].Buffer, tempUS[0].Length);
	initializeUnicodeString(&pp.DllPath, tempUS[1].Buffer, tempUS[1].Length);
	initializeUnicodeString(&pp.ImagePathName, tempUS[2].Buffer, tempUS[2].Length);
	initializeUnicodeString(&pp.CommandLine, tempUS[3].Buffer, tempUS[3].Length);
	initializeUnicodeString(&pp.WindowTitle, tempUS[4].Buffer, tempUS[4].Length);
	initializeUnicodeString(&pp.DesktopName, tempUS[5].Buffer, tempUS[5].Length);
	initializeUnicodeString(&pp.ShellInfo, tempUS[6].Buffer, tempUS[6].Length);
	initializeUnicodeString(&pp.RuntimeData, tempUS[7].Buffer, tempUS[7].Length);
	pp.Environment = reinterpret_cast<wchar_t *>(heapAlloc(environmentLength));
	copyMemory(pp.Environment, environment, environmentLength);
	copyMemory(reinterpret_cast<uint8_t *>(myPEB_->ProcessParameters), reinterpret_cast<uint8_t *>(&pp), sizeof(RTL_USER_PROCESS_PARAMETERS));

	freeVirtual(buffers);
	freeVirtual(environment);
}

void *Win32NativeHelper::createHeap(size_t baseAddress)
{
	typedef void *(__stdcall *RtlCreateHeapPtr)(size_t Flags, void *HeapBase, size_t Reservesize, size_t Commitsize, void *Lock, void *Parameters);
	//flag 2: HEAP_GROWABLE
	return reinterpret_cast<RtlCreateHeapPtr>(rtlCreateHeap_)(2, reinterpret_cast<void *>(baseAddress), 0, 0, 0, 0);
}

void Win32NativeHelper::destroyHeap(void *heap)
{
	typedef void (__stdcall *RtlDestroyHeapPtr)(void *heap);
	reinterpret_cast<RtlDestroyHeapPtr>(rtlDestroyHeap_)(heap);
}

void *Win32NativeHelper::allocateHeap(size_t dwBytes)
{
	typedef void *(__stdcall *RtlAllocateHeapPtr)(void *hHeap, uint32_t dwFlags, size_t dwBytes);

	return reinterpret_cast<RtlAllocateHeapPtr>(rtlAllocateHeap_)(myPEB_->ProcessHeap, 0, dwBytes);
}

bool Win32NativeHelper::freeHeap(void *ptr)
{
	if(ptr > tempHeap && ptr < tempHeap + tempHeapPtr)
		return true;
	typedef bool (__stdcall *RtlFreeHeapPtr)(void *hHeap, uint32_t dwFlags, void *ptr);

	return reinterpret_cast<RtlFreeHeapPtr>(rtlFreeHeap_)(myPEB_->ProcessHeap, 0, ptr);
}

size_t Win32NativeHelper::sizeHeap(void *ptr)
{
	typedef size_t (__stdcall *RtlSizeHeapPtr)(void *hHeap, size_t flags, void *ptr);

	return reinterpret_cast<RtlSizeHeapPtr>(rtlSizeHeap_)(myPEB_->ProcessHeap, 0, ptr);
}

void *Win32NativeHelper::allocateVirtual(size_t desiredAddress, size_t RegionSize, size_t AllocationType, size_t Protect)
{
	typedef int32_t (__stdcall *NtAllocateVirtualMemoryPtr)(void *ProcessHandle, void **BaseAddress, size_t ZeroBits, size_t *RegionSize, size_t AllocationType, size_t Protect);

	if(init_ && desiredAddress == 0x00400000)
		freeVirtual(reinterpret_cast<void *>(0x00400000));

	void **result = reinterpret_cast<void **>(&desiredAddress);
	reinterpret_cast<NtAllocateVirtualMemoryPtr>(ntAllocateVirtualMemory_)(NtCurrentProcess(), result, 0, &RegionSize, AllocationType, Protect);
	
	return *result;
}

void Win32NativeHelper::freeVirtual(void *baseAddress)
{
	typedef int32_t (__stdcall *NtFreeVirtualMemoryPtr)(void *ProcessHandle, void **BaseAddress, size_t *RegionSize, size_t FreeType);

	size_t regionSize = 0;
	int result = reinterpret_cast<NtFreeVirtualMemoryPtr>(ntFreeVirtualMemory_)(NtCurrentProcess(), &baseAddress, &regionSize, MEM_RELEASE);
}

void Win32NativeHelper::protectVirtual(void *BaseAddress, size_t NumberOfBytes, size_t NewAccessProtection, size_t *OldAccessProtection)
{
	typedef int32_t (__stdcall *NtProtectVirtualMemoryPtr)(void *ProcessHandle, void **BaseAddress, size_t *NumberOfBytesToProtect, size_t NewAccessProtection, size_t *OldAccessProtection);

	reinterpret_cast<NtProtectVirtualMemoryPtr>(ntProtectVirtualMemory_)(NtCurrentProcess(), &BaseAddress, &NumberOfBytes, NewAccessProtection, OldAccessProtection);
}

void *Win32NativeHelper::createFile(uint32_t DesiredAccess, const wchar_t *Filename, size_t FilenameLength, size_t FileAttributes, size_t ShareAccess, size_t CreateDisposition, size_t CreateOptions)
{
	typedef int32_t (__stdcall *NtCreateFilePtr)(void **FileHandle, uint32_t DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, size_t FileAttributes, size_t ShareAccess, size_t CreateDisposition, size_t CreateOptions, void *EaBuffer, size_t EaLength);

	void *result = nullptr;
	OBJECT_ATTRIBUTES attributes;
	IO_STATUS_BLOCK statusBlock;
	UNICODE_STRING name;
	uint32_t desiredAccess = 0;
	
	//Prefix
	addPrefixToPath(&name, Filename, FilenameLength);

	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = OBJ_CASE_INSENSITIVE;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;

	if(DesiredAccess & GENERIC_READ)
		desiredAccess |= FILE_GENERIC_READ;
	if(desiredAccess & GENERIC_WRITE)
		desiredAccess |= FILE_GENERIC_WRITE;

	reinterpret_cast<NtCreateFilePtr>(ntCreateFile_)(&result, desiredAccess, &attributes, &statusBlock, nullptr, FileAttributes, ShareAccess, CreateDisposition, CreateOptions, nullptr, 0);

	freeUnicodeString(&name);

	return result;
}

void Win32NativeHelper::closeHandle(void *handle)
{
	typedef int32_t (__stdcall *NtClosePtr)(void *handle);

	reinterpret_cast<NtClosePtr>(ntClose_)(handle);
}

void *Win32NativeHelper::createSection(void *file, uint32_t flProtect, uint64_t sectionSize, wchar_t *lpName, size_t NameLength)
{
	typedef int32_t (__stdcall *NtCreateSectionPtr)(void **SectionHandle, uint32_t DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PLARGE_INTEGER MaximumSize, size_t SectionPageProtection, size_t AllocationAttributes, void *FileHandle);

	void *result;
	uint32_t desiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
	OBJECT_ATTRIBUTES attributes;
	PLARGE_INTEGER size = nullptr;
	LARGE_INTEGER localSize;
	uint32_t allocationAttributes = SEC_COMMIT;
	UNICODE_STRING name;

	name.Buffer = lpName;
	name.Length = NameLength * 2;
	name.MaximumLength = NameLength * 2;

	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = 0;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;

	if(sectionSize)
	{
		size = &localSize;
		localSize.QuadPart = sectionSize;
	}

	reinterpret_cast<NtCreateSectionPtr>(ntCreateSection_)(&result, SECTION_ALL_ACCESS, &attributes, size, flProtect, allocationAttributes, file);

	return result;
}

void *Win32NativeHelper::mapViewOfSection(void *section, uint32_t dwDesiredAccess, uint64_t offset, size_t dwNumberOfBytesToMap, size_t lpBaseAddress)
{
	typedef int32_t (__stdcall *NtMapViewOfSectionPtr)(void *SectionHandle, void *ProcessHandle, void **BaseAddress, uint32_t *ZeroBits, size_t CommitSize, PLARGE_INTEGER SectionOffset, size_t *ViewSize, uint32_t InheritDisposition, size_t AlllocationType, size_t AccessProtection);

	void **result = reinterpret_cast<void **>(&lpBaseAddress);
	LARGE_INTEGER sectionOffset;
	size_t viewSize;
	size_t protect;

	sectionOffset.QuadPart = offset;
	viewSize = dwNumberOfBytesToMap;
	if(dwDesiredAccess == FILE_MAP_COPY)
		protect = PAGE_WRITECOPY;
	else if(dwDesiredAccess & FILE_MAP_WRITE)
		protect = dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
	else if(dwDesiredAccess & FILE_MAP_READ)
		protect = dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READ : PAGE_READONLY;

	reinterpret_cast<NtMapViewOfSectionPtr>(ntMapViewOfSection_)(section, NtCurrentProcess(), result, 0, 0, &sectionOffset, &viewSize, 1, 0, protect);

	return *result;
}

void Win32NativeHelper::unmapViewOfSection(void *lpBaseAddress)
{
	typedef int32_t (__stdcall *NtUnmapViewOfSectionPtr)(void *ProcessHandle, void *BaseAddress);

	reinterpret_cast<NtUnmapViewOfSectionPtr>(ntUnmapViewOfSection_)(NtCurrentProcess(), lpBaseAddress);
}

uint32_t Win32NativeHelper::getFileAttributes(const wchar_t *filePath, size_t filePathLen)
{
	typedef int32_t (__stdcall *NtQueryFullAttributesFilePtr)(POBJECT_ATTRIBUTES ObjectAttributes, PFILE_NETWORK_OPEN_INFORMATION FileInformation);

	OBJECT_ATTRIBUTES attributes;
	UNICODE_STRING name;
	uint32_t desiredAccess = 0;
	FILE_NETWORK_OPEN_INFORMATION result;

	addPrefixToPath(&name, filePath, filePathLen);

	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = OBJ_CASE_INSENSITIVE;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;

	int32_t status = reinterpret_cast<NtQueryFullAttributesFilePtr>(ntQueryFullAttributesFile_)(&attributes, &result);

	freeUnicodeString(&name);

	if(status < 0)
		return INVALID_FILE_ATTRIBUTES;
	return result.FileAttributes;
}

wchar_t *Win32NativeHelper::getCommandLine()
{
	return myPEB_->ProcessParameters->CommandLine.Buffer;
}

wchar_t *Win32NativeHelper::getCurrentDirectory()
{
	return myPEB_->ProcessParameters->CurrentDirectoryPath.Buffer;
}

wchar_t *Win32NativeHelper::getEnvironments()
{
	return reinterpret_cast<wchar_t *>(myPEB_->ProcessParameters->Environment);
}

API_SET_HEADER *Win32NativeHelper::getApiSet()
{
	return myPEB_->ApiSet;
}

bool Win32NativeHelper::isInitialized()
{
	return init_;
}

PEB *Win32NativeHelper::getPEB()
{
	return myPEB_;
}

List<Win32LoadedImage> &Win32NativeHelper::getLoadedImages()
{
	return *loadedImages_;
}

void* operator new(size_t num)
{
	return heapAlloc(num);
}

void* operator new[](size_t num)
{
	return heapAlloc(num);
}

void operator delete(void *ptr)
{
	heapFree(ptr);
}

void operator delete[](void *ptr)
{
	heapFree(ptr);
}

extern "C"
{
	int _purecall()
	{
		return 0;
	}
#ifdef _DEBUG
	void *memset(void *dst, int val, size_t size)
	{
		for(size_t i = 0; i < size; i ++)
			*(reinterpret_cast<uint8_t *>(dst) + i) = static_cast<uint8_t>(val);
		return dst;
	}
#endif
}
