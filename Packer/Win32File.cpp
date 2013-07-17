#include "Win32File.h"

#include "Util.h"

#include <array>

#include <windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

void Win32File::open(const std::string &filename)
{
	std::wstring wFileName = StringToWString(filename);
	fileHandle_ = CreateFile(wFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if(fileHandle_ == INVALID_HANDLE_VALUE)
		throw std::exception();

	if(PathIsRelative(wFileName.c_str()))
	{
		wchar_t temp[MAX_PATH + 1];
		GetCurrentDirectory(MAX_PATH, temp);
		fileName_ = filename;
		filePath_ = WStringToString(std::wstring(temp));
	}
	else
	{
		wchar_t temp[MAX_PATH + 1];
		std::copy(wFileName.begin(), wFileName.end(), temp);
		PathRemoveFileSpec(temp);
		filePath_ = WStringToString(std::wstring(temp));
		fileName_ = WStringToString(std::wstring(PathFindFileName(wFileName.c_str())));
	}
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

std::string Win32File::getFileName()
{
	return fileName_;
}

std::string Win32File::getFilePath()
{
	return filePath_;
}

std::string File::combinePath(const std::string &directory, const std::string &filename)
{
	wchar_t temp[MAX_PATH + 1];
	PathCombine(temp, StringToWString(directory).c_str(), StringToWString(filename).c_str());
	return WStringToString(std::wstring(temp));
}

bool File::isPathExists(const std::string &path)
{
	return PathFileExists(StringToWString(path).c_str()) == TRUE;
}
