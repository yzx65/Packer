#pragma once

#include <cstdint>
#include <utility>

#include "Vector.h"
#include "List.h"
#include "Image.h"
#include "SharedPtr.h"

class FormatBase
{
public:
	FormatBase() {}
	virtual ~FormatBase() {}

	virtual Image serialize() = 0;
	virtual String getFilename() = 0;
	virtual SharedPtr<FormatBase> loadImport(const String &filename) = 0;
	virtual List<Import> getImports() = 0;

	virtual bool isSystemLibrary(const String &filename) = 0;
};