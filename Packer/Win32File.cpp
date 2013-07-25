#include "Win32File.h"

#include "Util.h"

#include <windows.h>

void Win32File::open(const String &filename)
{
	WString wFileName = StringToWString(filename);
	fileHandle_ = CreateFile(wFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if(fileHandle_ == INVALID_HANDLE_VALUE)
		return;

	if(wFileName[1] != L':') //relative
	{
		wchar_t temp[MAX_PATH + 1];
		GetCurrentDirectory(MAX_PATH, temp);
		fileName_ = filename;
		filePath_ = WStringToString(WString(temp));
	}
	else
	{
		int pathSeparatorPos = wFileName.rfind(L'\\');
		WString path = wFileName.substr(0, pathSeparatorPos);
		WString fileName = wFileName.substr(pathSeparatorPos + 1);
		filePath_ = WStringToString(path);
		fileName_ = WStringToString(fileName);
	}
}

void Win32File::close() 
{
	CloseHandle(fileHandle_);
}

uint8_t *Win32File::map()
{
	if(mapAddress_)
		return mapAddress_;
	mapHandle_ = CreateFileMapping(fileHandle_, NULL, PAGE_READONLY, 0, 0, NULL);
	mapAddress_ = static_cast<uint8_t *>(MapViewOfFile(mapHandle_, FILE_MAP_READ, 0, 0, 0));
	return mapAddress_;
}

void Win32File::unmap()
{
	if(!mapAddress_)
		return;
	UnmapViewOfFile(static_cast<LPVOID>(mapAddress_));
	CloseHandle(mapHandle_);
	mapHandle_ = mapAddress_ = nullptr;
}

String Win32File::getFileName()
{
	return fileName_;
}

String Win32File::getFilePath()
{
	return filePath_;
}

String File::combinePath(const String &directory, const String &filename)
{
	return directory + '\\' + filename;
}

bool File::isPathExists(const String &path)
{
	if(GetFileAttributes(StringToWString(path).c_str()) == INVALID_FILE_ATTRIBUTES)
		return false;
	return true;
}
