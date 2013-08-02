#pragma once

#include <cstdint>

#include "Runtime.h"
#include "Vector.h"
#include "String.h"

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
};

enum SectionFlag
{
	SectionFlagCode = 1,
	SectionFlagData = 2,
	SectionFlagRead = 4,
	SectionFlagWrite = 8,
	SectionFlagExecute = 16,
};

struct ExtendedData //resource, header, ...
{
	ExtendedData() {}
	ExtendedData(ExtendedData &&operand) : baseAddress(operand.baseAddress), data(std::move(operand.data)) {}
	const ExtendedData &operator =(ExtendedData &&operand)
	{
		baseAddress = operand.baseAddress;
		data = std::move(operand.data);

		return *this;
	}
	uint64_t baseAddress;
	Vector<uint8_t> data;
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
	String name;
	uint64_t baseAddress;
	uint64_t size;
	Vector<uint8_t> data;
	uint32_t flag;
};

struct ImportFunction
{
	ImportFunction() {}
	ImportFunction(ImportFunction &&operand) : ordinal(operand.ordinal), name(std::move(operand.name)), iat(operand.iat) {}
	const ImportFunction &operator =(ImportFunction &&operand)
	{
		ordinal = operand.ordinal;
		name = std::move(operand.name);
		iat = operand.iat;

		return *this;
	}
	uint16_t ordinal;
	String name;
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
	ExportFunction(ExportFunction &&operand) : ordinal(operand.ordinal), name(std::move(operand.name)), address(operand.address), forward(std::move(operand.forward)) {}
	const ExportFunction &operator =(ExportFunction &&operand)
	{
		ordinal = operand.ordinal;
		name = std::move(operand.name);
		address = operand.address;
		forward = std::move(operand.forward);

		return *this;
	}
	uint16_t ordinal;
	String name;
	uint64_t address;
	String forward;
};

struct Image
{
	Image() {}
	Image(Image &&operand) : 
		info(operand.info), sections(std::move(operand.sections)), 
		imports(std::move(operand.imports)), relocations(std::move(operand.relocations)),
		extendedData(std::move(operand.extendedData)), fileName(std::move(operand.fileName)),
		exports(std::move(operand.exports)), nameExportLen(operand.nameExportLen) {}
	const Image &operator =(Image &&operand)
	{
		info = std::move(operand.info);
		sections = std::move(operand.sections);
		imports = std::move(operand.imports);
		relocations = std::move(operand.relocations);
		extendedData = std::move(operand.extendedData);
		fileName = std::move(operand.fileName);
		exports = std::move(operand.exports);
		nameExportLen = operand.nameExportLen;

		return *this;
	}
	ImageInfo info;
	String fileName;
	size_t nameExportLen;
	Vector<ExportFunction> exports;
	Vector<Section> sections;
	Vector<Import> imports;
	Vector<uint64_t> relocations;
	Vector<ExtendedData> extendedData;
};
