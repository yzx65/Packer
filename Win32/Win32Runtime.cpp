#include "Win32Runtime.h"

#include <cstdint>

#include "../Util/String.h"
#include "../Util/Util.h"
#include "../Runtime/Allocator.h"
#include "Win32Structure.h"

#include <intrin.h>

const uint16_t Win5152x64SystemCalls[SystemCallMax] =  {0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x00ce, 0x0024, 0x0048, 0x0098}; //XP, Server 2003
const uint16_t Win60SP0x64SystemCalls[SystemCallMax] = {0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x0112, 0x0024, 0x0048, 0x00c1}; //Vista SP0
const uint16_t Win60SP1x64SystemCalls[SystemCallMax] = {0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x010d, 0x0024, 0x0048, 0x00bf}; //Vista SP1~, Server 2008
const uint16_t Win61x64SystemCalls[SystemCallMax] =    {0x0015, 0x001b, 0x004d, 0x0052, 0x0005, 0x000c, 0x0047, 0x0025, 0x0027, 0x0113, 0x0024, 0x0048, 0x00c2}; //7
const uint16_t Win62x64SystemCalls[SystemCallMax] =    {0x0016, 0x001c, 0x004e, 0x0053, 0x0006, 0x000d, 0x0048, 0x0026, 0x0028, 0x0125, 0x0025, 0x0049, 0x00d4}; //Server 2012, 8
const uint16_t Win63x64SystemCalls[SystemCallMax] =    {0x0017, 0x001d, 0x004f, 0x0054, 0x0007, 0x000e, 0x0049, 0x0027, 0x0029, 0x0128, 0x0026, 0x004a, 0x00d6}; //8.1

const uint16_t Win51x86SystemCalls[SystemCallMax] =    {0x0011, 0x0053, 0x0089, 0x0025, 0x0112, 0x0019, 0x0032, 0x006c, 0x010b, 0x0095, 0x00e0, 0x004d, 0x004e}; //XP
const uint16_t Win52x86SystemCalls[SystemCallMax] =    {0x0012, 0x0057, 0x008f, 0x0027, 0x011c, 0x001b, 0x0034, 0x0071, 0x0115, 0x009c, 0x00e9, 0x0051, 0x0052}; //Server 2003
const uint16_t Win60SP0x86SystemCalls[SystemCallMax] = {0x0012, 0x0093, 0x00d2, 0x003c, 0x0167, 0x0030, 0x004b, 0x00b1, 0x0160, 0x00df, 0x0131, 0x008c, 0x008d}; //Vista SP0
const uint16_t Win60SP1x86SystemCalls[SystemCallMax] = {0x0012, 0x0093, 0x00d2, 0x003c, 0x0163, 0x0030, 0x004b, 0x00b1, 0x015c, 0x00df, 0x012d, 0x008c, 0x008d}; //Vista SP1~, Server 2008
const uint16_t Win61x86SystemCalls[SystemCallMax] =    {0x0013, 0x0083, 0x00d7, 0x0042, 0x018c, 0x0032, 0x0054, 0x00a8, 0x0181, 0x00e4, 0x0149, 0x007b, 0x007d}; //7
const uint16_t Win62x86SystemCalls[SystemCallMax] =    {0x0196, 0x0118, 0x00c3, 0x0163, 0x0005, 0x0174, 0x0150, 0x00f3, 0x0013, 0x00b6, 0x004e, 0x0120, 0x011e}; //8
const uint16_t Win63x86SystemCalls[SystemCallMax] =    {0x019b, 0x011c, 0x00c6, 0x0168, 0x0006, 0x0179, 0x0154, 0x00f6, 0x0013, 0x00b9, 0x0051, 0x0124, 0x0122}; //8.1

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
		copyMemory(dest->Buffer, src->Buffer, sizeof(wchar_t) * src->Length + 2);
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

Win32NativeHelper *Win32NativeHelper::get()
{
	static Win32NativeHelper helper;
	return &helper;
}

uint32_t __declspec(naked) Win32NativeHelper::executeWin32Syscall(uint32_t syscallno, uint32_t *argv)
{
	_asm
	{
		push ebp
		mov ebp, esp

		mov ecx, esp
		mov esp, dword ptr [argv] //setup stack as if we called ntdll function directly.
		sub esp, 4

		mov eax, syscallno
		mov edx, 0x7ffe0300 //SharedUserData!SystemCallStub
		call [edx]

		mov esp, ecx

		pop ebp
		ret
	}
}

