#pragma once

#include "FormatBase.h"
#include "File.h"
#include "SharedPtr.h"
#include "List.h"

class PEFormat : public FormatBase
{
private:
	List<Section> sections_;
	List<Import> imports_;
	List<uint64_t> relocations_;
	List<ExtendedData> extendedData_;
	List<ExportFunction> exports_;
	ImageInfo info_;

	String fileName_;
	String filePath_;

	void processRelocation(uint8_t *info_);
	void processImport(uint8_t *descriptor_);
	void processExport(uint8_t *directory_);
	uint8_t *getDataPointerOfRVA(uint32_t rva);
	void loadSectionData(int numberOfSections, uint8_t *data, size_t offset, bool fromLoaded);
	void processExtra(void *dataDirectories_, uint8_t *data, size_t headerSize);
public:
	PEFormat(uint8_t *data, bool fromLoaded = false);
	~PEFormat();

	virtual void setFileName(const String &fileName);
	virtual void setFilePath(const String &filePath);
	virtual String getFileName();
	virtual SharedPtr<FormatBase> loadImport(const String &filename);
	virtual Image serialize();
	virtual List<Import> getImports();
	virtual List<ExportFunction> getExports();

	virtual bool isSystemLibrary(const String &filename);
};
