#pragma once

#include <cstdint>

#include "../Util/TypeTraits.h"
#include "../Util/Vector.h"
#include "../Util/List.h"
#include "../Util/String.h"
#include "../Util/DataSource.h"

enum ArchitectureType
{
	ArchitectureWin32 = 1,
	ArchitectureWin32AMD64 = 2,
};

enum ImageFlag
{
	ImageFlagLibrary = 1,
};

struct ImageInfo
{
	ArchitectureType architecture;
	uint64_t baseAddress;
	uint64_t entryPoint;
	uint64_t size;
	uint32_t flag;

	uint64_t platformData; //PE: security cookie
	uint64_t platformData1; //PE: tls entry
};

enum SectionFlag
{
	SectionFlagCode = 1,
	SectionFlagData = 2,
	SectionFlagUninitializedData = 4,
	SectionFlagRead = 8,
	SectionFlagWrite = 16,
	SectionFlagExecute = 32,
};

struct Section
{
	Section() {}
	Section(Section &&operand) : name(std::move(operand.name)), baseAddress(operand.baseAddress), size(operand.size), data(std::move(operand.data)), flag(operand.flag) {}
	const Section &operator =(Section &&operand)
	{
		name = std::move(operand.name);
		baseAddress = operand.baseAddress;
		size = operand.size;
		data = std::move(operand.data);
		flag = operand.flag;

		return *this;
	}

	const Section &operator =(const Section &operand)
	{
		name = operand.name;
		baseAddress = operand.baseAddress;
		size = operand.size;
		data = operand.data;
		flag = operand.flag;

		return *this;
	}
	String name;
	uint64_t baseAddress;
	uint64_t size;
	SharedPtr<DataView> data;
	uint32_t flag;
};

struct ImportFunction
{
	ImportFunction() {}
	ImportFunction(ImportFunction &&operand) : ordinal(operand.ordinal), name(std::move(operand.name)), iat(operand.iat), nameHash(operand.nameHash) {}
	const ImportFunction &operator =(ImportFunction &&operand)
	{
		ordinal = operand.ordinal;
		name = std::move(operand.name);
		nameHash = operand.nameHash;
		iat = operand.iat;

		return *this;
	}
	uint16_t ordinal;
	String name;
	uint32_t nameHash;
	uint64_t iat;
};

struct Import
{
	Import() {}
	Import(Import &&operand) : libraryName(std::move(operand.libraryName)), functions(std::move(operand.functions)) {}
	const Import &operator =(Import &&operand)
	{
		libraryName = std::move(operand.libraryName);
		functions = std::move(operand.functions);

		return *this;
	}
	String libraryName;
	Vector<ImportFunction> functions;
};

struct ExportFunction
{
	ExportFunction() {}
	ExportFunction(ExportFunction &&operand) : ordinal(operand.ordinal), name(std::move(operand.name)), address(operand.address), forward(std::move(operand.forward)), nameHash(operand.nameHash) {}
	const ExportFunction &operator =(ExportFunction &&operand)
	{
		ordinal = operand.ordinal;
		name = std::move(operand.name);
		nameHash = operand.nameHash;
		address = operand.address;
		forward = std::move(operand.forward);

		return *this;
	}
	uint16_t ordinal;
	String name;
	uint32_t nameHash;
	uint64_t address;
	String forward;
};

struct Image
{
	Image() {}
	Image(Image &&operand) : 
		info(operand.info), sections(std::move(operand.sections)), 
		imports(std::move(operand.imports)), relocations(std::move(operand.relocations)),
		fileName(std::move(operand.fileName)), filePath(std::move(operand.filePath)), 
		exports(std::move(operand.exports)), 
		header(std::move(operand.header)){}
	const Image &operator =(Image &&operand)
	{
		info = std::move(operand.info);
		sections = std::move(operand.sections);
		imports = std::move(operand.imports);
		relocations = std::move(operand.relocations);
		fileName = std::move(operand.fileName);
		filePath = std::move(operand.filePath);
		exports = std::move(operand.exports);
		header = std::move(operand.header);

		return *this;
	}
	ImageInfo info;
	String fileName;
	String filePath;
	uint32_t nameExportLen;
	Vector<ExportFunction> exports;
	List<Section> sections;
	List<Import> imports;
	List<uint64_t> relocations;
	SharedPtr<DataView> header;

	Vector<uint8_t> serialize() const;
	static Image unserialize(SharedPtr<DataView> data, size_t *processedSize);
};

