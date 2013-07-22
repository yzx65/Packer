#pragma once

#include "Vector.h"
#include "Image.h"

class Win32Loader
{
private:
	const Image &image_;
	Vector<Image> imports_;
	void *loadLibrary(const char *filename);
	uint32_t getFunctionAddress(void *library, const char *functionName);
	uint8_t *loadImage(const Image &image);
public:
	Win32Loader(const Image &image, Vector<Image> &&imports);
	virtual ~Win32Loader() {}

	void execute();
};