#include "Win32Loader.h"

#include "FormatBase.h"
#include "Win32Runtime.h"
#include "Win32Structure.h"
#include "Util.h"
#include "File.h"
#include "PEFormat.h"
#include "PEHeader.h"

#define DLL_PROCESS_ATTACH   1    
#define DLL_THREAD_ATTACH    2    
#define DLL_THREAD_DETACH    3    
#define DLL_PROCESS_DETACH   0  

Win32Loader *loaderInstance_;

Win32Loader::Win32Loader(const Image &image, Vector<Image> &&imports) : image_(image), imports_(imports.begin(), imports.end())
{
	loaderInstance_ = this;
}

uint8_t *Win32Loader::mapImage(const Image &image)
{
	uint8_t *baseAddress = reinterpret_cast<uint8_t *>(Win32NativeHelper::get()->allocateVirtual(static_cast<size_t>(image.info.size), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	copyMemory(baseAddress, image.header->get(), image.header->getSize());

	for(auto &i : image.sections)
		copyMemory(baseAddress + i.baseAddress, i.data->get(), i.data->getSize());

	int64_t diff = -static_cast<int64_t>(image.info.baseAddress) + reinterpret_cast<int64_t>(baseAddress);
	for(auto &j : image.relocations)
		if(image.info.architecture == ArchitectureWin32)
			*reinterpret_cast<int32_t *>(baseAddress + j) += static_cast<int32_t>(diff);
		else
			*reinterpret_cast<int64_t *>(baseAddress + j) += static_cast<int64_t>(diff);

	loadedLibraries_.insert(String(image.fileName), reinterpret_cast<uint64_t>(baseAddress));
	loadedImages_.insert(reinterpret_cast<uint64_t>(baseAddress), &image);
	return baseAddress;
}

void Win32Loader::processImports(uint8_t *baseAddress, const Image &image)
{
	for(auto &i : image.imports)
	{
		void *library = loadLibrary(i.libraryName);
		for(auto &j : i.functions)
		{
			uint64_t function = getFunctionAddress(library, j.name, j.ordinal);
			if(image.info.architecture == ArchitectureWin32)
				*reinterpret_cast<uint32_t *>(j.iat + baseAddress) = static_cast<uint32_t>(function);
			else
				*reinterpret_cast<uint64_t *>(j.iat + baseAddress) = static_cast<uint64_t>(function);
		}
	}
}

void Win32Loader::adjustPageProtection(uint8_t *baseAddress, const Image &image)
{
	for(auto &i : image.sections)
	{
		uint32_t unused, protect = 0;
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

		Win32NativeHelper::get()->protectVirtual(baseAddress + i.baseAddress, static_cast<int32_t>(i.size), protect, &unused);
	}
}

void Win32Loader::executeEntryPoint(uint8_t *baseAddress, const Image &image)
{
	//security cookie
	if(image.info.platformData)
	{	
		if(image.info.architecture == ArchitectureWin32)
			*reinterpret_cast<uint32_t *>(baseAddress - image.info.baseAddress + image.info.platformData) += 10;
		else
			*reinterpret_cast<uint64_t *>(baseAddress - image.info.baseAddress + image.info.platformData) += 10;
	}
	if(image.info.entryPoint)
	{
		if((image.info.flag & ImageFlagLibrary))
		{
			typedef int (__stdcall *DllEntryPointType)(void *, int, void *);
			DllEntryPointType entryPoint = reinterpret_cast<DllEntryPointType>(baseAddress + image.info.entryPoint);
			entryPoint(reinterpret_cast<void *>(baseAddress), DLL_PROCESS_ATTACH, reinterpret_cast<void *>(1));  //lpReserved is non-null for static loads
		}
		else
		{
			typedef int (__stdcall *EntryPointType)();
			EntryPointType entryPoint = reinterpret_cast<EntryPointType>(baseAddress + image.info.entryPoint);
			entryPoint();
		}
	}
}

void Win32Loader::executeEntryPointQueue()
{
	for(auto i = entryPointQueue_.begin(); i != entryPointQueue_.end(); )
	{
		auto oldi = i;
		uint64_t baseAddress = *i;
		i ++;
		entryPointQueue_.remove(oldi);
		executeEntryPoint(reinterpret_cast<uint8_t *>(baseAddress), *loadedImages_[baseAddress]);
	}
}

uint8_t *Win32Loader::loadImage(const Image &image)
{
	uint8_t *baseAddress = mapImage(image);
	processImports(baseAddress, image);
	adjustPageProtection(baseAddress, image);
	entryPointQueue_.push_back(reinterpret_cast<uint64_t>(baseAddress));
	return baseAddress;
}

void Win32Loader::execute()
{
	uint8_t *baseAddress = mapImage(image_);
	Win32NativeHelper::get()->getPEB()->ImageBaseAddress = reinterpret_cast<void *>(baseAddress);
	processImports(baseAddress, image_);
	adjustPageProtection(baseAddress, image_);

	executeEntryPointQueue();
	executeEntryPoint(baseAddress, image_);
}

void *Win32Loader::loadLibrary(const String &filename)
{
	String normalizedFilename = filename;
	int pos;
	if((pos = filename.rfind('\\')) != -1)
		normalizedFilename = filename.substr(pos + 1);
	if(filename.find('.') == -1)
		normalizedFilename.append(".dll");

	auto it = loadedLibraries_.find(normalizedFilename);
	if(it != loadedLibraries_.end())
		return reinterpret_cast<void *>(it->value);

	//check if already loaded
	auto images = Win32NativeHelper::get()->getLoadedImages();
	WString wstrName(StringToWString(filename));
	for(auto it = images.begin(); it != images.end(); it ++)
	{
		if(wstrName.icompare(it->fileName) == 0)
		{
			uint8_t *baseAddress = it->baseAddress;
			PEFormat format;
			format.load(MakeShared<MemoryDataSource>(baseAddress), true);
			format.setFileName(filename);
			auto it = imports_.push_back(format.serialize());
			loadedLibraries_.insert(filename, reinterpret_cast<uint64_t>(baseAddress));
			loadedImages_.insert(reinterpret_cast<uint64_t>(baseAddress), &*it);

			if(it->fileName.icompare("kernelbase.dll") == 0)
			{
				//We need to patch ResolveDelayLoadedAPI
				for(auto i : it->imports)
				{
					if(i.libraryName.icompare("ntdll.dll") == 0)
					{
						for(auto j : i.functions)
						{
							if(j.name.icompare("LdrResolveDelayLoadedAPI") == 0)
							{
								size_t dest = reinterpret_cast<size_t>(baseAddress) + static_cast<size_t>(j.iat);
								size_t old;

								Win32NativeHelper::get()->protectVirtual(reinterpret_cast<void *>(dest), sizeof(size_t), PAGE_READWRITE, &old);
								*reinterpret_cast<size_t *>(dest) = reinterpret_cast<size_t>(LdrResolveDelayLoadedAPIProxy);
								Win32NativeHelper::get()->protectVirtual(reinterpret_cast<void *>(dest), sizeof(size_t), old, &old);
								break;
							}
						}
					}
				}
			}

			return baseAddress;
		}
	}

	String temp = normalizedFilename.substr(0, 4);
	if(temp.icompare("api-") == 0 || temp.icompare("ext-") == 0)
	{
		API_SET_HEADER *apiSet = Win32NativeHelper::get()->getApiSet();
		String temp = filename;
		temp.resize(temp.length() - 4); //minus .dll
		auto item = binarySearch(apiSet->Entries, apiSet->Entries + apiSet->NumberOfEntries, 
			[&](const API_SET_ENTRY *entry) -> int
			{
				wchar_t *name = reinterpret_cast<wchar_t *>(reinterpret_cast<uint8_t *>(apiSet) + entry->Name);
				size_t i = 0;
				for(; i < entry->NameLength / sizeof(wchar_t) - 1; i ++)
					if(static_cast<char>(WString::to_lower(name[i])) != String::to_lower(temp[i + 4]))
						return static_cast<char>(WString::to_lower(name[i])) - String::to_lower(temp[i + 4]);
				return static_cast<char>(WString::to_lower(name[i])) - String::to_lower(temp[i + 4]);
			});
		if(item != apiSet->Entries + apiSet->NumberOfEntries)
		{
			API_SET_HOST_DESCRIPTOR *descriptor = reinterpret_cast<API_SET_HOST_DESCRIPTOR *>(reinterpret_cast<uint8_t *>(apiSet) + item->HostDescriptor);
			for(size_t i = descriptor->NumberOfHosts - 1; i >= 0 ; i --)
			{
				wchar_t *hostName = reinterpret_cast<wchar_t *>(reinterpret_cast<uint8_t *>(apiSet) + descriptor->Hosts[i].HostModuleName);
				WString moduleName(hostName, hostName + descriptor->Hosts[i].HostModuleNameLength / sizeof(wchar_t));
				void *library;
				if((library = GetModuleHandleWProxy(moduleName.c_str())) == 0)
				{
					library = loadLibrary(WStringToString(moduleName));
					loadedLibraries_.insert(filename, reinterpret_cast<uint64_t>(library));
				}
				if(library)
					return library;
			}
			return nullptr;
		}
	}

	Image *image = nullptr;
	for(auto &i : imports_)
		if(i.fileName.icompare(filename) == 0)
			return loadImage(i);

	SharedPtr<FormatBase> format = FormatBase::loadImport(filename, image_.filePath);
	if(!format.get())
		return nullptr;
	return loadImage(*imports_.push_back(format->serialize()));
}

uint64_t Win32Loader::getFunctionAddress(void *library, const String &functionName, int ordinal)
{
	auto it = loadedImages_.find(reinterpret_cast<uint64_t>(library));
	if(it != loadedImages_.end())
	{
		const Image *image = it->value;
		if(image->fileName.icompare("kernel32.dll") == 0 || image->fileName.icompare("kernelbase.dll") == 0)
		{
			if(functionName.icompare("LoadLibraryExW") == 0)
				return reinterpret_cast<uint64_t>(LoadLibraryExWProxy);
			else if(functionName.icompare("LoadLibraryExA") == 0)
				return reinterpret_cast<uint64_t>(LoadLibraryExAProxy);
			else if(functionName.icompare("LoadLibraryW") == 0)
				return reinterpret_cast<uint64_t>(LoadLibraryWProxy);
			else if(functionName.icompare("LoadLibraryA") == 0)
				return reinterpret_cast<uint64_t>(LoadLibraryAProxy);
			else if(functionName.icompare("GetModuleHandleExW") == 0)
				return reinterpret_cast<uint64_t>(GetModuleHandleExWProxy);
			else if(functionName.icompare("GetModuleHandleExA") == 0)
				return reinterpret_cast<uint64_t>(GetModuleHandleExAProxy);
			else if(functionName.icompare("GetModuleHandleW") == 0)
				return reinterpret_cast<uint64_t>(GetModuleHandleWProxy);
			else if(functionName.icompare("GetModuleHandleA") == 0)
				return reinterpret_cast<uint64_t>(GetModuleHandleAProxy);
			else if(functionName.icompare("GetProcAddress") == 0)
				return reinterpret_cast<uint64_t>(GetProcAddressProxy);
			else if(functionName.icompare("ResolveDelayLoadedAPI") == 0)
				return reinterpret_cast<uint64_t>(LdrResolveDelayLoadedAPIProxy);
		}
		else if(image->fileName.icompare("ntdll.dll") == 0)
		{
			if(functionName.icompare("LdrAddRefDll") == 0)
				return reinterpret_cast<uint64_t>(LdrAddRefDllProxy);
			else if(functionName.icompare("LdrLoadDll") == 0)
				return reinterpret_cast<uint64_t>(LdrLoadDllProxy);
			else if(functionName.icompare("LdrResolveDelayLoadedAPI") == 0)
				return reinterpret_cast<uint64_t>(LdrResolveDelayLoadedAPIProxy);
		}

		auto item = static_cast<ExportFunction *>(nullptr);
		if(functionName.length())
		{
			item = binarySearch(image->exports.begin(), image->exports.begin() + image->nameExportLen, [&](const ExportFunction *a) -> int { return a->name.compare(functionName); });
			if(item == image->exports.begin() + image->nameExportLen)
				item = nullptr;
		}
		if(item == nullptr)
			if(ordinal != -1)
				for(auto &i : image->exports)
					if(i.ordinal == ordinal)
						item = &i;
		if(item == nullptr)
			return 0;
		if(item->forward.length())
		{
			int point = item->forward.find('.');
			String dllName = item->forward.substr(0, point);
			String functionName = item->forward.substr(point + 1);
			int ordinal = -1;
			if(functionName[0] == '#')
				ordinal = StringToInt(functionName.substr(1));

			return getFunctionAddress(loadLibrary(dllName + ".dll"), functionName, ordinal);
		}
		return item->address + reinterpret_cast<uint64_t>(library);
	}
	return 0;
}

void * __stdcall Win32Loader::LoadLibraryAProxy(const char *libraryName)
{
	return LoadLibraryExAProxy(libraryName, nullptr, 0);
}

void * __stdcall Win32Loader::LoadLibraryWProxy(const wchar_t *libraryName)
{
	return LoadLibraryExWProxy(libraryName, nullptr, 0);
}

void * __stdcall Win32Loader::LoadLibraryExAProxy(const char *libraryName, void *a1, uint32_t a2)
{
	return LoadLibraryExWProxy(StringToWString(String(libraryName)).c_str(), a1, a2);
}

void * __stdcall Win32Loader::LoadLibraryExWProxy(const wchar_t *libraryName, void *, uint32_t)
{
	UNICODE_STRING str;
	void *result;
	str.Buffer = const_cast<wchar_t *>(libraryName);
	str.Length = -1;
	str.MaximumLength = -1;

	LdrLoadDllProxy(nullptr, nullptr, &str, &result);
	return result;
}

void * __stdcall Win32Loader::GetModuleHandleAProxy(const char *fileName)
{
	void *result;
	GetModuleHandleExAProxy(0, fileName, &result);
	return result;
}

void * __stdcall Win32Loader::GetModuleHandleWProxy(const wchar_t *fileName)
{
	void *result;
	GetModuleHandleExWProxy(0, fileName, &result);
	return result;
}

uint32_t __stdcall Win32Loader::GetModuleHandleExAProxy(uint32_t a1, const char *filename_, void **result)
{
	if(!filename_)
		return GetModuleHandleExWProxy(a1, nullptr, result);
	return GetModuleHandleExWProxy(a1, StringToWString(String(filename_)).c_str(), result);
}

uint32_t __stdcall Win32Loader::GetModuleHandleExWProxy(uint32_t, const wchar_t *filename_, void **result)
{
	*result = 0;
	if(filename_ == nullptr)
	{
		*result = reinterpret_cast<void *>(Win32NativeHelper::get()->getPEB()->ImageBaseAddress);
		return 1;
	}
	String filename(WStringToString(WString(filename_)));
	for(auto &i : loaderInstance_->loadedImages_)
	{
		if(i.value->fileName.icompare(filename) == 0 || 
			File::combinePath(i.value->filePath, i.value->fileName).icompare(filename) == 0 || 
			i.value->fileName.substr(0, i.value->fileName.length() - 4).icompare(filename) == 0)
		{
			*result = reinterpret_cast<void *>(i.key);
			return 1;
		}
	}
	return 0;
}

void * __stdcall Win32Loader::GetProcAddressProxy(void *library, char *functionName)
{
	if(reinterpret_cast<size_t>(functionName) < 0x10000)
		return reinterpret_cast<void *>(loaderInstance_->getFunctionAddress(library, String(), reinterpret_cast<size_t>(functionName)));
	return reinterpret_cast<void *>(loaderInstance_->getFunctionAddress(library, String(functionName)));
}

uint32_t __stdcall Win32Loader::LdrAddRefDllProxy(uint32_t flags, void *library)
{
	return 0;
}

uint32_t __stdcall Win32Loader::LdrLoadDllProxy(wchar_t *searchPath, size_t *dllCharacteristics, UNICODE_STRING *dllName, void **baseAddress)
{
	List<uint64_t> entryPointQueueTemp(std::move(loaderInstance_->entryPointQueue_));
	void *result = loaderInstance_->loadLibrary(WStringToString(WString(dllName->Buffer)));
	loaderInstance_->executeEntryPointQueue();
	loaderInstance_->entryPointQueue_ = std::move(entryPointQueueTemp);
	if(baseAddress)
		*baseAddress = result;

	return 0;
}

size_t __stdcall Win32Loader::LdrResolveDelayLoadedAPIProxy(uint8_t *base, PCIMAGE_DELAYLOAD_DESCRIPTOR desc, void *dllhook, void *syshook, size_t *addr, size_t flags)
{
	char *dllName = reinterpret_cast<char *>(base + desc->DllNameRVA);
	size_t *iatTable = reinterpret_cast<size_t *>(base + desc->ImportAddressTableRVA);
	size_t *nameTable = reinterpret_cast<size_t *>(base + desc->ImportNameTableRVA);
	size_t *moduleHandle = reinterpret_cast<size_t *>(base + desc->ModuleHandleRVA);
	
	if(!*moduleHandle)
	{
		*moduleHandle = reinterpret_cast<size_t>(loaderInstance_->GetModuleHandleAProxy(dllName));
		if(!*moduleHandle)
			*moduleHandle = reinterpret_cast<size_t>(loaderInstance_->LoadLibraryAProxy(dllName));
	}

	int i = 0;
	while(true)
	{
		uint8_t *dest = reinterpret_cast<uint8_t *>(iatTable[i]);
		size_t nameOffset = nameTable[i];
		IMAGE_IMPORT_BY_NAME *nameEntry = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(base + nameOffset);
		i ++;

		if(!dest)
			break;
		if(reinterpret_cast<size_t>(dest) != *addr)
			continue;
		size_t functionAddress;
		if(nameOffset & IMAGE_ORDINAL_FLAG32)
			functionAddress = static_cast<size_t>(loaderInstance_->getFunctionAddress(reinterpret_cast<void *>(*moduleHandle), String(), nameOffset & 0xffff));
		else
			functionAddress = static_cast<size_t>(loaderInstance_->getFunctionAddress(reinterpret_cast<void *>(*moduleHandle), String(nameEntry->Name)));

		size_t old;
		Win32NativeHelper::get()->protectVirtual(dest, 5, PAGE_READWRITE, &old);
		*dest = 0xe9; //jmp
		size_t rel = functionAddress - (reinterpret_cast<size_t>(dest) + 5);
		*reinterpret_cast<size_t *>(dest + 1) = rel;
		Win32NativeHelper::get()->protectVirtual(dest, 5, old, &old);

		return static_cast<size_t>(functionAddress);
	}

	return 0;
}