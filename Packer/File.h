#pragma once

#include <cstdint>

#include "String.h"
#include "SharedPtr.h"
#include "DataSource.h"

class File : public DataSource, public EnableSharedFromThis<File>
{
public:
	File() {}
	virtual ~File() {}

	static SharedPtr<File> open(const String &filename, bool write = false);

	virtual String getFileName() = 0;
	virtual String getFilePath() = 0;
	virtual void *getHandle() = 0;
	virtual void resize(uint64_t newSize) = 0;
	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size) = 0;
	virtual void write(const uint8_t *data, size_t size) = 0;

	void write(const Vector<uint8_t> &data)
	{
		write(data.get(), data.size());
	}
	template<typename T>
	void write(T *data, size_t size)
	{
		write(reinterpret_cast<const uint8_t *>(data), size);
	}

	static String combinePath(const String &directory, const String &filename);
	static bool isPathExists(const String &path);
};

