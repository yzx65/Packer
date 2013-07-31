#include "Win32Runtime.h"

#include <cstdint>

#include "Win32Structure.h"
#include "PEFormat.h"
#include "String.h"
#include "Util.h"

Win32NativeHelper g_helper;

Win32NativeHelper *Win32NativeHelper::get()
{
	if(g_helper.init_ == false)
		g_helper.init();
	return &g_helper;
}

int compareString(const char *a, const char *b)
{
	while(*a && *b && *a == *b) *a++, *b++;
	return *a - *b;
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
				rtlAllocateHeap_ = static_cast<size_t>(address);
			else if(compareString(name, "RtlFreeHeap") == 0)
				rtlFreeHeap_ = static_cast<size_t>(address);
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
	heap_ = myPEB_->ProcessHeap;

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

	return reinterpret_cast<RtlAllocateHeapPtr>(ntdllBase_ + rtlAllocateHeap_)(heap_, 0, dwBytes);
}

bool Win32NativeHelper::freeHeap(void *ptr)
{
	typedef bool (__stdcall *RtlFreeHeapPtr)(void *hHeap, uint32_t dwFlags, void *ptr);

	return reinterpret_cast<RtlFreeHeapPtr>(ntdllBase_ + rtlFreeHeap_)(heap_, 0, ptr);
}

wchar_t *Win32NativeHelper::getCommandLine()
{
	return myPEB_->ProcessParameters->CommandLine.Buffer;
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

	void *memset(void *dst, int val, size_t size)
	{
		for(size_t i = 0; i < size; i ++)
			*(reinterpret_cast<uint8_t *>(dst) + i) = static_cast<uint8_t>(val);
		return dst;
	}
}