uint32_t __declspec(naked) Win32NativeHelper::executeWoW64Syscall(uint32_t syscallno, uint64_t *argv)
{
	_asm
	{
		push ebp
		mov ebp, esp
		push ebx

		push 0x33
		push offset x64
		call fword ptr [esp] //jump to x64 mode
		add esp, 8

		pop ebx
		pop ebp
		ret
x64:
		mov eax, syscallno

		dec eax //this opcode will act as 64bit prefix.
		mov ebx, esp //mov rbx, rsp

		mov esp, dword ptr [argv]
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

void Win32NativeHelper::init()
{
	myPEB_ = reinterpret_cast<PEB *>(__readfsdword(0x30));
	myBase_ = reinterpret_cast<size_t>(myPEB_->ImageBaseAddress);

	KUSER_SHARED_DATA *sharedData = reinterpret_cast<KUSER_SHARED_DATA *>(0x7ffe0000);
	systemCalls_ = nullptr;
	if(__readfsdword(0xC0) != 0)
	{
		isWoW64_ = true;
		if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 3)
			systemCalls_ = Win63x64SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 2)
			systemCalls_ = Win62x64SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 1)
			systemCalls_ = Win61x64SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0 && myPEB_->OSBuildNumber == 6000)
			systemCalls_ = Win60SP0x64SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0)
			systemCalls_ = Win60SP1x64SystemCalls;
		else if(sharedData->NtMajorVersion == 5)
			systemCalls_ = Win5152x64SystemCalls;
	}
	else
	{
		isWoW64_ = false;
		if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 3)
			systemCalls_ = Win63x86SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 2)
			systemCalls_ = Win62x86SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 1)
			systemCalls_ = Win61x86SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0 && myPEB_->OSBuildNumber == 6000)
			systemCalls_ = Win60SP0x86SystemCalls;
		else if(sharedData->NtMajorVersion == 6 && sharedData->NtMinorVersion == 0)
			systemCalls_ = Win60SP1x86SystemCalls;
		else if(sharedData->NtMajorVersion == 5 && sharedData->NtMinorVersion == 1)
			systemCalls_ = Win51x86SystemCalls;
		else if(sharedData->NtMajorVersion == 5 && sharedData->NtMinorVersion == 2)
			systemCalls_ = Win52x86SystemCalls;
	}
}

size_t Win32NativeHelper::getMyBase() const
{
	return myBase_;
}

void Win32NativeHelper::setMyBase(size_t address)
{
	myBase_ = address;;
	myPEB_->ImageBaseAddress = reinterpret_cast<void *>(address);
}

void *Win32NativeHelper::allocateVirtual(size_t DesiredAddress, size_t RegionSize, size_t AllocationType, size_t Protect)
{
	uint64_t address = DesiredAddress;
	uint64_t size = RegionSize;
	uint32_t result;
	if(isWoW64_)
	{
		uint64_t args[] = {NtCurrentProcess64(), reinterpret_cast<uint64_t>(&address), 0x7ffeffff, reinterpret_cast<uint64_t>(&size), AllocationType, Protect};
		//passing big ZeroBits acts as bit mask.
		result = executeWoW64Syscall(systemCalls_[NtAllocateVirtualMemory], args);
	}
	else
	{
		uint32_t args[] = {NtCurrentProcess(), reinterpret_cast<uint32_t>(&address), 0, reinterpret_cast<uint32_t>(&size), AllocationType, Protect};
		result = executeWin32Syscall(systemCalls_[NtAllocateVirtualMemory], args);
	}
	if(result < 0)
		return 0;
	return reinterpret_cast<void *>(address);
}

