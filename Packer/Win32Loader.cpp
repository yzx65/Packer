#include "Win32Loader.h"

#include <Windows.h>

Win32Loader *loaderInstance_;

Win32Loader::Win32Loader(const Executable &executable, const DataStorage<Executable> imports) : executable_(executable), imports_(imports)
{
	loaderInstance_ = this;
}

uint8_t *Win32Loader::loadExecutable(const Executable &executable)
{
	uint8_t *baseAddress;
	baseAddress = reinterpret_cast<uint8_t *>(VirtualAlloc(nullptr, static_cast<uint32_t>(executable.info.size), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

	for(auto &i : executable.extendedData)
		memcpy(baseAddress + i.baseAddress, &i.data[0], i.data.size());

	for(auto &i : executable.sections)
		memcpy(baseAddress + i.baseAddress, &i.data[0], i.data.size());

	int32_t diff = reinterpret_cast<int32_t>(baseAddress) - static_cast<int32_t>(executable.info.baseAddress);
	for(auto &i : executable.relocations)
		*reinterpret_cast<int32_t *>(baseAddress + i) += diff;

	for(auto &i : executable.imports)
	{
		void *library = loadLibrary(i.libraryName.get());
		for(auto &j : i.functions)
		{
			uint32_t functionAddress = getFunctionAddress(library, j.name.get());
			*reinterpret_cast<uint32_t *>(j.iat + baseAddress) = functionAddress;
		}
	}

	for(auto &i : executable.sections)
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

	return baseAddress;
}

void Win32Loader::execute()
{
	uint8_t *baseAddress = loadExecutable(executable_);

	typedef void (*EntryPointType)();
	EntryPointType entryPoint = reinterpret_cast<EntryPointType>(baseAddress + executable_.info.entryPoint);
	entryPoint();
}

void *Win32Loader::loadLibrary(const char *filename)
{
	for(auto &i : imports_)
		if(strcmp(filename, i.fileName.get()) == 0)
			return loadExecutable(i);

	return LoadLibraryA(filename);
}

uint32_t Win32Loader::getFunctionAddress(void *library, const char *functionName)
{
	return reinterpret_cast<uint32_t>(GetProcAddress(reinterpret_cast<HMODULE>(library), functionName));
}