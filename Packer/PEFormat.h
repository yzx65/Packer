#pragma once

#include "FormatBase.h"
#include "File.h"
#include "PEHeader.h"
#include "SharedPtr.h"
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
	List<ExportFunction> exports_;
	ImageInfo info_;

	String fileName_;
	String filePath_;

	void processDataDirectory();
	void processRelocation(IMAGE_BASE_RELOCATION *info);
	void processImport(IMAGE_IMPORT_DESCRIPTOR *descriptor);
	void processExport(IMAGE_EXPORT_DIRECTORY *directory);
	uint8_t *getDataPointerOfRVA(uint32_t rva);
public:
	PEFormat(uint8_t *data, const String &fileName, const String &filePath, bool fromLoaded = false);
	~PEFormat();

	virtual String getFilename();
	virtual SharedPtr<FormatBase> loadImport(const String &filename);
	virtual Image serialize();
	virtual List<Import> getImports();

	virtual bool isSystemLibrary(const String &filename);
};
