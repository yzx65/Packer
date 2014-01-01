#pragma once

#include "FormatBase.h"
#include "File.h"
#include "../Util/SharedPtr.h"
#include "../Util/List.h"
#include "../Util/DataSource.h"

struct _IMAGE_DATA_DIRECTORY;
typedef _IMAGE_DATA_DIRECTORY IMAGE_DATA_DIRECTORY;
class PEFormat : public FormatBase
{
private:
	List<Section> sections_;
	List<Import> imports_;
	List<uint64_t> relocations_;
	List<ExportFunction> exports_;
	SharedPtr<DataView> header_;
	ImageInfo info_;
	size_t dataDirectoryBase_;
	bool processedRelocation_;
	bool processedImport_;
	bool processedExport_;

	String fileName_;
	String filePath_;

	size_t loadHeader(SharedPtr<DataSource> source, bool fromMemory);
	void processRelocation();
	void processImport();
	void processExport();
	uint8_t *getDataPointerOfRVA(uint32_t rva);
	String PEFormat::checkExportForwarder(uint64_t address, size_t exportTableBase, size_t exportTableSize);
	IMAGE_DATA_DIRECTORY *getDataDirectory(size_t index);
public:
	PEFormat();
	virtual ~PEFormat();

	virtual bool load(SharedPtr<DataSource> source, bool fromMemory);
	virtual void setFileName(const String &fileName);
	virtual void setFilePath(const String &filePath);
	virtual const String &getFileName() const;
	virtual const String &getFilePath() const;
	virtual Image toImage();
	virtual const List<Import> &getImports();
	virtual const List<ExportFunction> &getExports();
	virtual const ImageInfo &getInfo() const;
	virtual const List<uint64_t> &getRelocations();
	virtual const List<Section> &getSections() const;

	virtual void setSections(const List<Section> &sections);
	virtual void setRelocations(const List<uint64_t> &relocations);
	virtual void setImageInfo(const ImageInfo &info);

	virtual void save(SharedPtr<DataSource> target);
	virtual size_t estimateSize() const;

	virtual bool isSystemLibrary(const String &filename);
};
