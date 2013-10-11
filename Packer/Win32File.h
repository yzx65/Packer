#pragma once

#include "File.h"

class Win32File : public File
{
	friend class Win32FileView;
private:
	void *fileHandle_;
	void *mapHandle_;
	uint8_t *mapAddress_;
	int mapCounter_;
	void open(const String &filename, bool write);
	void close();
	String fileName_;
	String filePath_;
public:
	Win32File(const String &filename, bool write = false);
	virtual ~Win32File();

	virtual String getFileName();
	virtual String getFilePath();
	virtual void *getHandle();

	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size);
	virtual uint8_t *map(uint64_t offset);
	virtual void unmap();
	virtual void write(uint8_t *data, size_t size);
};
