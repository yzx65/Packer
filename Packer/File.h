#pragma once

#include <cstdint>
#include <memory>

#include "Vector.h"

class File
{
public:
	File() {}
	virtual ~File() {}

	static std::shared_ptr<File> open(const std::string &filename);

	virtual std::string getFileName() = 0;
	virtual std::string getFilePath() = 0;

	virtual uint8_t *map() = 0;
	virtual void unmap() = 0;

	static std::string combinePath(const std::string &directory, const std::string &filename);
	static bool isPathExists(const std::string &path);
};
