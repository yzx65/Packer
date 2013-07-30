#pragma once

#include "Vector.h"
#include "List.h"
#include "Map.h"
#include "Image.h"

class Win32Loader
{
private:
	const Image &image_;
	List<Image> imports_;
	Map<uint64_t, const Image *> loadedImages_;
	Map<String, uint64_t> loadedLibraries_;
	void *loadLibrary(const char *filename);
	uint64_t getFunctionAddress(void *library, const char *functionName);
	uint8_t *loadImage(const Image &image);
public:
	Win32Loader(const Image &image, Vector<Image> &&imports);
	virtual ~Win32Loader() {}

	void execute();
};