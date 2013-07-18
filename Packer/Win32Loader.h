#pragma once

#include "DataStorage.h"
#include "Executable.h"

class Win32Loader
{
private:
	const Executable &executable_;
	const DataStorage<Executable> &imports_;
	void *loadLibrary(const char *filename);
	uint32_t getFunctionAddress(void *library, const char *functionName);
	uint8_t *loadExecutable(const Executable &executable);
public:
	Win32Loader(const Executable &executable, const DataStorage<Executable> imports);
	virtual ~Win32Loader() {}

	void execute();
};