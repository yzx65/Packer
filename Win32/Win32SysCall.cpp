#include "Win32SysCall.h"

#include <intrin.h>

#include "Win32Structure.h"
#include "../Util/Util.h"
#include "../Runtime/Allocator.h"

const uint16_t Win5152x64SystemCalls[SystemCallMax] ={0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x00ce, 0x0024, 0x0048, 0x0098, 0x0029}; //XP, Server 2003
const uint16_t Win60SP0x64SystemCalls[SystemCallMax] ={0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x0112, 0x0024, 0x0048, 0x00c1, 0x0029}; //Vista SP0
const uint16_t Win60SP1x64SystemCalls[SystemCallMax] ={0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x010d, 0x0024, 0x0048, 0x00bf, 0x0029}; //Vista SP1~, Server 2008
const uint16_t Win61x64SystemCalls[SystemCallMax] ={0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x0113, 0x0024, 0x0048, 0x00c2, 0x0029}; //7
const uint16_t Win62x64SystemCalls[SystemCallMax] ={0x0016, 0x001c, 0x004e, 0x0053, 0x0006, 0x000d, 0x0048, 0x0026, 0x0028, 0x0125, 0x0025, 0x0049, 0x00d4, 0x002a}; //Server 2012, 8
const uint16_t Win63x64SystemCalls[SystemCallMax] ={0x0017, 0x001d, 0x004f, 0x0054, 0x0007, 0x000e, 0x0049, 0x0027, 0x0029, 0x0128, 0x0026, 0x004a, 0x00d6, 0x002b}; //8.1

const uint16_t Win51x86SystemCalls[SystemCallMax] ={0x0011, 0x0053, 0x0089, 0x0025, 0x0112, 0x0019, 0x0032, 0x006c, 0x010b, 0x0095, 0x00e0, 0x004d, 0x004e, 0x0101}; //XP
const uint16_t Win52x86SystemCalls[SystemCallMax] ={0x0012, 0x0057, 0x008f, 0x0027, 0x011c, 0x001b, 0x0034, 0x0071, 0x0115, 0x009c, 0x00e9, 0x0051, 0x0052, 0x010a}; //Server 2003
const uint16_t Win60SP0x86SystemCalls[SystemCallMax] ={0x0012, 0x0093, 0x00d2, 0x003c, 0x0167, 0x0030, 0x004b, 0x00b1, 0x0160, 0x00df, 0x0131, 0x008c, 0x008d, 0x0152}; //Vista SP0
const uint16_t Win60SP1x86SystemCalls[SystemCallMax] ={0x0012, 0x0093, 0x00d2, 0x003c, 0x0163, 0x0030, 0x004b, 0x00b1, 0x015c, 0x00df, 0x012d, 0x008c, 0x008d, 0x014e}; //Vista SP1~, Server 2008
const uint16_t Win61x86SystemCalls[SystemCallMax] ={0x0013, 0x0083, 0x00d7, 0x0042, 0x018c, 0x0032, 0x0054, 0x00a8, 0x0181, 0x00e4, 0x0149, 0x007b, 0x007d, 0x0172}; //7
const uint16_t Win62x86SystemCalls[SystemCallMax] ={0x0196, 0x0118, 0x00c3, 0x0163, 0x0005, 0x0174, 0x0150, 0x00f3, 0x0013, 0x00b6, 0x004e, 0x0120, 0x011e, 0x0023}; //8
const uint16_t Win63x86SystemCalls[SystemCallMax] ={0x019b, 0x011c, 0x00c6, 0x0168, 0x0006, 0x0179, 0x0154, 0x00f6, 0x0013, 0x00b9, 0x0051, 0x0124, 0x0122, 0x0023}; //8.1

void * operator new (size_t, void *p) throw() { return p ; }
//utilites
#define NtCurrentProcess() 0xffffffff
#define NtCurrentProcess64() 0xffffffffffffffff

