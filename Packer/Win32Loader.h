#pragma once

#include "Vector.h"
#include "String.h"
#include "List.h"
#include "Map.h"
#include "Image.h"

class Win32Loader
{
private:
	const Image &image_;
	List<Image> imports_;
	Map<uint64_t, const Image *> loadedImages_;
	Map<String, uint64_t, CaseInsensitiveStringComparator<String>> loadedLibraries_;
	void *loadLibrary(const String &filename);
	uint64_t getFunctionAddress(void *library, const String &functionName);
	uint8_t *loadImage(const Image &image);

	static void * __stdcall LoadLibraryExProxy(wchar_t *libraryName, void *, uint32_t);
	static uint32_t __stdcall GetModuleHandleExProxy(uint32_t, wchar_t *filename_, void **result);
public:
	Win32Loader(const Image &image, Vector<Image> &&imports);
	virtual ~Win32Loader() {}

	void execute();
};