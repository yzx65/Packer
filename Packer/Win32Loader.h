#pragma once

#include "Vector.h"
#include "String.h"
#include "List.h"
#include "Map.h"
#include "Image.h"

class Win32Loader
{
private:
	uint64_t mainBase;
	const Image &image_;
	List<Image> imports_;
	Map<uint64_t, const Image *> loadedImages_;
	Map<String, uint64_t, CaseInsensitiveStringComparator<String>> loadedLibraries_;
	void *loadLibrary(const String &filename);
	uint64_t getFunctionAddress(void *library, const String &functionName, int ordinal = -1	);
	uint8_t *loadImage(const Image &image, bool executable = false);

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
public:
	Win32Loader(const Image &image, Vector<Image> &&imports);
	virtual ~Win32Loader() {}

	void execute();
};