template<typename UnicodeStringType = UNICODE_STRING>
void copyUnicodeString(UnicodeStringType *dest, UnicodeStringType *src)
{
	dest->Length = src->Length;
	dest->MaximumLength = src->Length;
	if(!src->Buffer)
		dest->Buffer = nullptr;
	else
		copyMemory(dest->Buffer, src->Buffer, sizeof(wchar_t)* src->Length + 2);
}

template<typename UnicodeStringType = UNICODE_STRING>
void initializeUnicodeString(UnicodeStringType *string, wchar_t *data, size_t dataSize, size_t length = 0)
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

template<typename UnicodeStringType = UNICODE_STRING>
void freeUnicodeString(UnicodeStringType *string)
{
	heapFree(string->Buffer);
}

template<typename UnicodeStringType = UNICODE_STRING>
void getPrefixedPathUnicodeString(UnicodeStringType *string, const wchar_t *path, size_t pathSize)
{
	initializeUnicodeString(string, L"\\??\\", 4, pathSize + 4);
	for(size_t i = 4; i < pathSize + 4; i ++)
		string->Buffer[i] = path[i - 4];
	string->Buffer[pathSize + 4] = 0;
	string->Length = pathSize * 2 + 8;
}

void normalizeCreateFileOptions(uint32_t *resultDesiredAccess, uint32_t *resultCreateOptions, uint32_t DesiredAccess)
{
	*resultDesiredAccess = *resultCreateOptions = 0;

	if(DesiredAccess & GENERIC_READ)
		*resultDesiredAccess |= FILE_GENERIC_READ;
	if(DesiredAccess & GENERIC_WRITE)
	{
		*resultDesiredAccess |= FILE_GENERIC_WRITE;
		*resultCreateOptions = 0x00000010; //FILE_SYNCHRONOUS_IO_ALERT;
	}
}

Win32SystemCaller *Win32SystemCaller::get()
{
	static bool initialized = false;
	static uint8_t callerStorage[sizeof(Win32SystemCaller)];

	Win32SystemCaller *result;
	if(initialized)
		result = reinterpret_cast<Win32SystemCaller *>(callerStorage);
	else
	{
		PEB *peb = reinterpret_cast<PEB *>(__readfsdword(0x30));
		KUSER_SHARED_DATA *sharedData = reinterpret_cast<KUSER_SHARED_DATA *>(0x7ffe0000);
	
		const uint16_t *systemCalls_ = nullptr;
		if(__readfsdword(0xC0) != 0)
		{
			if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 3)
				systemCalls_ = Win63x64SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 2)
				systemCalls_ = Win62x64SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 1)
				systemCalls_ = Win61x64SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0 && peb->OSBuildNumber == 6000)
				systemCalls_ = Win60SP0x64SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0)
				systemCalls_ = Win60SP1x64SystemCalls;
			else if(sharedData->NtMajorVersion == 5)
				systemCalls_ = Win5152x64SystemCalls;

			result = new (callerStorage)Win32WOW64SystemCaller(systemCalls_);
		}
		else
		{
			if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 3)
				systemCalls_ = Win63x86SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 2)
				systemCalls_ = Win62x86SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 1)
				systemCalls_ = Win61x86SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0 && peb->OSBuildNumber == 6000)
				systemCalls_ = Win60SP0x86SystemCalls;
			else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0)
				systemCalls_ = Win60SP1x86SystemCalls;
			else if(sharedData->NtMajorVersion == 5 && sharedData->NtMinorVersion == 1)
				systemCalls_ = Win51x86SystemCalls;
			else if(sharedData->NtMajorVersion == 5 && sharedData->NtMinorVersion == 2)
				systemCalls_ = Win52x86SystemCalls;

			result = new (callerStorage)Win32x86SystemCaller(systemCalls_);
		}
		initialized = true;
	}
	
	return result;
}

