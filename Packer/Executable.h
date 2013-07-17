#pragma once

#include <cstdint>
#include <utility>
#include "DataStorage.h"

enum ArchitectureType
{
	ArchitectureWin32,
	ArchitectureWin32AMD64,
};
struct ExecutableInfo
{
	ArchitectureType architecture;
	uint64_t baseAddress;
	uint64_t entryPoint;
	uint64_t size;
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
	DataStorage<uint8_t> data;
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
	DataStorage<char> name;
	uint64_t baseAddress;
	uint64_t size;
	DataStorage<uint8_t> data;
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
	DataStorage<char> name;
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
	DataStorage<char> libraryName;
	DataStorage<ImportFunction> functions;
};

struct Executable
{
	Executable() {}
	Executable(Executable &&operand) : info(operand.info), sections(std::move(operand.sections)), imports(std::move(operand.imports)), relocations(std::move(operand.relocations)), extendedData(std::move(operand.extendedData)), fileName(std::move(operand.fileName)) {}
	const Executable &operator =(Executable &&operand)
	{
		info = std::move(operand.info);
		sections = std::move(operand.sections);
		imports = std::move(operand.imports);
		relocations = std::move(operand.relocations);
		extendedData = std::move(operand.extendedData);
		fileName = std::move(operand.fileName);

		return *this;
	}
	ExecutableInfo info;
	DataStorage<char> fileName;
	DataStorage<Section> sections;
	DataStorage<Import> imports;
	DataStorage<uint64_t> relocations;
	DataStorage<ExtendedData> extendedData;
};
