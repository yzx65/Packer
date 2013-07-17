#include "Win32Loader.h"

#include <Windows.h>

Win32Loader *loaderInstance_;

Win32Loader::Win32Loader(const Executable &executable) : executable_(executable)
{
	loaderInstance_ = this;
}

void Win32Loader::load()
{
	uint8_t *baseAddress;
	baseAddress = reinterpret_cast<uint8_t *>(VirtualAlloc(nullptr, static_cast<uint32_t>(executable_.info.size), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

	for(auto &i : executable_.extendedData)
		memcpy(baseAddress + i.baseAddress, &i.data[0], i.data.size());

	for(auto &i : executable_.sections)
		memcpy(baseAddress + i.baseAddress, &i.data[0], i.data.size());

	int32_t diff = reinterpret_cast<int32_t>(baseAddress) - static_cast<int32_t>(executable_.info.baseAddress);
	for(auto &i : executable_.relocations)
		*reinterpret_cast<int32_t *>(baseAddress + i) += diff;

	for(auto &i : executable_.imports)
	{
		void *library = loadLibrary(i.libraryName.get());
		for(auto &j : i.functions)
		{
			uint32_t functionAddress = getFunctionAddress(library, j.name.get());
			*reinterpret_cast<uint32_t *>(j.iat + baseAddress) = functionAddress;
		}
	}

	for(auto &i : executable_.sections)
	{
		DWORD unused, protect = 0;
		if(i.flag & SectionFlagRead)
			protect = PAGE_READONLY;
		if(i.flag & SectionFlagWrite)
			protect = PAGE_READWRITE;
		if(i.flag & SectionFlagExecute)
		{
			if(i.flag & SectionFlagWrite)
				protect = PAGE_EXECUTE_READWRITE;
			else
				protect = PAGE_EXECUTE_READ;
		}

		VirtualProtect(baseAddress + i.baseAddress, static_cast<int32_t>(i.size), protect, &unused);
	}

	typedef void (*EntryPointType)();
	EntryPointType entryPoint = reinterpret_cast<EntryPointType>(baseAddress + executable_.info.entryPoint);
	entryPoint();
}

void *Win32Loader::loadLibrary(const char *filename)
{
	return LoadLibraryA(filename);
}

uint32_t Win32Loader::getFunctionAddress(void *library, const char *functionName)
{
	return reinterpret_cast<uint32_t>(GetProcAddress(reinterpret_cast<HMODULE>(library), functionName));
}