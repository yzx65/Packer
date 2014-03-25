#include "Win32NativeHelper.h"

#include <cstdint>

#include "../Util/String.h"
#include "../Runtime/Allocator.h"
#include "Win32Structure.h"

#include <intrin.h>

Win32NativeHelper *Win32NativeHelper::get()
{
	static Win32NativeHelper helper;
	return &helper;
}

uint32_t Win32NativeHelper::getPEB64()
{
	_asm
	{
		push 0x33
		push offset x64
		call fword ptr[esp] //jump to x64 mode
		add esp, 8
		jmp end
x64:
		__asm __emit 0x65 __asm __emit 0x48 __asm __emit 0x8b __asm __emit 0x04 __asm __emit 0x25 __asm __emit 0x60 __asm __emit 0x00 __asm __emit 0x00 __asm __emit 0x00
		//mov rax, gs:[0x60]
		retf //return to x32
end:
	}
}

void Win32NativeHelper::init()
{
	myPEB_ = reinterpret_cast<PEB *>(__readfsdword(0x30));
	myBase_ = reinterpret_cast<size_t>(myPEB_->ImageBaseAddress);

	if(__readfsdword(0xC0) != 0)
	{
		isWoW64_ = true;
		myPEB64_ = reinterpret_cast<PEB64 *>(getPEB64());
	}
	else
	{
		isWoW64_ = false;
		myPEB64_ = nullptr;
	}
}

size_t Win32NativeHelper::getMyBase() const
{
	return myBase_;
}

void Win32NativeHelper::setMyBase(size_t address)
{
	myBase_ = address;
	myPEB_->ImageBaseAddress = reinterpret_cast<void *>(address);
	if(isWoW64_) //we also have to modify 64bit peb if we are on wow64.
		myPEB64_->ImageBaseAddress = address;
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

uint32_t Win32NativeHelper::getRandomValue()
{
	KUSER_SHARED_DATA *sharedData = reinterpret_cast<KUSER_SHARED_DATA *>(0x7ffe0000);
	uint64_t tick = sharedData->TickCountQuad;
	uint32_t *temp = reinterpret_cast<uint32_t *>(&tick);
	return ((temp[0] | temp[1]) ^ 0xbeafdead) * temp[0];
}

void Win32NativeHelper::showError(const String &message)
{

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