bool Win32NativeHelper::freeVirtual(void *BaseAddress)
{
	uint64_t RegionSize = 0;
	uint32_t result;
	if(isWoW64_)
	{
		uint64_t args[] = {NtCurrentProcess64(), reinterpret_cast<uint64_t>(&BaseAddress), reinterpret_cast<uint64_t>(&RegionSize), MEM_RELEASE};
		result = executeWoW64Syscall(systemCalls_[NtFreeVirtualMemory], args);
	}
	else
	{
		uint32_t args[] = {NtCurrentProcess(), reinterpret_cast<uint32_t>(&BaseAddress), reinterpret_cast<uint32_t>(&RegionSize), MEM_RELEASE};
		result = executeWin32Syscall(systemCalls_[NtFreeVirtualMemory], args);
	}
	if(result < 0)
		return false;
	return true;
}

void Win32NativeHelper::protectVirtual(void *BaseAddress, size_t NumberOfBytes, size_t NewAccessProtection, size_t *OldAccessProtection)
{
	uint64_t temp;
	if(!OldAccessProtection)
		OldAccessProtection = reinterpret_cast<size_t *>(&temp);
	if(isWoW64_)
	{
		uint64_t baseAddress64 = reinterpret_cast<uint64_t>(BaseAddress);
		uint64_t numberOfBytes64 = NumberOfBytes;
		uint64_t args[] = {NtCurrentProcess64(), reinterpret_cast<uint64_t>(&baseAddress64), reinterpret_cast<uint64_t>(&numberOfBytes64), 
			NewAccessProtection, reinterpret_cast<uint64_t>(OldAccessProtection)};
		executeWoW64Syscall(systemCalls_[NtProtectVirtualMemory], args);
	}
	else
	{
		uint32_t args[] = {NtCurrentProcess(), reinterpret_cast<uint32_t>(&BaseAddress), reinterpret_cast<uint32_t>(&NumberOfBytes), 
			NewAccessProtection, reinterpret_cast<uint32_t>(OldAccessProtection)};
		executeWin32Syscall(systemCalls_[NtProtectVirtualMemory], args);
	}
}

void *Win32NativeHelper::createFile(uint32_t DesiredAccess, const wchar_t *Filename, size_t FilenameLength, size_t ShareAccess, size_t CreateDisposition)
{
	uint64_t __declspec(align(8)) result = 0;
	uint32_t desiredAccess = 0;
	uint32_t createOptions = 0;

	if(DesiredAccess & GENERIC_READ)
		desiredAccess |= FILE_GENERIC_READ;
	if(DesiredAccess & GENERIC_WRITE)
	{
		desiredAccess |= FILE_GENERIC_WRITE;
		createOptions = 0x00000010; //FILE_SYNCHRONOUS_IO_ALERT;
	}

	if(isWoW64_)
	{
		OBJECT_ATTRIBUTES64 attributes = {0, };
		IO_STATUS_BLOCK64 statusBlock = {0, };
		UNICODE_STRING64 name = {0, };
		getPrefixedPathUnicodeString(&name, Filename, FilenameLength);

		attributes.Length = sizeof(attributes);
		attributes.RootDirectory = nullptr;
		attributes.ObjectName = &name;
		attributes.Attributes = OBJ_CASE_INSENSITIVE;
		attributes.SecurityDescriptor = nullptr;
		attributes.SecurityQualityOfService = nullptr;

		uint64_t args[] = {reinterpret_cast<uint64_t>(&result), desiredAccess, reinterpret_cast<uint64_t>(&attributes), reinterpret_cast<uint64_t>(&statusBlock),
			0, 0, ShareAccess, CreateDisposition, createOptions, 0, 0};
		executeWoW64Syscall(systemCalls_[NtCreateFile], args);

		freeUnicodeString(&name);
	}
	else
	{
		OBJECT_ATTRIBUTES attributes;
		IO_STATUS_BLOCK statusBlock;
		UNICODE_STRING name;
		getPrefixedPathUnicodeString(&name, Filename, FilenameLength);

		attributes.Length = sizeof(attributes);
		attributes.RootDirectory = nullptr;
		attributes.ObjectName = &name;
		attributes.Attributes = OBJ_CASE_INSENSITIVE;
		attributes.SecurityDescriptor = nullptr;
		attributes.SecurityQualityOfService = nullptr;

		uint32_t args[] = {reinterpret_cast<uint32_t>(&result), desiredAccess, reinterpret_cast<uint32_t>(&attributes), reinterpret_cast<uint32_t>(&statusBlock),
			0, 0, ShareAccess, CreateDisposition, createOptions, 0, 0};
		executeWin32Syscall(systemCalls_[NtCreateFile], args);

		freeUnicodeString(&name);
	}

	return reinterpret_cast<void *>(result);
}

