#include "Win32File.h"

#include "Util.h"

#include <windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

void Win32File::open(const String &filename)
{
	WString wFileName = StringToWString(filename);
	fileHandle_ = CreateFile(wFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if(fileHandle_ == INVALID_HANDLE_VALUE)
		return;

	if(PathIsRelative(wFileName.c_str()))
	{
		wchar_t temp[MAX_PATH + 1];
		GetCurrentDirectory(MAX_PATH, temp);
		fileName_ = filename;
		filePath_ = WStringToString(WString(temp));
	}
	else
	{
		wchar_t temp[MAX_PATH + 1];
		for(size_t i = 0; i < wFileName.length() + 1; i ++)
			temp[i] = wFileName[i];
		PathRemoveFileSpec(temp);
		filePath_ = WStringToString(WString(temp));
		fileName_ = WStringToString(WString(PathFindFileName(wFileName.c_str())));
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
	wchar_t temp[MAX_PATH + 1];
	PathCombine(temp, StringToWString(directory).c_str(), StringToWString(filename).c_str());
	return WStringToString(WString(temp));
}

bool File::isPathExists(const String &path)
{
	return PathFileExists(StringToWString(path).c_str()) == TRUE;
}
