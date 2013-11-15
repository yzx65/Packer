#pragma once

#include "Vector.h"
#include "String.h"
#include "List.h"
#include "Map.h"
#include "Image.h"

struct _UNICODE_STRING;
typedef _UNICODE_STRING UNICODE_STRING;

struct _ANSI_STRING;
typedef _ANSI_STRING ANSI_STRING;

struct _IMAGE_DELAYLOAD_DESCRIPTOR;
typedef _IMAGE_DELAYLOAD_DESCRIPTOR IMAGE_DELAYLOAD_DESCRIPTOR;
typedef const IMAGE_DELAYLOAD_DESCRIPTOR *PCIMAGE_DELAYLOAD_DESCRIPTOR;
class Win32Loader
{
private:
	Image image_;
	List<Image> imports_;
	List<uint64_t> entryPointQueue_;
	Map<uint64_t, const Image *> loadedImages_;
	Map<String, uint64_t, CaseInsensitiveStringComparator<String>> loadedLibraries_;
	uint64_t loadLibrary(const String &filename, bool asDataFile = false);
	uint64_t getFunctionAddress(uint64_t library, const String &functionName, int ordinal = -1);
	uint64_t loadImage(Image &image, bool asDataFile = false);
	uint64_t mapImage(Image &image);
	void processImports(uint64_t baseAddress, const Image &image);
	void adjustPageProtection(uint64_t baseAddress, const Image &image);
	void executeEntryPoint(uint64_t baseAddress, const Image &image);
	void executeEntryPointQueue();

	static uint32_t __stdcall GetModuleFileNameAProxy(void *hModule, char *lpFilename, uint32_t nSize);
	static uint32_t __stdcall GetModuleFileNameWProxy(void *hModule, wchar_t *lpFilename, uint32_t nSize);
	static void * __stdcall LoadLibraryAProxy(const char *libraryName);
	static void * __stdcall LoadLibraryWProxy(const wchar_t *libraryName);
	static void * __stdcall LoadLibraryExAProxy(const char *libraryName, void *, uint32_t);
	static void * __stdcall LoadLibraryExWProxy(const wchar_t *libraryName, void *, uint32_t flags);
	static void * __stdcall GetModuleHandleAProxy(const char *fileName);
	static void * __stdcall GetModuleHandleWProxy(const wchar_t *fileName);
	static uint32_t __stdcall GetModuleHandleExAProxy(uint32_t flags, const char *filename_, void **result);
	static uint32_t __stdcall GetModuleHandleExWProxy(uint32_t flags, const wchar_t *filename_, void **result);
	static void * __stdcall GetProcAddressProxy(void *library, char *functionName);
	static size_t __stdcall DisableThreadLibraryCallsProxy(void *module);
	static size_t __stdcall LdrAddRefDllProxy(size_t flags, void *library);
	static size_t __stdcall LdrLoadDllProxy(wchar_t *searchPath, size_t dllCharacteristics, UNICODE_STRING *dllName, void **baseAddress);
	static size_t __stdcall LdrResolveDelayLoadedAPIProxy(void *base, PCIMAGE_DELAYLOAD_DESCRIPTOR desc, void *dllhook, void *syshook, size_t *addr, size_t flags);
	static size_t __stdcall LdrGetDllHandleProxy(wchar_t *DllPath, size_t dllCharacteristics, UNICODE_STRING *dllName, void **dllHandle);
	static size_t __stdcall LdrGetDllHandleExProxy(size_t Flags, wchar_t *DllPath, size_t DllCharacteristics, UNICODE_STRING *DllName, void **DllHandle);
	static size_t __stdcall LdrGetProcedureAddressProxy(void *BaseAddress, ANSI_STRING *Name, size_t Ordinal, void **ProcedureAddress);
	
	template<typename HeaderType, typename EntryType, typename HostDescriptorType>
	uint64_t matchApiSet(const String &filename, uint8_t *apiSetBase);
public:
	Win32Loader(Image &&image, List<Image> &&imports);
	virtual ~Win32Loader() {}

	void execute();
};