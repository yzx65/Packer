#pragma once

#include "FormatBase.h"
#include "File.h"
#include "SharedPtr.h"
#include "List.h"
#include "DataSource.h"

class PEFormat : public FormatBase
{
private:
	List<Section> sections_;
	List<Import> imports_;
	List<uint64_t> relocations_;
	List<ExportFunction> exports_;
	SharedPtr<DataView> header_;
	ImageInfo info_;
	size_t nameExportLen_;
	uint32_t exportTableBase_;
	uint32_t exportTableSize_;

	String fileName_;
	String filePath_;

	size_t loadHeader(SharedPtr<DataSource> source, bool fromMemory);
	void processRelocation(uint8_t *info_);
	void processImport(uint8_t *descriptor_);
	void processExport(uint8_t *directory_);
	uint8_t *getDataPointerOfRVA(uint32_t rva);
	String PEFormat::checkExportForwarder(uint64_t address);
public:
	PEFormat();
	virtual ~PEFormat();

	virtual bool load(SharedPtr<DataSource> source, bool fromMemory);
	virtual void setFileName(const String &fileName);
	virtual void setFilePath(const String &filePath);
	virtual const String &getFileName() const;
	virtual const String &getFilePath() const;
	virtual Image serialize();
	virtual const List<Import> &getImports() const;
	virtual const List<ExportFunction> &getExports() const;

	virtual bool isSystemLibrary(const String &filename);
};
