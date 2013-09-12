#pragma once

#include "File.h"

class Win32File : public File
{
	friend class Win32FileView;
private:
	void *fileHandle_;
	void *mapHandle_;
	uint8_t *mapAddress_;
	void open(const String &filename);
	void close();
	String fileName_;
	String filePath_;
public:
	Win32File(const String &filename);
	virtual ~Win32File();

	virtual String getFileName();
	virtual String getFilePath();
	virtual void *getHandle();

	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size);
};

SharedPtr<File> File::open(const String &filename)
{
	return MakeShared<Win32File>(filename);
}
