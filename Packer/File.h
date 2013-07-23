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

	virtual uint32_t read(int size, uint8_t *out) = 0;
	virtual void seek(uint64_t newPosition) = 0;
	virtual std::string getFileName() = 0;
	virtual std::string getFilePath() = 0;
	static std::string combinePath(const std::string &directory, const std::string &filename);
	static bool isPathExists(const std::string &path);

	template<typename ValueType>
	void read(ValueType *dest)
	{
		read(sizeof(ValueType), reinterpret_cast<uint8_t *>(dest));
	}

	Vector<uint8_t> readAmount(size_t size)
	{
		Vector<uint8_t> result;
		result.resize(size);
		read(size, &result[0]);
		return result;
	}
};
