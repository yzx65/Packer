#pragma once

#include <string>
#include <exception>

#include "File.h"

class Win32File : public File
{
private:
	void *fileHandle_;
	void *mapHandle_;
	uint8_t *mapAddress_;
	void open(const std::string &filename);
	void close();
	std::string fileName_;
	std::string filePath_;
public:
	Win32File(const std::string &filename) : mapHandle_(nullptr), mapAddress_(nullptr)
	{
		open(filename);
	}
	virtual ~Win32File()
	{
		unmap();
		close();
	}

	virtual std::string getFileName();
	virtual std::string getFilePath();

	virtual uint8_t *map();
	virtual void unmap();
};

std::shared_ptr<File> File::open(const std::string &filename)
{
	return std::make_shared<Win32File>(filename);
}
