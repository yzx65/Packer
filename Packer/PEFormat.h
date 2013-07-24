#pragma once

#include <memory>

#include "FormatBase.h"
#include "File.h"
#include "PEHeader.h"
#include "List.h"

class PEFormat : public FormatBase
{
private:
	IMAGE_DATA_DIRECTORY dataDirectories_[16];
	uint8_t *data_;
	List<Section> sections_;
	List<Import> imports_;
	List<uint64_t> relocations_;
	List<ExtendedData> extendedData_;
	ImageInfo info_;

	std::string fileName_;
	std::string filePath_;

	void processDataDirectory();
	void processRelocation(IMAGE_BASE_RELOCATION *info);
	void processImport(IMAGE_IMPORT_DESCRIPTOR *descriptor);
	uint8_t *getDataPointerOfRVA(uint64_t rva);
public:
	PEFormat(uint8_t *data, const std::string &fileName, const std::string &filePath, bool fromLoaded = false);
	~PEFormat();

	virtual std::string getFilename();
	virtual std::shared_ptr<FormatBase> loadImport(const std::string &filename);
	virtual Image serialize();
	virtual List<Import> getImports();

	virtual bool isSystemLibrary(const std::string &filename);
};
