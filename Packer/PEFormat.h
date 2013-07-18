#pragma once

#include <array>
#include <memory>
#include <list>

#include "FormatBase.h"
#include "File.h"
#include "PEHeader.h"

class PEFormat : public FormatBase
{
private:
	std::array<IMAGE_DATA_DIRECTORY, 16> dataDirectories_;
	std::shared_ptr<File> file_;
	std::list<Section> sections_;
	std::list<Import> imports_;
	std::list<uint64_t> relocations_;
	std::list<ExtendedData> extendedData_;
	ImageInfo info_;
	void processDataDirectory();
	void processRelocation(IMAGE_BASE_RELOCATION *info);
	void processImport(IMAGE_IMPORT_DESCRIPTOR *descriptor);
	uint8_t *getDataPointerOfRVA(uint64_t rva);
public:
	PEFormat(std::shared_ptr<File> file);
	~PEFormat();

	virtual std::string getFilename();
	virtual std::shared_ptr<FormatBase> loadImport(const std::string &filename);
	virtual Image serialize();
	virtual std::list<Import> getImports();

	virtual bool isSystemLibrary(const std::string &filename);
};
