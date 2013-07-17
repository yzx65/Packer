#pragma once

#include <string>
#include <exception>

#include "File.h"

class Win32File : public File
{
private:
	void *fileHandle_;
	void open(const std::string &filename);
	void close();
	std::string fileName_;
	std::string filePath_;
public:
	Win32File(const std::string &filename)
	{
		open(filename);
	}
	virtual ~Win32File()
	{
		close();
	}

	virtual uint32_t read(int size, uint8_t *out);
	virtual void seek(uint64_t newPosition);
	virtual std::string getFileName();
	virtual std::string getFilePath();
};

std::shared_ptr<File> File::open(const std::string &filename)
{
	return std::make_shared<Win32File>(filename);
}
