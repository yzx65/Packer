#include "Win32Loader.h"

#include "FormatBase.h"
#include "Win32Runtime.h"
#include "Win32Structure.h"
#include "Util.h"
#include <Windows.h>

Win32Loader *loaderInstance_;

Win32Loader::Win32Loader(const Image &image, Vector<Image> &&imports) : image_(image), imports_(imports.begin(), imports.end())
{
	loaderInstance_ = this;
}

uint8_t *Win32Loader::loadImage(const Image &image)
{
	if(image.fileName == "ntdll.dll") //ntdll.dll is always loaded
	{
		uint8_t *baseAddress = reinterpret_cast<uint8_t *>(Win32NativeHelper::get()->getNtdll());
		loadedLibraries_.insert(String(image.fileName), reinterpret_cast<uint64_t>(baseAddress));
		loadedImages_.insert(reinterpret_cast<uint64_t>(baseAddress), &image);
		return baseAddress;
	}
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
		void *library = loadLibrary(i.libraryName);
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

void *Win32Loader::loadLibrary(const String &filename)
{
	API_SET_HEADER *apiSet = Win32NativeHelper::get()->getApiSet();
	if(filename[0] == 'a' && filename[1] == 'p' && filename[2] == 'i' && filename[3] == '-')
	{
		String temp = filename;
		temp.resize(temp.length() - 4); //minus .dll
		auto item = binarySearch(apiSet->Entries, apiSet->Entries + apiSet->NumberOfEntries, 
			[&](const API_SET_ENTRY *entry) -> int
			{
				wchar_t *name = reinterpret_cast<wchar_t *>(reinterpret_cast<uint8_t *>(apiSet) + entry->Name);
				size_t i = 0;
				for(; i < entry->NameLength / sizeof(wchar_t); i ++)
					if(static_cast<char>(name[i]) != temp[i + 4])
						return temp[i + 4] - static_cast<char>(name[i]);
				return temp[i + 4] - static_cast<char>(name[i]);
			});
		if(item != apiSet->Entries + apiSet->NumberOfEntries)
		{
			API_SET_HOST_DESCRIPTOR *descriptor = reinterpret_cast<API_SET_HOST_DESCRIPTOR *>(reinterpret_cast<uint8_t *>(apiSet) + item->HostDescriptor);
			for(size_t i = 0; i < descriptor->NumberOfHosts; i ++)
			{
				wchar_t *hostName = reinterpret_cast<wchar_t *>(reinterpret_cast<uint8_t *>(apiSet) + descriptor->Hosts[i].HostModuleName);
				WString moduleName(hostName, hostName + descriptor->Hosts[i].HostModuleNameLength / sizeof(wchar_t));
				void *library = loadLibrary(WStringToString(moduleName));
				if(library)
					return library;
			}
			return nullptr;
		}
	}
	auto it = loadedLibraries_.find(String(filename));
	if(it != loadedLibraries_.end())
		return reinterpret_cast<void *>(*it);
	for(auto &i : imports_)
		if(i.fileName == filename)
			return loadImage(i);

	SharedPtr<FormatBase> format = FormatBase::loadImport(String(filename));
	if(!format.get())
		return nullptr;
	return loadImage(*imports_.push_back(format->serialize()));
}

uint64_t Win32Loader::getFunctionAddress(void *library, const char *functionName)
{
	auto it = loadedImages_.find(reinterpret_cast<uint64_t>(library));
	if(it != loadedImages_.end())
	{
		auto item = binarySearch((*it)->exports.begin(), (*it)->exports.begin() + (*it)->nameExportLen, [&](const ExportFunction *a) -> int { return a->name.compare(functionName); });
		if(item == (*it)->exports.end() + (*it)->nameExportLen)
			return 0;
		return item->address + reinterpret_cast<uint64_t>(library);
	}
	return 0;
}