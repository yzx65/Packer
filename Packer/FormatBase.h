#pragma once

#include "List.h"
#include "Image.h"
#include "String.h"
#include "SharedPtr.h"

class FormatBase
{
public:
	FormatBase() {}
	virtual ~FormatBase() {}

	virtual Image serialize() = 0;
	virtual void setFileName(const String &fileName) = 0;
	virtual void setFilePath(const String &filePath) = 0;
	virtual String getFileName() = 0;
	virtual String getFilePath() = 0;
	virtual List<Import> getImports() = 0;

	virtual bool isSystemLibrary(const String &filename) = 0;

	static SharedPtr<FormatBase> loadImport(const String &filename, const String &hint = String(""));
};