void Win32NativeHelper::flushFile(void *fileHandle)
{
	if(isWoW64_)
	{
		IO_STATUS_BLOCK64 statusBlock;

		uint64_t args[] = {reinterpret_cast<uint64_t>(fileHandle), reinterpret_cast<uint64_t>(&statusBlock)};
		executeWoW64Syscall(systemCalls_[NtFlushBuffersFile], args);
	}
	else
	{
		IO_STATUS_BLOCK statusBlock;

		uint32_t args[] = {reinterpret_cast<uint32_t>(fileHandle), reinterpret_cast<uint32_t>(&statusBlock)};
		executeWin32Syscall(systemCalls_[NtFlushBuffersFile], args);
	}
}

size_t Win32NativeHelper::writeFile(void *fileHandle, const uint8_t *buffer, size_t bufferSize)
{
	if(isWoW64_)
	{
		IO_STATUS_BLOCK64 statusBlock;
		uint64_t args[] = {reinterpret_cast<uint64_t>(fileHandle), 0, 0, 0, reinterpret_cast<uint64_t>(&statusBlock), reinterpret_cast<uint64_t>(buffer), bufferSize, 0, 0};
		executeWoW64Syscall(systemCalls_[NtWriteFile], args);

		return reinterpret_cast<size_t>(statusBlock.Information);
	}
	else
	{
		IO_STATUS_BLOCK statusBlock;
		uint32_t args[] = {reinterpret_cast<uint32_t>(fileHandle), 0, 0, 0, reinterpret_cast<uint32_t>(&statusBlock), reinterpret_cast<uint32_t>(buffer), bufferSize, 0, 0};
		executeWin32Syscall(systemCalls_[NtWriteFile], args);

		return reinterpret_cast<size_t>(statusBlock.Information);
	}
}

void Win32NativeHelper::closeHandle(void *handle)
{
	if(isWoW64_)
	{
		uint64_t args[] = {reinterpret_cast<uint64_t>(handle)};
		executeWoW64Syscall(systemCalls_[NtClose], args);
	}
	else
	{
		uint32_t args[] = {reinterpret_cast<uint32_t>(handle)};
		executeWin32Syscall(systemCalls_[NtClose], args);
	}
}

void *Win32NativeHelper::createSection(void *file, uint32_t flProtect, uint64_t sectionSize, wchar_t *lpName, size_t NameLength)
{
	uint64_t __declspec(align(8)) result;
	uint32_t desiredAccess = STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ;
	PLARGE_INTEGER size = nullptr;
	LARGE_INTEGER localSize;
	uint32_t allocationAttributes = SEC_COMMIT;

	if(sectionSize)
	{
		size = &localSize;
		localSize.QuadPart = sectionSize;
	}

	if(isWoW64_)
	{
		OBJECT_ATTRIBUTES64 attributes = {0, };
		UNICODE_STRING64 name = {0, };

		name.Buffer = lpName;
		name.Length = NameLength * 2;
		name.MaximumLength = NameLength * 2;

		attributes.Length = sizeof(attributes);
		attributes.RootDirectory = nullptr;
		attributes.ObjectName = &name;
		attributes.Attributes = 0;
		attributes.SecurityDescriptor = nullptr;
		attributes.SecurityQualityOfService = nullptr;

		uint64_t args[] = {reinterpret_cast<uint64_t>(&result), SECTION_ALL_ACCESS, reinterpret_cast<uint64_t>(&attributes), 
			reinterpret_cast<uint64_t>(size), flProtect, allocationAttributes, reinterpret_cast<uint64_t>(file)};
		executeWoW64Syscall(systemCalls_[NtCreateSection], args);
	}
	else
	{
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

		uint32_t args[] = {reinterpret_cast<uint64_t>(&result), SECTION_ALL_ACCESS, reinterpret_cast<uint32_t>(&attributes),
			reinterpret_cast<uint32_t>(size), flProtect, allocationAttributes, reinterpret_cast<uint32_t>(file)};
		executeWin32Syscall(systemCalls_[NtCreateSection], args);
	}

	return reinterpret_cast<void *>(result);
}

