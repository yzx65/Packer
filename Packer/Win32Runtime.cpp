#include "Win32Runtime.h"

#include <cstdint>

#include "PEHeader.h"
#include "Win32Structure.h"
#include "PEFormat.h"
#include "String.h"
#include "Util.h"

#include <intrin.h>

//utilites
#define NtCurrentProcess() reinterpret_cast<void *>(-1)

int compareString(const char *a, const char *b)
{
	while(*a && *b && *a == *b) *a++, *b++;
	return *a - *b;
}

void initializeUnicodeString(UNICODE_STRING *string, wchar_t *data, size_t dataSize, size_t length = 0)
{
	if(length == 0)
		length = dataSize;
	string->Buffer = reinterpret_cast<wchar_t *>(Win32NativeHelper::get()->allocateHeap(length * 2 + 2));
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

//helper implementation
Win32NativeHelper g_helper;

Win32NativeHelper *Win32NativeHelper::get()
{
	if(g_helper.init_ == false)
		g_helper.init();
	return &g_helper;
}

void Win32NativeHelper::initNtdllImport(size_t exportDirectoryAddress)
{
	IMAGE_EXPORT_DIRECTORY *directory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(exportDirectoryAddress);

	uint32_t *addressOfFunctions = reinterpret_cast<uint32_t *>(ntdllBase_ + directory->AddressOfFunctions);
	uint32_t *addressOfNames = reinterpret_cast<uint32_t *>(ntdllBase_ + directory->AddressOfNames);
	uint16_t *ordinals = reinterpret_cast<uint16_t *>(ntdllBase_ + directory->AddressOfNameOrdinals);
	for(size_t i = 0; i < directory->NumberOfNames; i ++)
	{
		uint16_t ordinal = ordinals[i];
		size_t address = addressOfFunctions[ordinal];
		if(addressOfNames && addressOfNames[i])
		{
			const char *name = reinterpret_cast<const char *>(ntdllBase_ + addressOfNames[i]);

			if(compareString(name, "RtlAllocateHeap") == 0)
				rtlAllocateHeap_ = address + ntdllBase_;
			else if(compareString(name, "RtlFreeHeap") == 0)
				rtlFreeHeap_ = address + ntdllBase_;
			else if(compareString(name, "NtAllocateVirtualMemory") == 0)
				ntAllocateVirtualMemory_ = address + ntdllBase_;
			else if(compareString(name, "NtProtectVirtualMemory") == 0)
				ntProtectVirtualMemory_ = address + ntdllBase_;
			else if(compareString(name, "NtCreateFile") == 0)
				ntCreateFile_ = address + ntdllBase_;
			else if(compareString(name, "NtClose") == 0)
				ntClose_ = address + ntdllBase_;
			else if(compareString(name, "NtCreateSection") == 0)
				ntCreateSection_ = address + ntdllBase_;
			else if(compareString(name, "NtMapViewOfSection") == 0)
				ntMapViewOfSection_ = address + ntdllBase_;
			else if(compareString(name, "NtUnmapViewOfSection") == 0)
				ntUnmapViewOfSection_ = address + ntdllBase_;
			else if(compareString(name, "NtQueryFullAttributesFile") == 0)
				ntQueryFullAttributesFile_ = address + ntdllBase_;
		}
	}
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
	PEFormat format(reinterpret_cast<uint8_t *>(module->BaseAddress), true, false);
	IMAGE_DATA_DIRECTORY *dataDirectories = reinterpret_cast<IMAGE_DATA_DIRECTORY *>(format.getDataDirectories());

	initNtdllImport(ntdllBase_ + dataDirectories[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	init_ = true;
}

void *Win32NativeHelper::allocateHeap(size_t dwBytes)
{
	typedef void *(__stdcall *RtlAllocateHeapPtr)(void *hHeap, uint32_t dwFlags, size_t dwBytes);

	return reinterpret_cast<RtlAllocateHeapPtr>(rtlAllocateHeap_)(myPEB_->ProcessHeap, 0, dwBytes);
}

bool Win32NativeHelper::freeHeap(void *ptr)
{
	typedef bool (__stdcall *RtlFreeHeapPtr)(void *hHeap, uint32_t dwFlags, void *ptr);

	return reinterpret_cast<RtlFreeHeapPtr>(rtlFreeHeap_)(myPEB_->ProcessHeap, 0, ptr);
}

void *Win32NativeHelper::allocateVirtual(size_t RegionSize, size_t AllocationType, size_t Protect)
{
	typedef int32_t (__stdcall *NtAllocateVirtualMemoryPtr)(void *ProcessHandle, void **BaseAddress, size_t ZeroBits, size_t *RegionSize, size_t AllocationType, size_t Protect);
	
	void *result = nullptr;
	reinterpret_cast<NtAllocateVirtualMemoryPtr>(ntAllocateVirtualMemory_)(NtCurrentProcess(), &result, 0, &RegionSize, AllocationType, Protect);
	return result;
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

void *Win32NativeHelper::createSection(void *file, uint32_t flProtect, uint32_t dwMaximumSizeHigh, uint32_t dwMaximumSizeLow, wchar_t *lpName, size_t NameLength)
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

	if(dwMaximumSizeLow || dwMaximumSizeHigh)
	{
		size = &localSize;
		localSize.HighPart = dwMaximumSizeHigh;
		localSize.LowPart = dwMaximumSizeLow;
	}

	if(flProtect == PAGE_READWRITE)
		desiredAccess |= SECTION_MAP_WRITE;
	else if(flProtect == PAGE_EXECUTE_READWRITE)
		desiredAccess |= SECTION_MAP_WRITE | SECTION_MAP_EXECUTE;
	else if(flProtect == PAGE_EXECUTE_READ)
		desiredAccess |= SECTION_MAP_EXECUTE;

	reinterpret_cast<NtCreateSectionPtr>(ntCreateSection_)(&result, desiredAccess, &attributes, size, flProtect, allocationAttributes, file);

	return result;
}

void *Win32NativeHelper::mapViewOfSection(void *section, uint32_t dwDesiredAccess, uint32_t dwFileOffsetHigh, uint32_t dwFileOffsetLow, size_t dwNumberOfBytesToMap, void *lpBaseAddress)
{
	typedef int32_t (__stdcall *NtMapViewOfSectionPtr)(void *SectionHandle, void *ProcessHandle, void **BaseAddress, uint32_t *ZeroBits, size_t CommitSize, PLARGE_INTEGER SectionOffset, size_t *ViewSize, uint32_t InheritDisposition, size_t AlllocationType, size_t AccessProtection);

	void *result = lpBaseAddress;
	LARGE_INTEGER sectionOffset;
	size_t viewSize;
	size_t protect;

	sectionOffset.LowPart = dwFileOffsetLow;
	sectionOffset.HighPart = dwFileOffsetHigh;
	viewSize = dwNumberOfBytesToMap;
	if(dwDesiredAccess == FILE_MAP_COPY)
		protect = PAGE_WRITECOPY;
	else if(dwDesiredAccess & FILE_MAP_WRITE)
		protect = dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
	else if(dwDesiredAccess & FILE_MAP_READ)
		protect = dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READ : PAGE_READONLY;

	reinterpret_cast<NtMapViewOfSectionPtr>(ntMapViewOfSection_)(section, NtCurrentProcess(), &result, 0, 0, &sectionOffset, &viewSize, 1, 0, protect);

	return result;
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

size_t Win32NativeHelper::getNtdll()
{
	return ntdllBase_;
}

API_SET_HEADER *Win32NativeHelper::getApiSet()
{
	return myPEB_->ApiSet;
}

void* operator new(size_t num)
{
	return Win32NativeHelper::get()->allocateHeap(num);
}

void* operator new[](size_t num)
{
	return Win32NativeHelper::get()->allocateHeap(num);
}

void operator delete(void *ptr)
{
	Win32NativeHelper::get()->freeHeap(ptr);
}

void operator delete[](void *ptr)
{
	Win32NativeHelper::get()->freeHeap(ptr);
}

String getCommandLine()
{
	WString temp(Win32NativeHelper::get()->getCommandLine());
	return WStringToString(temp);
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