uint32_t __declspec(naked) Win32SystemCaller::executeWin32Syscall(uint32_t syscallno, uint32_t *argv)
{
	_asm
	{
		push ebp
		mov ebp, esp
		push ebx

		mov ebx, esp
		mov esp, dword ptr[argv] //setup stack as if we called ntdll function directly.
		sub esp, 4

		mov eax, syscallno
		mov edx, 0x7ffe0300 //SharedUserData!SystemCallStub
		call[edx]

		mov esp, ebx

		pop ebx
		pop ebp
		ret
	}
}

uint32_t __declspec(naked) Win32SystemCaller::executeWoW64Syscall(uint32_t syscallno, uint64_t *argv)
{
	_asm
	{
		push ebp
		mov ebp, esp
		push ebx

		push 0x33
		push offset x64
		call fword ptr[esp] //jump to x64 mode
		add esp, 8

		pop ebx
		pop ebp
		ret
x64:
		mov eax, syscallno

		dec eax //this opcode will act as 64bit prefix.
		mov ebx, esp //mov rbx, rsp

		mov esp, dword ptr[argv]
		sub esp, 8

		dec esp //setup stack and register as if we called ntdll function directly.
		mov edx, [esp + 8] //mov r10, a1
		dec eax
		mov edx, [esp + 16] //mov rdx, a2
		dec esp
		mov eax, [esp + 24] //mov r8, a3
		dec esp
		mov ecx, [esp + 32] //mov r9, a4

		__asm __emit 0x0f __asm __emit 0x05 //syscall
		dec eax
		mov esp, ebx //mov rsp, rbx

		retf //return to x32
	}
}

Win32WOW64SystemCaller::Win32WOW64SystemCaller(const uint16_t *systemCalls) : systemCalls_(systemCalls) {}

void *Win32WOW64SystemCaller::allocateVirtual(size_t DesiredAddress, size_t RegionSize, size_t AllocationType, size_t Protect)
{
	uint64_t address = DesiredAddress;
	uint64_t size = RegionSize;
	uint64_t args[] = {NtCurrentProcess64(), reinterpret_cast<uint64_t>(&address), 0x7ffeffff, reinterpret_cast<uint64_t>(&size), AllocationType, Protect};
	//passing big ZeroBits acts as bit mask.
	uint32_t result = executeWoW64Syscall(systemCalls_[NtAllocateVirtualMemory], args);
	if(result < 0)
		return 0;
	return reinterpret_cast<void *>(address);
}

bool Win32WOW64SystemCaller::freeVirtual(void *BaseAddress)
{
	uint64_t RegionSize = 0;
	uint64_t args[] = {NtCurrentProcess64(), reinterpret_cast<uint64_t>(&BaseAddress), reinterpret_cast<uint64_t>(&RegionSize), MEM_RELEASE};
	uint32_t result = executeWoW64Syscall(systemCalls_[NtFreeVirtualMemory], args);
	
	if(result < 0)
		return false;
	return true;
}

void Win32WOW64SystemCaller::protectVirtual(void *BaseAddress, size_t NumberOfBytes, size_t NewAccessProtection, size_t *OldAccessProtection)
{
	uint64_t temp;
	if(!OldAccessProtection)
		OldAccessProtection = reinterpret_cast<size_t *>(&temp);

	uint64_t baseAddress64 = reinterpret_cast<uint64_t>(BaseAddress);
	uint64_t numberOfBytes64 = NumberOfBytes;
	uint64_t args[] ={NtCurrentProcess64(), reinterpret_cast<uint64_t>(&baseAddress64), reinterpret_cast<uint64_t>(&numberOfBytes64),
		NewAccessProtection, reinterpret_cast<uint64_t>(OldAccessProtection)};
	executeWoW64Syscall(systemCalls_[NtProtectVirtualMemory], args);	
}