void *Win32NativeHelper::mapViewOfSection(void *section, uint32_t dwDesiredAccess, uint64_t offset, size_t dwNumberOfBytesToMap, size_t lpBaseAddress)
{
	uint64_t result = lpBaseAddress;
	LARGE_INTEGER sectionOffset;
	uint64_t viewSize;
	size_t protect;

	sectionOffset.QuadPart = offset;
	viewSize = dwNumberOfBytesToMap;
	if(dwDesiredAccess == FILE_MAP_COPY)
		protect = PAGE_WRITECOPY;
	else if(dwDesiredAccess & FILE_MAP_WRITE)
		protect = dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
	else if(dwDesiredAccess & FILE_MAP_READ)
		protect = dwDesiredAccess & FILE_MAP_EXECUTE ? PAGE_EXECUTE_READ : PAGE_READONLY;

	if(isWoW64_)
	{
		uint64_t args[] = {reinterpret_cast<uint64_t>(section), NtCurrentProcess64(), reinterpret_cast<uint64_t>(&result), 0x7ffeffff, 0, 
			reinterpret_cast<uint64_t>(&sectionOffset), reinterpret_cast<uint64_t>(&viewSize), 1, 0, protect};
		executeWoW64Syscall(systemCalls_[NtMapViewOfSection], args);
	}
	else
	{
		uint32_t args[] = {reinterpret_cast<uint32_t>(section), NtCurrentProcess(), reinterpret_cast<uint32_t>(&result), 0, 0,
			reinterpret_cast<uint32_t>(&sectionOffset), reinterpret_cast<uint32_t>(&viewSize), 1, 0, protect};
		executeWin32Syscall(systemCalls_[NtMapViewOfSection], args);
	}

	return reinterpret_cast<void *>(result);
}

void Win32NativeHelper::unmapViewOfSection(void *lpBaseAddress)
{
	if(isWoW64_)
	{
		uint64_t args[] = {NtCurrentProcess64(), reinterpret_cast<uint64_t>(lpBaseAddress)};
		executeWoW64Syscall(systemCalls_[NtUnmapViewOfSection], args);
	}
	else
	{
		uint32_t args[] = {NtCurrentProcess(), reinterpret_cast<uint32_t>(lpBaseAddress)};
		executeWin32Syscall(systemCalls_[NtUnmapViewOfSection], args);
	}
}

uint32_t Win32NativeHelper::getFileAttributes(const wchar_t *filePath, size_t filePathLen)
{
	uint32_t desiredAccess = 0;
	FILE_NETWORK_OPEN_INFORMATION result;
	int32_t status;
	
	if(isWoW64_)
	{
		OBJECT_ATTRIBUTES64 attributes = {0, };
		UNICODE_STRING64 name = {0, };

		getPrefixedPathUnicodeString(&name, filePath, filePathLen);
		attributes.Length = sizeof(attributes);
		attributes.RootDirectory = nullptr;
		attributes.ObjectName = &name;
		attributes.Attributes = OBJ_CASE_INSENSITIVE;
		attributes.SecurityDescriptor = nullptr;
		attributes.SecurityQualityOfService = nullptr;

		uint64_t args[] = {reinterpret_cast<uint64_t>(&attributes), reinterpret_cast<uint64_t>(&result)};
		status = executeWoW64Syscall(systemCalls_[NtQueryFullAttributesFile], args);

		freeUnicodeString(&name);
	}
	else
	{
		OBJECT_ATTRIBUTES attributes;
		UNICODE_STRING name;

		getPrefixedPathUnicodeString(&name, filePath, filePathLen);
		attributes.Length = sizeof(attributes);
		attributes.RootDirectory = nullptr;
		attributes.ObjectName = &name;
		attributes.Attributes = OBJ_CASE_INSENSITIVE;
		attributes.SecurityDescriptor = nullptr;
		attributes.SecurityQualityOfService = nullptr;
		uint32_t args[] = {reinterpret_cast<uint32_t>(&attributes), reinterpret_cast<uint32_t>(&result)};

		status = executeWin32Syscall(systemCalls_[NtQueryFullAttributesFile], args);

		freeUnicodeString(&name);
	}

	if(status < 0)
		return INVALID_FILE_ATTRIBUTES;
	return result.FileAttributes;
}

