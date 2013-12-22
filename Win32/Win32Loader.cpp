#include "Win32Loader.h"

#include "../Runtime/FormatBase.h"
#include "Win32Runtime.h"
#include "Win32Structure.h"
#include "../Util/Util.h"
#include "../Runtime/File.h"
#include "../Runtime/PEFormat.h"
#include "../Runtime/PEHeader.h"

#define DLL_PROCESS_ATTACH   1    
#define DLL_THREAD_ATTACH    2    
#define DLL_THREAD_DETACH    3    
#define DLL_PROCESS_DETACH   0  

Win32Loader *loaderInstance_; //TODO: Remove global instance;

Win32Loader::Win32Loader(Image &&image, List<Image> &&imports) : image_(image), imports_(imports)
{
	loaderInstance_ = this;
}

uint64_t Win32Loader::mapImage(Image &image)
{
	uint64_t desiredAddress = 0;
	if(!image.relocations.size())
		desiredAddress = image.info.baseAddress;
	uint64_t baseAddress = reinterpret_cast<uint64_t>(Win32NativeHelper::get()->allocateVirtual(static_cast<size_t>(desiredAddress), static_cast<size_t>(image.info.size), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	copyMemory(reinterpret_cast<uint8_t *>(baseAddress), image.header->get(), image.header->size());

	for(auto &i : image.sections)
		copyMemory(reinterpret_cast<uint8_t *>(baseAddress + i.baseAddress), i.data->get(), i.data->size());

	int64_t diff = baseAddress;
	diff -= image.info.baseAddress;
	for(auto &j : image.relocations)
		if(image.info.architecture == ArchitectureWin32)
			*reinterpret_cast<int32_t *>(baseAddress + j) += static_cast<int32_t>(diff);
		else
			*reinterpret_cast<int64_t *>(baseAddress + j) += static_cast<int64_t>(diff);

	loadedLibraries_.insert(String(image.fileName), baseAddress);
	loadedImages_.insert(baseAddress, &image);
	return baseAddress;
}

void Win32Loader::processImports(uint64_t baseAddress, const Image &image)
{
	for(auto &i : image.imports)
	{
		uint64_t library = loadLibrary(i.libraryName);
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

void Win32Loader::adjustPageProtection(uint64_t baseAddress, const Image &image)
{
	for(auto &i : image.sections)
	{
		uint32_t protect = 0;
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

		Win32NativeHelper::get()->protectVirtual(reinterpret_cast<void *>(baseAddress + i.baseAddress), static_cast<int32_t>(i.size), protect);
		if(i.flag & SectionFlagExecute)
			Win32NativeHelper::get()->flushInstructionCache(static_cast<size_t>(baseAddress + i.baseAddress), static_cast<int32_t>(i.size));
	}
}

void Win32Loader::executeEntryPoint(uint64_t baseAddress, const Image &image)
{
	//security cookie
	if(image.info.platformData)
	{	
		if(image.info.architecture == ArchitectureWin32)
			*reinterpret_cast<uint32_t *>(baseAddress - image.info.baseAddress + image.info.platformData) += 10;
		else
			*reinterpret_cast<uint64_t *>(baseAddress - image.info.baseAddress + image.info.platformData) += 10;
	}
	if(image.info.platformData1)
	{
		typedef void (__stdcall *TlsCallbackType) (void *DllHandle, uint32_t Reason, void *Reserved);
		if(image.info.architecture == ArchitectureWin32)
		{
			IMAGE_TLS_DIRECTORY32 *directory = reinterpret_cast<IMAGE_TLS_DIRECTORY32 *>(image.info.platformData1 + baseAddress);
			uint32_t *callbacks = reinterpret_cast<uint32_t *>(directory->AddressOfCallBacks);
			while(*callbacks)
			{
				TlsCallbackType callback = reinterpret_cast<TlsCallbackType>(*callbacks);
				callback(reinterpret_cast<void *>(baseAddress), DLL_PROCESS_ATTACH, reinterpret_cast<void *>(1));
				callbacks ++;
			}
		}
		else
		{
			//TODO
		}
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
	for(auto &i = entryPointQueue_.begin(); i != entryPointQueue_.end(); )
	{
		auto oldi = i;
		uint64_t baseAddress = *i;
		i ++;
		entryPointQueue_.remove(oldi);
		executeEntryPoint(baseAddress, *loadedImages_[baseAddress]);
	}
}

uint64_t Win32Loader::loadImage(Image &image, bool asDataFile)
{
	uint64_t baseAddress = mapImage(image);
	if(!asDataFile)
		processImports(baseAddress, image);
	adjustPageProtection(baseAddress, image);
	if(!asDataFile)
		entryPointQueue_.push_back(baseAddress);
	return baseAddress;
}

void Win32Loader::execute()
{
	uint64_t baseAddress = mapImage(image_);
	Win32NativeHelper::get()->setMyBase(static_cast<size_t>(baseAddress));
	processImports(baseAddress, image_);
	adjustPageProtection(baseAddress, image_);

	executeEntryPointQueue();
	executeEntryPoint(baseAddress, image_);
}

template<typename HeaderType, typename EntryType, typename HostDescriptorType>
uint64_t Win32Loader::matchApiSet(const String &filename, uint8_t *apiSetBase)
{
	HeaderType *apiSet = reinterpret_cast<HeaderType *>(apiSetBase);
	auto item = binarySearch(apiSet->Entries, apiSet->Entries + apiSet->NumberOfEntries,
							 [&](const EntryType *entry) -> int {
		wchar_t *name = reinterpret_cast<wchar_t *>(apiSetBase + entry->Name);
		size_t i = 0;
		for(; i < entry->NameLength / sizeof(wchar_t) - 1; i ++)
			if(static_cast<char>(WString::to_lower(name[i])) != String::to_lower(filename[i + 4]))
				return static_cast<char>(WString::to_lower(name[i])) - String::to_lower(filename[i + 4]);
		return static_cast<char>(WString::to_lower(name[i])) - String::to_lower(filename[i + 4]);
	});
	if(item != apiSet->Entries + apiSet->NumberOfEntries)
	{
		HostDescriptorType *descriptor = reinterpret_cast<HostDescriptorType *>(apiSetBase + item->HostDescriptor);
		for(size_t i = descriptor->NumberOfHosts - 1; i >= 0; i --)
		{
			wchar_t *hostName = reinterpret_cast<wchar_t *>(apiSetBase + descriptor->Hosts[i].HostModuleName);
			WString moduleName(hostName, hostName + descriptor->Hosts[i].HostModuleNameLength / sizeof(wchar_t));
			uint64_t library;
			if((library = reinterpret_cast<uint64_t>(GetModuleHandleWProxy(moduleName.c_str()))) == 0)
			{
				library = loadLibrary(WStringToString(moduleName));
				loadedLibraries_.insert(filename, library);
			}
			if(library)
				return library;
		}
		return 0;
	}
	return 0;
}

uint64_t Win32Loader::loadLibrary(const String &filename, bool asDataFile)
{
	String normalizedFilename = filename;
	int pos;
	if((pos = filename.rfind('\\')) != -1)
		normalizedFilename = filename.substr(pos + 1);
	if(filename.find('.') == -1)
		normalizedFilename.append(".dll");

	auto &it = loadedLibraries_.find(normalizedFilename);
	if(it != loadedLibraries_.end())
		return it->value;

	//check if already loaded
	auto &images = Win32NativeHelper::get()->getLoadedImages();
	WString wstrName(StringToWString(normalizedFilename));
	for(auto &it = images.begin(); it != images.end(); it ++)
	{
		if(wstrName.icompare(it->fileName) == 0)
		{
			uint64_t baseAddress = it->baseAddress;
			PEFormat format;
			format.load(MakeShared<MemoryDataSource>(reinterpret_cast<uint8_t *>(baseAddress)), true);
			format.setFileName(normalizedFilename);
			auto &it = imports_.push_back(format.toImage());
			loadedLibraries_.insert(normalizedFilename, baseAddress);
			loadedImages_.insert(baseAddress, &*it);

			if(it->fileName.icompare("kernelbase.dll") == 0 || it->fileName.icompare("kernel32.dll") == 0)
			{
				//We need to patch ResolveDelayLoadedAPI
				for(auto &i : it->imports)
				{
					if(i.libraryName.icompare("ntdll.dll") == 0)
					{
						for(auto &j : i.functions)
						{
							if(j.nameHash == 0x6cac36c1)
							{
								size_t dest = static_cast<size_t>(baseAddress + j.iat);
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
		uint8_t *apiSetBase = Win32NativeHelper::get()->getApiSet();
		if(*reinterpret_cast<uint32_t *>(apiSetBase) == 2) // <= 8.0
			return matchApiSet<API_SET_HEADER, API_SET_ENTRY, API_SET_HOST_DESCRIPTOR>(filename, apiSetBase);
		else if(*reinterpret_cast<uint32_t *>(apiSetBase) == 4) // > 8.0
			return matchApiSet<API_SET_HEADER2, API_SET_ENTRY2, API_SET_HOST_DESCRIPTOR2>(filename, apiSetBase);
	}

	Image *image = nullptr;
	for(auto &i : imports_)
		if(i.fileName.icompare(filename) == 0)
			return loadImage(i, asDataFile);

	SharedPtr<FormatBase> format = FormatBase::loadImport(filename, image_.filePath, (asDataFile ? 0 : image_.info.architecture));
	if(!format.get())
		return 0;
	return loadImage(*imports_.push_back(format->toImage()), asDataFile);
}

uint64_t Win32Loader::getFunctionAddress(uint64_t library, const String &functionName, int ordinal)
{
	auto &it = loadedImages_.find(library);
	if(it != loadedImages_.end())
	{
		uint32_t functionNameHash = fnv1a(functionName.c_str(), functionName.length());
		const Image *image = it->value;
		if(image->fileName.icompare("kernel32.dll") == 0 || image->fileName.icompare("kernelbase.dll") == 0)
		{
			if(functionNameHash == 0x1cd12702)
				return reinterpret_cast<uint64_t>(LoadLibraryExWProxy);
			else if(functionNameHash == 0x6d10460)
				return reinterpret_cast<uint64_t>(LoadLibraryExAProxy);
			else if(functionNameHash == 0x41b1eab9)
				return reinterpret_cast<uint64_t>(LoadLibraryWProxy);
			else if(functionNameHash == 0x53b2070f)
				return reinterpret_cast<uint64_t>(LoadLibraryAProxy);
			else if(functionNameHash == 0xa9d0e95d)
				return reinterpret_cast<uint64_t>(GetModuleHandleExWProxy);
			else if(functionNameHash == 0xb3d0f91b)
				return reinterpret_cast<uint64_t>(GetModuleHandleExAProxy);
			else if(functionNameHash == 0xd263bde6)
				return reinterpret_cast<uint64_t>(GetModuleHandleWProxy);
			else if(functionNameHash == 0xe463da3c)
				return reinterpret_cast<uint64_t>(GetModuleHandleAProxy);
			else if(functionNameHash == 0xf8f45725)
				return reinterpret_cast<uint64_t>(GetProcAddressProxy);
			else if(functionNameHash == 0x96d3d469)
				return reinterpret_cast<uint64_t>(LdrResolveDelayLoadedAPIProxy);
			else if(functionNameHash == 0x99fbc63d)
				return reinterpret_cast<uint64_t>(GetModuleFileNameAProxy);
			else if(functionNameHash == 0xa3fbd5fb)
				return reinterpret_cast<uint64_t>(GetModuleFileNameWProxy);
			else if(functionNameHash == 0x1c59c83)
				return reinterpret_cast<uint64_t>(DisableThreadLibraryCallsProxy);
		}
		else if(image->fileName.icompare("ntdll.dll") == 0)
		{
			if(functionNameHash == 0x7385e79f)
				return reinterpret_cast<uint64_t>(LdrAddRefDllProxy);
			else if(functionNameHash == 0x7b566b5f)
				return reinterpret_cast<uint64_t>(LdrLoadDllProxy);
			else if(functionNameHash == 0x6cac36c1)
				return reinterpret_cast<uint64_t>(LdrResolveDelayLoadedAPIProxy);
			else if(functionNameHash == 0x9b08d96f)
				return reinterpret_cast<uint64_t>(LdrGetDllHandleProxy);
			else if(functionNameHash == 0x4738792)
				return reinterpret_cast<uint64_t>(LdrGetDllHandleExProxy);
			else if(functionNameHash == 0x1478f484)
				return reinterpret_cast<uint64_t>(LdrGetProcedureAddressProxy);
		}

		ExportFunction *item = nullptr;
		for(auto &i : image->exports)
		{
			if((functionNameHash != 0 && i.nameHash == functionNameHash) || (ordinal != -1 && ordinal == i.ordinal))
			{
				item = &i;
				break;
			}
		}
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
		return item->address + library;
	}
	return 0;
}

size_t __stdcall Win32Loader::DisableThreadLibraryCallsProxy(void *module)
{
	return 1;
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

void * __stdcall Win32Loader::LoadLibraryExWProxy(const wchar_t *libraryName, void *, uint32_t flags)
{
	UNICODE_STRING str;
	void *result;
	str.Buffer = const_cast<wchar_t *>(libraryName);
	str.Length = -1;
	str.MaximumLength = -1;

	LdrLoadDllProxy(nullptr, flags, &str, &result);
	return result;
}

uint32_t __stdcall Win32Loader::GetModuleFileNameAProxy(void *hModule, char *lpFilename, uint32_t nSize)
{
	String path;
	if(hModule == nullptr)
		path = File::combinePath(loaderInstance_->image_.filePath, loaderInstance_->image_.fileName);
	else
	{
		auto &it = loaderInstance_->loadedImages_.find(reinterpret_cast<uint64_t>(hModule));
		if(it != loaderInstance_->loadedImages_.end())
			path = File::combinePath(it->value->filePath, it->value->fileName);
	}
	if(nSize < path.length())
		return path.length();
	copyMemory(reinterpret_cast<uint8_t *>(lpFilename), reinterpret_cast<const uint8_t *>(path.c_str()), path.length() + 1);
	return path.length();
}

uint32_t __stdcall Win32Loader::GetModuleFileNameWProxy(void *hModule, wchar_t *lpFilename, uint32_t nSize)
{
	char temp[255];
	uint32_t result = GetModuleFileNameAProxy(hModule, temp, 255);
	WString wstr = StringToWString(String(temp));
	copyMemory(reinterpret_cast<uint8_t *>(lpFilename), reinterpret_cast<const uint8_t *>(wstr.c_str()), wstr.length() * 2 + 2);

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

uint32_t __stdcall Win32Loader::GetModuleHandleExAProxy(uint32_t flags, const char *filename_, void **result)
{
	if(!filename_ || flags == 4)
		return GetModuleHandleExWProxy(flags, reinterpret_cast<const wchar_t *>(filename_), result);
	return GetModuleHandleExWProxy(flags, StringToWString(String(filename_)).c_str(), result);
}

uint32_t __stdcall Win32Loader::GetModuleHandleExWProxy(uint32_t flags, const wchar_t *filename_, void **result)
{
	*result = 0;
	if(flags == 4) //GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
	{
		size_t address = reinterpret_cast<size_t>(filename_);
		for(auto &i : loaderInstance_->loadedImages_)
		{
			if(i.key <= address && address <= i.key + i.value->info.size)
			{
				*result = reinterpret_cast<void *>(i.key);
				return 1;
			}
		}
		return 0;
	}
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
		return reinterpret_cast<void *>(loaderInstance_->getFunctionAddress(reinterpret_cast<uint64_t>(library), String(), reinterpret_cast<size_t>(functionName)));
	return reinterpret_cast<void *>(loaderInstance_->getFunctionAddress(reinterpret_cast<uint64_t>(library), String(functionName)));
}

size_t __stdcall Win32Loader::LdrAddRefDllProxy(uint32_t flags, void *library)
{
	return 0;
}

size_t __stdcall Win32Loader::LdrLoadDllProxy(wchar_t *searchPath, size_t dllCharacteristics, UNICODE_STRING *dllName, void **baseAddress)
{
	List<uint64_t> entryPointQueueTemp(std::move(loaderInstance_->entryPointQueue_));
	void *result = reinterpret_cast<void *>(loaderInstance_->loadLibrary(WStringToString(WString(dllName->Buffer)), (dllCharacteristics & 0x02 ? true : false)));
	loaderInstance_->executeEntryPointQueue();
	loaderInstance_->entryPointQueue_ = std::move(entryPointQueueTemp);
	if(baseAddress)
		*baseAddress = result;

	return 0;
}

size_t __stdcall Win32Loader::LdrResolveDelayLoadedAPIProxy(void *base_, PCIMAGE_DELAYLOAD_DESCRIPTOR desc, void *dllhook, void *syshook, size_t *addr, size_t flags)
{
	uint8_t *base = reinterpret_cast<uint8_t *>(base_);
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
			functionAddress = static_cast<size_t>(loaderInstance_->getFunctionAddress(static_cast<uint64_t>(*moduleHandle), String(), nameOffset & 0xffff));
		else
			functionAddress = static_cast<size_t>(loaderInstance_->getFunctionAddress(static_cast<uint64_t>(*moduleHandle), String(nameEntry->Name)));

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

size_t __stdcall Win32Loader::LdrGetDllHandleProxy(wchar_t *DllPath, size_t DllCharacteristics, UNICODE_STRING *DllName, void **DllHandle)
{
	GetModuleHandleExWProxy(DllCharacteristics, DllName->Buffer, DllHandle);
	return 0;
}

size_t __stdcall Win32Loader::LdrGetDllHandleExProxy(size_t Flags, wchar_t *DllPath, size_t DllCharacteristics, UNICODE_STRING *DllName, void **DllHandle)
{
	GetModuleHandleExWProxy(DllCharacteristics, DllName->Buffer, DllHandle);
	return 0;
}

size_t __stdcall Win32Loader::LdrGetProcedureAddressProxy(void *BaseAddress, ANSI_STRING *Name, size_t Ordinal, void **ProcedureAddress)
{
	if(ProcedureAddress) 
		*ProcedureAddress = GetProcAddressProxy(BaseAddress, Name->Buffer);
	return 0;
}
