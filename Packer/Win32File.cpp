#include "Win32File.h"

#include "Util.h"
#include <windows.h>

void Win32File::open(const std::string &filename)
{
	fileHandle_ = CreateFile(StringToWString(filename).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if(fileHandle_ == INVALID_HANDLE_VALUE)
		throw std::exception();
}

void Win32File::close() 
{
	CloseHandle(fileHandle_);
}

uint32_t Win32File::read(int size, uint8_t *out)
{
	DWORD readBytes;
	if(ReadFile(fileHandle_, out, size, &readBytes, nullptr) == FALSE)
		throw std::exception();
	
	return readBytes;
}

void Win32File::seek(uint64_t newPosition)
{
	LARGE_INTEGER distance;
	distance.QuadPart = newPosition;

	SetFilePointerEx(fileHandle_, distance, nullptr, FILE_BEGIN);
}

std::string Win32File::getFilename()
{
	return filename_;
}