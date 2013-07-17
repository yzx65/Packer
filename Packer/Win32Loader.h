#pragma once

#include "Executable.h"

class Win32Loader
{
private:
	const Executable &executable_;
	void *loadLibrary(const char *filename);
	uint32_t getFunctionAddress(void *library, const char *functionName);
public:
	Win32Loader(const Executable &executable);
	virtual ~Win32Loader() {}

	void load();
};