void *Win32WOW64SystemCaller::createFile(uint32_t DesiredAccess, const wchar_t *Filename, size_t FilenameLength, size_t ShareAccess, size_t CreateDisposition)
{
	uint64_t __declspec(align(8)) result = 0;
	uint32_t desiredAccess = 0;
	uint32_t createOptions = 0;

	normalizeCreateFileOptions(&desiredAccess, &createOptions, DesiredAccess);

	OBJECT_ATTRIBUTES64 attributes ={0, };
	IO_STATUS_BLOCK64 statusBlock ={0, };
	UNICODE_STRING64 name ={0, };
	getPrefixedPathUnicodeString(&name, Filename, FilenameLength);

	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = OBJ_CASE_INSENSITIVE;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;

	uint64_t args[] ={reinterpret_cast<uint64_t>(&result), desiredAccess, reinterpret_cast<uint64_t>(&attributes), reinterpret_cast<uint64_t>(&statusBlock),
		0, 0, ShareAccess, CreateDisposition, createOptions, 0, 0};
	executeWoW64Syscall(systemCalls_[NtCreateFile], args);

	freeUnicodeString(&name);

	return reinterpret_cast<void *>(result);
}

void Win32WOW64SystemCaller::flushFile(void *fileHandle)
{
	IO_STATUS_BLOCK64 statusBlock;

	uint64_t args[] ={reinterpret_cast<uint64_t>(fileHandle), reinterpret_cast<uint64_t>(&statusBlock)};
	executeWoW64Syscall(systemCalls_[NtFlushBuffersFile], args);
}

size_t Win32WOW64SystemCaller::writeFile(void *fileHandle, const uint8_t *buffer, size_t bufferSize)
{
	IO_STATUS_BLOCK64 statusBlock;
	uint64_t args[] ={reinterpret_cast<uint64_t>(fileHandle), 0, 0, 0, reinterpret_cast<uint64_t>(&statusBlock), reinterpret_cast<uint64_t>(buffer), bufferSize, 0, 0};
	executeWoW64Syscall(systemCalls_[NtWriteFile], args);

	return reinterpret_cast<size_t>(statusBlock.Information);
}

void Win32WOW64SystemCaller::closeHandle(void *handle)
{
	uint64_t args[] ={reinterpret_cast<uint64_t>(handle)};
	executeWoW64Syscall(systemCalls_[NtClose], args);
}

void *Win32WOW64SystemCaller::createSection(void *file, uint32_t flProtect, uint64_t sectionSize, wchar_t *lpName, size_t NameLength)
{
	uint64_t __declspec(align(8)) result;
	PLARGE_INTEGER size = nullptr;
	LARGE_INTEGER localSize;

	if(sectionSize)
	{
		size = &localSize;
		localSize.QuadPart = sectionSize;
	}

	OBJECT_ATTRIBUTES64 attributes ={0, };
	UNICODE_STRING64 name ={0, };

	name.Buffer = lpName;
	name.Length = NameLength * 2;
	name.MaximumLength = NameLength * 2;

	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = 0;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;

	uint64_t args[] ={reinterpret_cast<uint64_t>(&result), SECTION_ALL_ACCESS, reinterpret_cast<uint64_t>(&attributes),
		reinterpret_cast<uint64_t>(size), flProtect, SEC_COMMIT, reinterpret_cast<uint64_t>(file)};
	executeWoW64Syscall(systemCalls_[NtCreateSection], args);

	return reinterpret_cast<void *>(result);
}

uint32_t normalizeMapViewOfSectionOptions(uint32_t dwDesiredAccess)
{
	if(dwDesiredAccess == FILE_MAP_COPY)
		return PAGE_WRITECOPY;
	else if(dwDesiredAccess & FILE_MAP_WRITE)
		return dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
	else if(dwDesiredAccess & FILE_MAP_READ)
		return dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READ : PAGE_READONLY;
	return 0;
}

