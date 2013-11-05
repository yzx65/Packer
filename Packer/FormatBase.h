#pragma once

#include "List.h"
#include "Image.h"
#include "String.h"
#include "SharedPtr.h"
#include "DataSource.h"

class FormatBase
{
public:
	FormatBase() {}
	virtual ~FormatBase() {}

	virtual bool load(SharedPtr<DataSource> source, bool fromMemory) = 0;
	virtual Image toImage() = 0;
	virtual void setFileName(const String &fileName) = 0;
	virtual void setFilePath(const String &filePath) = 0;
	virtual const String &getFileName() const = 0;
	virtual const String &getFilePath() const = 0;
	virtual const List<Import> &getImports() const = 0;
	virtual const List<ExportFunction> &getExports() const = 0;

	virtual bool isSystemLibrary(const String &filename) = 0;

	static SharedPtr<FormatBase> loadImport(const String &filename, const String &hint = String(""));
};