#include "Win32File.h"

#include "Util.h"

#include "Win32Runtime.h"

Win32File::Win32File(const String &filename, bool write) : mapCounter_(0), write_(write), mapHandle_(INVALID_HANDLE_VALUE)
{
	open(filename);
}

Win32File::~Win32File()
{
	close();
}

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

	int access = GENERIC_READ;
	int disposition = FILE_OPEN;

	if(write_)
	{
		access |= GENERIC_WRITE;
		disposition = FILE_OVERWRITE_IF;
	}

	fileHandle_ = Win32NativeHelper::get()->createFile(access, fullPath.c_str(), fullPath.length(), FILE_SHARE_READ, disposition);
	if(fileHandle_ == INVALID_HANDLE_VALUE)
		return;
}

void Win32File::close() 
{
	Win32NativeHelper::get()->closeHandle(mapHandle_);
	Win32NativeHelper::get()->closeHandle(fileHandle_);
}

String Win32File::getFileName()
{
	return fileName_;
}

String Win32File::getFilePath()
{
	return filePath_;
}

void *Win32File::getHandle()
{
	return fileHandle_;
}

void Win32File::write(const uint8_t *data, size_t size)
{
	Win32NativeHelper::get()->writeFile(fileHandle_, data, size);
}

SharedPtr<DataView> Win32File::getView(uint64_t offset, size_t size)
{
	return MakeShared<DataView>(sharedFromThis(), offset, size);
}

uint8_t *Win32File::map(uint64_t offset)
{
	if(mapHandle_ == INVALID_HANDLE_VALUE)
	{
		int sectionProtect = PAGE_READONLY;
		if(write_)
			sectionProtect = PAGE_READWRITE;
		mapHandle_ = Win32NativeHelper::get()->createSection(fileHandle_, sectionProtect, 0, nullptr, 0);
	}

	if(mapCounter_ == 0)
	{
		uint32_t access = FILE_MAP_READ;
		if(write_)
			access = FILE_MAP_READ | FILE_MAP_WRITE;
		mapAddress_ = static_cast<uint8_t *>(Win32NativeHelper::get()->mapViewOfSection(mapHandle_, access, 0, 0, 0));
	}

	mapCounter_ ++;
	return mapAddress_ + offset;
}

void Win32File::unmap()
{
	mapCounter_ --;
	if(mapCounter_ == 0)
	{
		Win32NativeHelper::get()->unmapViewOfSection(mapAddress_);
		mapAddress_ = 0;
	}
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

SharedPtr<File> File::open(const String &filename)
{
	return MakeShared<Win32File>(filename);
}