void *Win32WOW64SystemCaller::mapViewOfSection(void *section, uint32_t dwDesiredAccess, uint64_t offset, size_t dwNumberOfBytesToMap, size_t lpBaseAddress)
{
	uint64_t result = lpBaseAddress;
	LARGE_INTEGER sectionOffset;
	uint64_t viewSize;
	size_t protect = normalizeMapViewOfSectionOptions(dwDesiredAccess);

	sectionOffset.QuadPart = offset;
	viewSize = dwNumberOfBytesToMap;
	
	uint64_t args[] ={reinterpret_cast<uint64_t>(section), NtCurrentProcess64(), reinterpret_cast<uint64_t>(&result), 0x7ffeffff, 0,
		reinterpret_cast<uint64_t>(&sectionOffset), reinterpret_cast<uint64_t>(&viewSize), 1, 0, protect};
	executeWoW64Syscall(systemCalls_[NtMapViewOfSection], args);

	return reinterpret_cast<void *>(result);
}

void Win32WOW64SystemCaller::unmapViewOfSection(void *lpBaseAddress)
{
	uint64_t args[] ={NtCurrentProcess64(), reinterpret_cast<uint64_t>(lpBaseAddress)};
	executeWoW64Syscall(systemCalls_[NtUnmapViewOfSection], args);
}

uint32_t Win32WOW64SystemCaller::getFileAttributes(const wchar_t *filePath, size_t filePathLen)
{
	uint32_t desiredAccess = 0;
	FILE_NETWORK_OPEN_INFORMATION result;
	int32_t status;

	OBJECT_ATTRIBUTES64 attributes ={0, };
	UNICODE_STRING64 name ={0, };

	getPrefixedPathUnicodeString(&name, filePath, filePathLen);
	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = OBJ_CASE_INSENSITIVE;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;

	uint64_t args[] ={reinterpret_cast<uint64_t>(&attributes), reinterpret_cast<uint64_t>(&result)};
	status = executeWoW64Syscall(systemCalls_[NtQueryFullAttributesFile], args);

	freeUnicodeString(&name);

	if(status < 0)
		return INVALID_FILE_ATTRIBUTES;
	return result.FileAttributes;
}

void Win32WOW64SystemCaller::setFileSize(void *file, uint64_t size)
{
	LARGE_INTEGER largeSize;
	largeSize.QuadPart = size;

	IO_STATUS_BLOCK64 result;
	uint64_t args[] ={reinterpret_cast<uint64_t>(file), reinterpret_cast<uint64_t>(&result), reinterpret_cast<uint64_t>(&largeSize), sizeof(largeSize), 20};//FileEndOfFileInfo
	executeWoW64Syscall(systemCalls_[NtSetInformationFile], args);
	args[4] = 19;//FileAllocationInfo
	executeWoW64Syscall(systemCalls_[NtSetInformationFile], args);
}

void Win32WOW64SystemCaller::flushInstructionCache(size_t offset, size_t size)
{
	uint64_t args[] ={NtCurrentProcess64(), offset, size};
	executeWoW64Syscall(systemCalls_[NtFlushInstructionCache], args);
}

void Win32WOW64SystemCaller::terminate()
{
	uint64_t args[] ={NtCurrentProcess64(), 0};
	executeWoW64Syscall(systemCalls_[NtTerminateProcess], args);
}

Win32x86SystemCaller::Win32x86SystemCaller(const uint16_t *systemCalls) : systemCalls_(systemCalls) {}

void *Win32x86SystemCaller::allocateVirtual(size_t DesiredAddress, size_t RegionSize, size_t AllocationType, size_t Protect)
{
	uint32_t address = DesiredAddress;
	uint32_t size = RegionSize;
	uint32_t args[] ={NtCurrentProcess(), reinterpret_cast<uint32_t>(&address), 0, reinterpret_cast<uint32_t>(&size), AllocationType, Protect};
	uint32_t result = executeWin32Syscall(systemCalls_[NtAllocateVirtualMemory], args);

	if(result < 0)
		return 0;
	return reinterpret_cast<void *>(address);
}

