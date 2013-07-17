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
	std::string filename_;
public:
	Win32File(const std::string &filename) : filename_(filename)
	{
		open(filename);
	}
	virtual ~Win32File()
	{
		close();
	}

	virtual uint32_t read(int size, uint8_t *out);
	virtual void seek(uint64_t newPosition);
	virtual std::string getFilename();
};

std::shared_ptr<File> File::open(const std::string &filename)
{
	return std::make_shared<Win32File>(filename);
}
