#pragma once

#include "File.h"

class Win32File : public File
{
private:
	void *fileHandle_;
	void *mapHandle_;
	uint8_t *mapAddress_;
	void open(const String &filename);
	void close();
	String fileName_;
	String filePath_;
public:
	Win32File(const String &filename) : mapHandle_(nullptr), mapAddress_(nullptr)
	{
		open(filename);
	}
	virtual ~Win32File()
	{
		unmap();
		close();
	}

	virtual String getFileName();
	virtual String getFilePath();

	virtual uint8_t *map();
	virtual void unmap();
};

SharedPtr<File> File::open(const String &filename)
{
	return MakeShared<Win32File>(filename);
}
