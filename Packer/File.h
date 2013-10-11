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

	static SharedPtr<File> open(const String &filename);

	virtual String getFileName() = 0;
	virtual String getFilePath() = 0;
	virtual void *getHandle() = 0;
	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size) = 0;
	void write(const Vector<uint8_t> &data)
	{
		write(data.get(), data.size());
	}
	virtual void write(uint8_t *data, size_t size) = 0;

	static String combinePath(const String &directory, const String &filename);
	static bool isPathExists(const String &path);
};

