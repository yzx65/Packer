#include "Win32Loader.h"

#include "FormatBase.h"
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

	int64_t diff = reinterpret_cast<int64_t>(baseAddress) - static_cast<int64_t>(image.info.baseAddress);
	for(auto &i : image.relocations)
		if(image.info.architecture == ArchitectureWin32)
			*reinterpret_cast<int32_t *>(baseAddress + i) += static_cast<int32_t>(diff);
		else
			*reinterpret_cast<int64_t *>(baseAddress + i) += diff;

	loadedLibraries_.insert(String(image.fileName), reinterpret_cast<uint64_t>(baseAddress));
	loadedImages_.insert(reinterpret_cast<uint64_t>(baseAddress), &image);

	for(auto &i : image.imports)
	{
		void *library = loadLibrary(i.libraryName.c_str());
		for(auto &j : i.functions)
		{
			if(image.info.architecture == ArchitectureWin32)
			{
				uint32_t functionAddress = static_cast<uint32_t>(getFunctionAddress(library, j.name.c_str()));
				*reinterpret_cast<uint32_t *>(j.iat + baseAddress) = functionAddress;
			}
			else
			{
				uint64_t functionAddress = static_cast<uint64_t>(getFunctionAddress(library, j.name.c_str()));
				*reinterpret_cast<uint64_t *>(j.iat + baseAddress) = functionAddress;
			}
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

	if(image.info.entryPoint)
	{
		if(image.info.flag & ImageFlagLibrary)
		{
			typedef int (__stdcall *DllEntryPointType)(HINSTANCE, int, LPVOID);
			DllEntryPointType entryPoint = reinterpret_cast<DllEntryPointType>(baseAddress + image.info.entryPoint);
			entryPoint(reinterpret_cast<HINSTANCE>(baseAddress), DLL_PROCESS_ATTACH, reinterpret_cast<LPVOID>(1));  //lpReserved is non-null for static loads
		}
		else
		{
			typedef int (__stdcall *EntryPointType)();
			EntryPointType entryPoint = reinterpret_cast<EntryPointType>(baseAddress + image.info.entryPoint);
			entryPoint();
		}
	}

	return baseAddress;
}

void Win32Loader::execute()
{
	uint8_t *baseAddress = loadImage(image_);
}

void *Win32Loader::loadLibrary(const char *filename)
{
	auto it = loadedLibraries_.find(String(filename));
	if(it != loadedLibraries_.end())
		return reinterpret_cast<void *>(*it);
	for(auto &i : imports_)
		if(i.fileName == filename)
			return loadImage(i);

	SharedPtr<FormatBase> format = FormatBase::loadImport(String(filename));
	Image image = format->serialize();
	return loadImage(*imports_.push_back(image));
}

uint64_t Win32Loader::getFunctionAddress(void *library, const char *functionName)
{
	auto it = loadedImages_.find(reinterpret_cast<uint64_t>(library));
	if(it != loadedImages_.end())
		for(auto &i : (*it)->exports)
			if(i.name == functionName)
				return i.address + reinterpret_cast<uint64_t>(library);
	return 0;
}