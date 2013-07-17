#include "PEFormat.h"

#include "Signature.h"
#include "PEHeader.h"

#include <list>

PEFormat::PEFormat(std::shared_ptr<File> file): file_(file)
{
	IMAGE_DOS_HEADER dosHeader;
	uint32_t ntSignature;
	IMAGE_FILE_HEADER fileHeader;
	IMAGE_OPTIONAL_HEADER_BASE optionalHeaderBase;
	uint32_t headerSize;

	file_->read(&dosHeader);
	if(!dosHeader.e_lfanew)
		throw std::exception(); //not PE
	file_->seek(dosHeader.e_lfanew);
	file_->read(&ntSignature);
	if(ntSignature != IMAGE_NT_SIGNATURE)
		throw std::exception(); //not PE
	file_->read(&fileHeader);
	file_->read(&optionalHeaderBase);

	info_.entryPoint = optionalHeaderBase.AddressOfEntryPoint;
	if(optionalHeaderBase.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER32 optionalHeader;
		file_->read(&optionalHeader);
		std::copy(optionalHeader.DataDirectory, optionalHeader.DataDirectory + IMAGE_NUMBEROF_DIRECTORY_ENTRIES, dataDirectories_.begin());
		info_.baseAddress = optionalHeader.ImageBase;
		info_.size = optionalHeader.SizeOfImage;
		info_.architecture = ArchitectureWin32;
		headerSize = optionalHeader.SizeOfHeaders;
	}
	else if(optionalHeaderBase.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER64 optionalHeader;
		file_->read(&optionalHeader);
		std::copy(optionalHeader.DataDirectory, optionalHeader.DataDirectory + IMAGE_NUMBEROF_DIRECTORY_ENTRIES, dataDirectories_.begin());
		info_.baseAddress = optionalHeader.ImageBase;
		info_.size = optionalHeader.SizeOfImage;
		headerSize = optionalHeader.SizeOfHeaders;
		info_.architecture = ArchitectureWin32AMD64;
	}

	std::list<IMAGE_SECTION_HEADER> sectionHeaders;
	for(int i = 0; i < fileHeader.NumberOfSections; i ++)
	{
		IMAGE_SECTION_HEADER sectionHeader;
		file_->read(&sectionHeader);
		sectionHeaders.push_back(std::move(sectionHeader));
	}
	for(auto &sectionHeader : sectionHeaders)
	{
		Section section;
		section.baseAddress = sectionHeader.VirtualAddress;
		containerToDataStorage(section.name, std::string(reinterpret_cast<const char *>(sectionHeader.Name)));
		section.size = sectionHeader.VirtualSize;
		section.flag = 0;

		if(sectionHeader.Characteristics & IMAGE_SCN_CNT_CODE)
			section.flag |= SectionFlagCode;
		if(sectionHeader.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
			section.flag |= SectionFlagData;
		if(sectionHeader.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
			section.flag |= SectionFlagData;
		if(sectionHeader.Characteristics & IMAGE_SCN_MEM_READ)
			section.flag |= SectionFlagRead;
		if(sectionHeader.Characteristics & IMAGE_SCN_MEM_WRITE)
			section.flag |= SectionFlagWrite;
		if(sectionHeader.Characteristics & IMAGE_SCN_MEM_EXECUTE)
			section.flag |= SectionFlagExecute;

		file_->seek(sectionHeader.PointerToRawData);
		section.data = std::move(file_->readAmount(sectionHeader.SizeOfRawData));

		sections_.push_back(std::move(section));
	}

	processDataDirectory();

	//copy header
	file_->seek(0);
	ExtendedData extendedData;
	extendedData.baseAddress = 0;
	extendedData.data = std::move(file_->readAmount(headerSize));

	extendedData_.push_back(std::move(extendedData));
}

PEFormat::~PEFormat()
{

}

uint8_t *PEFormat::getDataPointerOfRVA(uint64_t rva)
{
	for(auto &section : sections_)
		if(rva >= section.baseAddress && rva < section.baseAddress + section.size)
			return &section.data[static_cast<size_t>(rva - section.baseAddress)];
	return nullptr;
}

void PEFormat::processRelocation(IMAGE_BASE_RELOCATION *info)
{
	if(!info)
		return;
	while(true)
	{
		if(info->SizeOfBlock == 0)
			break;
		uint16_t *ptr = reinterpret_cast<uint16_t *>(info + 1);
		for(size_t i = 0; i < (info->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t); i ++)
		{
			uint8_t type = (*ptr & 0xf000) >> 12;
			uint16_t offset = *ptr & 0x0fff;
			if(type == 0)
				break;
			relocations_.push_back(static_cast<uint64_t>(info->VirtualAddress) + offset);
			ptr ++;
		}
		info = reinterpret_cast<IMAGE_BASE_RELOCATION *>(reinterpret_cast<uint8_t *>(info) + info->SizeOfBlock);
	}
}

void PEFormat::processImport(IMAGE_IMPORT_DESCRIPTOR *descriptor)
{
	if(!descriptor)
		return;
	while(true)
	{
		if(descriptor->OriginalFirstThunk == 0)
			break;

		Import import;

		uint8_t *libraryNamePtr = getDataPointerOfRVA(descriptor->Name);
		containerToDataStorage(import.libraryName, std::string(reinterpret_cast<const char *>(libraryNamePtr)));

		uint32_t *nameEntryPtr = reinterpret_cast<uint32_t *>(getDataPointerOfRVA(descriptor->OriginalFirstThunk));
		uint64_t iat = descriptor->FirstThunk;

		std::list<ImportFunction> importFunctions;
		while(true)
		{
			if(*nameEntryPtr == 0)
				break;

			ImportFunction function;
			function.ordinal = 0;
			function.iat = iat;

			if((info_.architecture == ArchitectureWin32AMD64 && (*reinterpret_cast<uint64_t *>(nameEntryPtr) & IMAGE_ORDINAL_FLAG64)) || (*reinterpret_cast<uint32_t *>(nameEntryPtr) & IMAGE_ORDINAL_FLAG32))
			{
				if(info_.architecture == ArchitectureWin32AMD64)
					function.ordinal = *reinterpret_cast<uint64_t *>(nameEntryPtr) & 0xffff;
				else
					function.ordinal = *reinterpret_cast<uint32_t *>(nameEntryPtr) & 0xffff;
			}
			else
			{
				IMAGE_IMPORT_BY_NAME *nameEntry;
				if(info_.architecture == ArchitectureWin32AMD64)
					nameEntry = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(getDataPointerOfRVA(*reinterpret_cast<uint64_t *>(nameEntryPtr)));
				else
					nameEntry = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(getDataPointerOfRVA(*reinterpret_cast<uint32_t *>(nameEntryPtr)));

				
				containerToDataStorage(function.name, std::string(reinterpret_cast<const char *>(nameEntry->Name)));
			}

			if(info_.architecture == ArchitectureWin32AMD64)
			{
				nameEntryPtr += 2;
				iat += 8;
			}
			else
			{
				nameEntryPtr ++;
				iat += 4;
			}

			importFunctions.push_back(std::move(function));
		}
		containerToDataStorage(import.functions, std::move(importFunctions));

		imports_.push_back(std::move(import));

		descriptor ++;
	}
}

void PEFormat::processDataDirectory()
{
	processImport(reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(getDataPointerOfRVA(dataDirectories_[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)));
	processRelocation(reinterpret_cast<IMAGE_BASE_RELOCATION *>(getDataPointerOfRVA(dataDirectories_[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)));
}

std::string PEFormat::getFilename()
{
	return file_->getFilename();
}

std::shared_ptr<FormatBase> PEFormat::loadImport(const std::string &filename)
{
	return std::make_shared<PEFormat>(nullptr);
}

bool PEFormat::isSystemLibrary()
{
	return true;
}

std::list<Import> PEFormat::getImports()
{
	return imports_;
}

Executable PEFormat::serialize()
{
	Executable executable;
	executable.info = info_;
	containerToDataStorage(executable.imports, imports_);
	containerToDataStorage(executable.sections, sections_);
	containerToDataStorage(executable.relocations, relocations_);
	containerToDataStorage(executable.extendedData, extendedData_);

	return std::move(executable);
}
