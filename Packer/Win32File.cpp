#include "Win32File.h"

#include "Util.h"

#include "Win32Runtime.h"

void Win32File::open(const String &filename)
{
	WString wFileName = StringToWString(filename);
	WString fullPath;

	if(wFileName[1] != L':') //relative
	{
		WString currentDirectory(Win32NativeHelper::get()->getCurrentDirectory());
		
		fileName_ = filename;
		filePath_ = WStringToString(currentDirectory);
		fullPath = currentDirectory + wFileName;
	}
	else
	{
		int pathSeparatorPos = wFileName.rfind(L'\\');
		WString path = wFileName.substr(0, pathSeparatorPos);
		WString fileName = wFileName.substr(pathSeparatorPos + 1);
		filePath_ = WStringToString(path);
		fileName_ = WStringToString(fileName);
		fullPath = wFileName;
	}

	fileHandle_ = Win32NativeHelper::get()->createFile(GENERIC_READ, fullPath.c_str(), fullPath.length(), 0, FILE_SHARE_READ, FILE_OPEN, 0);
	if(fileHandle_ == INVALID_HANDLE_VALUE)
		return;
}

void Win32File::close() 
{
	Win32NativeHelper::get()->closeHandle(fileHandle_);
}

uint8_t *Win32File::map()
{
	if(mapAddress_)
		return mapAddress_;
	mapHandle_ = Win32NativeHelper::get()->createSection(fileHandle_, PAGE_READONLY, 0, 0, NULL, 0);
	mapAddress_ = static_cast<uint8_t *>(Win32NativeHelper::get()->mapViewOfSection(mapHandle_, FILE_MAP_READ, 0, 0, 0, nullptr));
	return mapAddress_;
}

void Win32File::unmap()
{
	if(!mapAddress_)
		return;
	Win32NativeHelper::get()->unmapViewOfSection(mapAddress_);
	Win32NativeHelper::get()->closeHandle(mapHandle_);
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
	WString widePath = StringToWString(path);
	if(Win32NativeHelper::get()->getFileAttributes(widePath.c_str(), widePath.length()) == INVALID_FILE_ATTRIBUTES)
		return false;
	return true;
}