bool Win32x86SystemCaller::freeVirtual(void *BaseAddress)
{
	uint32_t RegionSize = 0;
	uint32_t args[] ={NtCurrentProcess(), reinterpret_cast<uint32_t>(&BaseAddress), reinterpret_cast<uint32_t>(&RegionSize), MEM_RELEASE};
	uint32_t result = executeWin32Syscall(systemCalls_[NtFreeVirtualMemory], args);
	if(result < 0)
		return false;
	return true;
}

void Win32x86SystemCaller::protectVirtual(void *BaseAddress, size_t NumberOfBytes, size_t NewAccessProtection, size_t *OldAccessProtection)
{
	uint32_t temp;
	if(!OldAccessProtection)
		OldAccessProtection = reinterpret_cast<size_t *>(&temp);
	
	uint32_t args[] ={NtCurrentProcess(), reinterpret_cast<uint32_t>(&BaseAddress), reinterpret_cast<uint32_t>(&NumberOfBytes),
		NewAccessProtection, reinterpret_cast<uint32_t>(OldAccessProtection)};
	executeWin32Syscall(systemCalls_[NtProtectVirtualMemory], args);
}

void *Win32x86SystemCaller::createFile(uint32_t DesiredAccess, const wchar_t *Filename, size_t FilenameLength, size_t ShareAccess, size_t CreateDisposition)
{
	uint32_t result = 0;
	uint32_t desiredAccess = 0;
	uint32_t createOptions = 0;

	normalizeCreateFileOptions(&desiredAccess, &createOptions, DesiredAccess);

	OBJECT_ATTRIBUTES attributes ={0, };
	IO_STATUS_BLOCK statusBlock ={0, };
	UNICODE_STRING name ={0, };
	getPrefixedPathUnicodeString(&name, Filename, FilenameLength);

	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = OBJ_CASE_INSENSITIVE;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;

	uint32_t args[] ={reinterpret_cast<uint32_t>(&result), desiredAccess, reinterpret_cast<uint32_t>(&attributes), reinterpret_cast<uint32_t>(&statusBlock),
		0, 0, ShareAccess, CreateDisposition, createOptions, 0, 0};
	executeWin32Syscall(systemCalls_[NtCreateFile], args);

	freeUnicodeString(&name);

	return reinterpret_cast<void *>(result);
}

void Win32x86SystemCaller::flushFile(void *fileHandle)
{
	IO_STATUS_BLOCK statusBlock;

	uint32_t args[] ={reinterpret_cast<uint32_t>(fileHandle), reinterpret_cast<uint32_t>(&statusBlock)};
	executeWin32Syscall(systemCalls_[NtFlushBuffersFile], args);
}

size_t Win32x86SystemCaller::writeFile(void *fileHandle, const uint8_t *buffer, size_t bufferSize)
{
	IO_STATUS_BLOCK statusBlock;
	uint32_t args[] ={reinterpret_cast<uint32_t>(fileHandle), 0, 0, 0, reinterpret_cast<uint32_t>(&statusBlock), reinterpret_cast<uint32_t>(buffer), bufferSize, 0, 0};
	executeWin32Syscall(systemCalls_[NtWriteFile], args);

	return reinterpret_cast<size_t>(statusBlock.Information);
}

void Win32x86SystemCaller::closeHandle(void *handle)
{
	uint32_t args[] ={reinterpret_cast<uint32_t>(handle)};
	executeWin32Syscall(systemCalls_[NtClose], args);
}

