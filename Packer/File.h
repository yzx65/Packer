#pragma once

#include <cstdint>

#include "String.h"
#include "SharedPtr.h"

class File
{
public:
	File() {}
	virtual ~File() {}

	static SharedPtr<File> open(const String &filename);

	virtual String getFileName() = 0;
	virtual String getFilePath() = 0;

	virtual uint8_t *map() = 0;
	virtual void unmap() = 0;

	static String combinePath(const String &directory, const String &filename);
	static bool isPathExists(const String &path);
};
