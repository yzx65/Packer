#pragma once

#include "Vector.h"
#include "String.h"
#include "List.h"
#include "Map.h"
#include "Image.h"

struct _UNICODE_STRING;
typedef _UNICODE_STRING UNICODE_STRING;

struct _IMAGE_DELAYLOAD_DESCRIPTOR;
typedef _IMAGE_DELAYLOAD_DESCRIPTOR IMAGE_DELAYLOAD_DESCRIPTOR;
typedef const IMAGE_DELAYLOAD_DESCRIPTOR *PCIMAGE_DELAYLOAD_DESCRIPTOR;
class Win32Loader
{
private:
	Image &image_;
	List<Image> imports_;
	List<uint64_t> entryPointQueue_;
	Map<uint64_t, const Image *> loadedImages_;
	Map<String, uint64_t, CaseInsensitiveStringComparator<String>> loadedLibraries_;
	void *loadLibrary(const String &filename);
	uint64_t getFunctionAddress(void *library, const String &functionName, int ordinal = -1	);
	uint8_t *loadImage(Image &image);
	uint8_t *mapImage(Image &image);
	void processImports(uint8_t *baseAddress, const Image &image);
	void adjustPageProtection(uint8_t *baseAddress, const Image &image);
	void executeEntryPoint(uint8_t *baseAddress, const Image &image);
	void executeEntryPointQueue();

	static uint32_t __stdcall GetModuleFileNameAProxy(void *hModule, char *lpFilename, uint32_t nSize);
	static uint32_t __stdcall GetModuleFileNameWProxy(void *hModule, wchar_t *lpFilename, uint32_t nSize);
	static void * __stdcall LoadLibraryAProxy(const char *libraryName);
	static void * __stdcall LoadLibraryWProxy(const wchar_t *libraryName);
	static void * __stdcall LoadLibraryExAProxy(const char *libraryName, void *, uint32_t);
	static void * __stdcall LoadLibraryExWProxy(const wchar_t *libraryName, void *, uint32_t);
	static void * __stdcall GetModuleHandleAProxy(const char *fileName);
	static void * __stdcall GetModuleHandleWProxy(const wchar_t *fileName);
	static uint32_t __stdcall GetModuleHandleExAProxy(uint32_t, const char *filename_, void **result);
	static uint32_t __stdcall GetModuleHandleExWProxy(uint32_t, const wchar_t *filename_, void **result);
	static void * __stdcall GetProcAddressProxy(void *library, char *functionName);
	static uint32_t __stdcall LdrAddRefDllProxy(uint32_t flags, void *library);
	static uint32_t __stdcall LdrLoadDllProxy(wchar_t *searchPath, size_t *dllCharacteristics, UNICODE_STRING *dllName, void **baseAddress);
	static size_t __stdcall LdrResolveDelayLoadedAPIProxy(uint8_t *base, PCIMAGE_DELAYLOAD_DESCRIPTOR desc, void *dllhook, void *syshook, size_t *addr, size_t flags);
public:
	Win32Loader(Image &image, Vector<Image> &&imports);
	virtual ~Win32Loader() {}

	void execute();
};