void *Win32x86SystemCaller::createSection(void *file, uint32_t flProtect, uint64_t sectionSize, wchar_t *lpName, size_t NameLength)
{
	uint32_t  result;
	PLARGE_INTEGER size = nullptr;
	LARGE_INTEGER localSize;

	if(sectionSize)
	{
		size = &localSize;
		localSize.QuadPart = sectionSize;
	}

	OBJECT_ATTRIBUTES attributes;
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

	uint32_t args[] ={reinterpret_cast<uint32_t>(&result), SECTION_ALL_ACCESS, reinterpret_cast<uint32_t>(&attributes),
		reinterpret_cast<uint32_t>(size), flProtect, SEC_COMMIT, reinterpret_cast<uint32_t>(file)};
	executeWin32Syscall(systemCalls_[NtCreateSection], args);

	return reinterpret_cast<void *>(result);
}

void *Win32x86SystemCaller::mapViewOfSection(void *section, uint32_t dwDesiredAccess, uint64_t offset, size_t dwNumberOfBytesToMap, size_t lpBaseAddress)
{
	uint32_t result = lpBaseAddress;
	LARGE_INTEGER sectionOffset;
	uint32_t viewSize;
	size_t protect = normalizeMapViewOfSectionOptions(dwDesiredAccess);

	uint32_t args[] ={reinterpret_cast<uint32_t>(section), NtCurrentProcess(), reinterpret_cast<uint32_t>(&result), 0, 0,
		reinterpret_cast<uint32_t>(&sectionOffset), reinterpret_cast<uint32_t>(&viewSize), 1, 0, protect};
	executeWin32Syscall(systemCalls_[NtMapViewOfSection], args);

	return reinterpret_cast<void *>(result);
}

void Win32x86SystemCaller::unmapViewOfSection(void *lpBaseAddress)
{
	uint32_t args[] ={NtCurrentProcess(), reinterpret_cast<uint32_t>(lpBaseAddress)};
	executeWin32Syscall(systemCalls_[NtUnmapViewOfSection], args);
}

uint32_t Win32x86SystemCaller::getFileAttributes(const wchar_t *filePath, size_t filePathLen)
{
	uint32_t desiredAccess = 0;
	FILE_NETWORK_OPEN_INFORMATION result;
	int32_t status;

	OBJECT_ATTRIBUTES attributes;
	UNICODE_STRING name;

	getPrefixedPathUnicodeString(&name, filePath, filePathLen);
	attributes.Length = sizeof(attributes);
	attributes.RootDirectory = nullptr;
	attributes.ObjectName = &name;
	attributes.Attributes = OBJ_CASE_INSENSITIVE;
	attributes.SecurityDescriptor = nullptr;
	attributes.SecurityQualityOfService = nullptr;
	uint32_t args[] ={reinterpret_cast<uint32_t>(&attributes), reinterpret_cast<uint32_t>(&result)};

	status = executeWin32Syscall(systemCalls_[NtQueryFullAttributesFile], args);

	freeUnicodeString(&name);

	if(status < 0)
		return INVALID_FILE_ATTRIBUTES;
	return result.FileAttributes;
}

void Win32x86SystemCaller::setFileSize(void *file, uint64_t size)
{
	LARGE_INTEGER largeSize;
	largeSize.QuadPart = size;

	IO_STATUS_BLOCK result;
	uint32_t args[] ={reinterpret_cast<uint32_t>(file), reinterpret_cast<uint32_t>(&result), reinterpret_cast<uint32_t>(&largeSize), sizeof(largeSize), 20};
	executeWin32Syscall(systemCalls_[NtSetInformationFile], args);
	args[4] = 19;
	executeWin32Syscall(systemCalls_[NtSetInformationFile], args);
}

void Win32x86SystemCaller::flushInstructionCache(size_t offset, size_t size)
{
	uint32_t args[] ={NtCurrentProcess(), offset, size};
	executeWin32Syscall(systemCalls_[NtFlushInstructionCache], args);
}

void Win32x86SystemCaller::terminate()
{
	uint32_t args[] ={NtCurrentProcess(), 0};
	executeWin32Syscall(systemCalls_[NtTerminateProcess], args);
}
