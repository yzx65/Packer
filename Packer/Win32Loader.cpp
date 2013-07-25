#include "Win32Loader.h"

#include <Windows.h>

Win32Loader *loaderInstance_;

Win32Loader::Win32Loader(const Image &image, Vector<Image> &&imports) : image_(image), imports_(imports)
{
	loaderInstance_ = this;
}

uint8_t *Win32Loader::loadImage(const Image &image)
{
	uint8_t *baseAddress;
	baseAddress = reinterpret_cast<uint8_t *>(VirtualAlloc(nullptr, static_cast<uint32_t>(image.info.size), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

	for(auto &i : image.extendedData)
		for(size_t j = 0; j < i.data.size(); j ++)
			(baseAddress + i.baseAddress)[j] = i.data[j];

	for(auto &i : image.sections)
		for(size_t j = 0; j < i.data.size(); j ++)
			(baseAddress + i.baseAddress)[j] = i.data[j];

	int32_t diff = reinterpret_cast<int32_t>(baseAddress) - static_cast<int32_t>(image.info.baseAddress);
	for(auto &i : image.relocations)
		*reinterpret_cast<int32_t *>(baseAddress + i) += diff;

	for(auto &i : image.imports)
	{
		void *library = loadLibrary(i.libraryName.c_str());
		for(auto &j : i.functions)
		{
			uint32_t functionAddress = getFunctionAddress(library, j.name.c_str());
			*reinterpret_cast<uint32_t *>(j.iat + baseAddress) = functionAddress;
		}
	}

	for(auto &i : image.sections)
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
	uint8_t *baseAddress = loadImage(image_);

	typedef void (*EntryPointType)();
	EntryPointType entryPoint = reinterpret_cast<EntryPointType>(baseAddress + image_.info.entryPoint);
	entryPoint();
}

void *Win32Loader::loadLibrary(const char *filename)
{
	for(auto &i : imports_)
		if(i.fileName == filename)
			return loadImage(i);

	return LoadLibraryA(filename);
}

uint32_t Win32Loader::getFunctionAddress(void *library, const char *functionName)
{
	return reinterpret_cast<uint32_t>(GetProcAddress(reinterpret_cast<HMODULE>(library), functionName));
}