void Win32NativeHelper::setFileSize(void *file, uint64_t size)
{
	LARGE_INTEGER largeSize;
	largeSize.QuadPart = size;

	if(isWoW64_)
	{
		IO_STATUS_BLOCK64 result;
		uint64_t args[] = {reinterpret_cast<uint64_t>(file), reinterpret_cast<uint64_t>(&result), reinterpret_cast<uint64_t>(&largeSize), sizeof(largeSize), 20};//FileEndOfFileInfo
		executeWoW64Syscall(systemCalls_[NtSetInformationFile], args);
		args[4] = 19;//FileAllocationInfo
		executeWoW64Syscall(systemCalls_[NtSetInformationFile], args);
	}
	else
	{
		IO_STATUS_BLOCK result;
		uint32_t args[] = {reinterpret_cast<uint32_t>(file), reinterpret_cast<uint32_t>(&result), reinterpret_cast<uint32_t>(&largeSize), sizeof(largeSize), 20};
		executeWin32Syscall(systemCalls_[NtSetInformationFile], args);
		args[4] = 19;
		executeWin32Syscall(systemCalls_[NtSetInformationFile], args);
	}
}

void Win32NativeHelper::flushInstructionCache(size_t offset, size_t size)
{
	if(isWoW64_)
	{
		uint64_t args[] = {NtCurrentProcess64(), offset, size};
		executeWoW64Syscall(systemCalls_[NtFlushInstructionCache], args);
	}
	else
	{
		uint32_t args[] = {NtCurrentProcess(), offset, size};
		executeWin32Syscall(systemCalls_[NtFlushInstructionCache], args);
	}
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

uint8_t *Win32NativeHelper::getApiSet()
{
	return myPEB_->ApiSet;
}

PEB *Win32NativeHelper::getPEB()
{
	return myPEB_;
}

List<Win32LoadedImage> Win32NativeHelper::getLoadedImages()
{
	List<Win32LoadedImage> result;
	LDR_MODULE *node = reinterpret_cast<LDR_MODULE *>(getPEB()->LoaderData->InLoadOrderModuleList.Flink->Flink);
	while(node->BaseAddress)
	{
		Win32LoadedImage image;
		image.baseAddress = reinterpret_cast<uint64_t>(node->BaseAddress);
		image.fileName = node->BaseDllName.Buffer;
		result.push_back(image);
		node = reinterpret_cast<LDR_MODULE *>(node->InLoadOrderModuleList.Flink);
	}
	return result;
}

List<String> Win32NativeHelper::getArgumentList()
{
	String str = WStringToString(getCommandLine());

	String item;
	List<String> items;
	bool quote = false;
	bool slash = false;
	for(size_t i = 0; i < str.length(); i ++)
	{
		if(slash == false && quote == false && str[i] == '\"')
		{
			quote = true;
			continue;
		}
		if(slash == false && quote == true && str[i] == '\"')
		{
			quote = false;
			continue;
		}
		if(slash == true && quote == true && str[i] == '\"')
		{
			item.push_back('\"');
			slash = false;
			continue;
		}
		if(slash == true && str[i] != '\"')
		{
			item.push_back('\\');
			slash = false;
		}
		if(slash == false && str[i] == '\\')
		{
			slash = true;
			continue;
		}
		if(quote == false && str[i] == ' ')
		{
			if(item.length() == 0)
				continue;
			items.push_back(std::move(item));
			item = "";
			continue;
		}

		item.push_back(str[i]);
	}
	if(item.length())
		items.push_back(std::move(item));

	return items;
}

String Win32NativeHelper::getSystem32Directory() const
{
	KUSER_SHARED_DATA *sharedData = reinterpret_cast<KUSER_SHARED_DATA *>(0x7ffe0000);

	String SystemRoot(WStringToString(sharedData->NtSystemRoot));
	return SystemRoot + "\\System32";
}

String Win32NativeHelper::getSysWOW64Directory() const
{
	KUSER_SHARED_DATA *sharedData = reinterpret_cast<KUSER_SHARED_DATA *>(0x7ffe0000);

	String SystemRoot(WStringToString(sharedData->NtSystemRoot));
	return SystemRoot + "\\SysWOW64";
}

bool Win32NativeHelper::isWoW64()
{
	return isWoW64_;
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
