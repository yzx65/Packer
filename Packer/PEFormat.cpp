#include "PEFormat.h"

#include "Signature.h"
#include "PEHeader.h"
#include "Util.h"

#include <algorithm>

#ifdef _WIN32
//We can't just include windows.h because of structure name in PEHeader.h is same as one in windows.h.
extern "C" {
	int __stdcall GetEnvironmentVariableW(
		_In_opt_   const wchar_t * lpName,
		_Out_opt_  wchar_t * lpBuffer,
		_In_       unsigned int nSize
		);
}
#endif

//Helper
class structureAtOffset
{
private:
	uint8_t *data_;
	size_t offset_;
public:
	structureAtOffset(uint8_t *data, size_t offset) : data_(data), offset_(offset) {}

	template<typename T>
	operator T()
	{
		return reinterpret_cast<T>(data_ + offset_);
	}
};
template<typename T>
inline T *getStructureAtOffset(uint8_t *data, size_t offset)
{
	return reinterpret_cast<T *>(data + offset);
}

PEFormat::PEFormat(uint8_t *data, const std::string &fileName, const std::string &filePath, bool fromLoaded) : data_(data), fileName_(fileName), filePath_(filePath)
{
	IMAGE_DOS_HEADER *dosHeader;
	uint32_t *ntSignature;
	IMAGE_FILE_HEADER *fileHeader;
	IMAGE_OPTIONAL_HEADER_BASE *optionalHeaderBase;
	uint32_t headerSize;
	size_t offset;

	dosHeader = structureAtOffset(data, 0);
	if(!dosHeader->e_lfanew)
		throw std::exception(); //not PE

	ntSignature = structureAtOffset(data, dosHeader->e_lfanew);
	if(*ntSignature != IMAGE_NT_SIGNATURE)
		throw std::exception(); //not PE
	fileHeader = structureAtOffset(data, dosHeader->e_lfanew + sizeof(uint32_t));
	optionalHeaderBase = structureAtOffset(data, dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER));

	offset = dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER_BASE);
	info_.entryPoint = optionalHeaderBase->AddressOfEntryPoint;
	if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER32 *optionalHeader = structureAtOffset(data, offset);
		std::copy(optionalHeader->DataDirectory, optionalHeader->DataDirectory + IMAGE_NUMBEROF_DIRECTORY_ENTRIES, dataDirectories_);
		info_.baseAddress = optionalHeader->ImageBase;
		info_.size = optionalHeader->SizeOfImage;
		info_.architecture = ArchitectureWin32;
		headerSize = optionalHeader->SizeOfHeaders;
		offset += sizeof(IMAGE_OPTIONAL_HEADER32);
	}
	else if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER64 *optionalHeader = structureAtOffset(data, offset);
		std::copy(optionalHeader->DataDirectory, optionalHeader->DataDirectory + IMAGE_NUMBEROF_DIRECTORY_ENTRIES, dataDirectories_);
		info_.baseAddress = optionalHeader->ImageBase;
		info_.size = optionalHeader->SizeOfImage;
		headerSize = optionalHeader->SizeOfHeaders;
		info_.architecture = ArchitectureWin32AMD64;
		offset += sizeof(IMAGE_OPTIONAL_HEADER64);
	}

	List<IMAGE_SECTION_HEADER> sectionHeaders;
	for(int i = 0; i < fileHeader->NumberOfSections; i ++)
	{
		IMAGE_SECTION_HEADER *sectionHeader = structureAtOffset(data, offset);
		sectionHeaders.push_back(std::move(*sectionHeader));
		offset += sizeof(IMAGE_SECTION_HEADER);
	}
	for(auto &sectionHeader : sectionHeaders)
	{
		Section section;
		section.baseAddress = sectionHeader.VirtualAddress;
		section.name = containerToDataStorage(std::string(reinterpret_cast<const char *>(sectionHeader.Name)));
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

		if(!fromLoaded)
			section.data.assign(data + sectionHeader.PointerToRawData, sectionHeader.VirtualSize);
		else
			section.data.assign(data + sectionHeader.VirtualAddress, sectionHeader.VirtualSize);

		sections_.push_back(std::move(section));
	}

	processDataDirectory();

	//copy header
	ExtendedData extendedData;
	extendedData.baseAddress = 0;
	extendedData.data.assign(data, headerSize);

	extendedData_.push_back(std::move(extendedData));

	if(dataDirectories_[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)
	{
		ExtendedData extendedData;
		extendedData.baseAddress = dataDirectories_[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
		extendedData.data.assign(getDataPointerOfRVA(dataDirectories_[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), dataDirectories_[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);

		extendedData_.push_back(std::move(extendedData));
	}
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
		import.libraryName = containerToDataStorage(std::string(reinterpret_cast<const char *>(libraryNamePtr)));

		uint32_t *nameEntryPtr = reinterpret_cast<uint32_t *>(getDataPointerOfRVA(descriptor->OriginalFirstThunk));
		uint64_t iat = descriptor->FirstThunk;

		List<ImportFunction> importFunctions;
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

				
				function.name = containerToDataStorage(std::string(reinterpret_cast<const char *>(nameEntry->Name)));
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
		import.functions = containerToDataStorage(std::move(importFunctions));

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
	return fileName_;
}

std::shared_ptr<FormatBase> PEFormat::loadImport(const std::string &filename)
{
	List<std::string> searchPaths;
	if(filePath_.size())
		searchPaths.push_back(filePath_);
#ifdef _WIN32
	wchar_t buffer[32768];
	GetEnvironmentVariableW(L"Path", buffer, 32768);

	std::wstring temp(buffer);
	int s = 0, e = 0;
	while(true)
	{
		e = temp.find(L';', e + 1);
		if(e == -1)
			break;
		searchPaths.push_back(WStringToString(temp.substr(s, e - s)));
		s = e + 1;
	}
#endif
	for(auto &i : searchPaths)
	{
		std::string path = File::combinePath(i, filename);
		if(File::isPathExists(path))
		{
			std::shared_ptr<File> file = File::open(path);
			uint8_t *map = file->map();
			std::shared_ptr<FormatBase> result = std::make_shared<PEFormat>(map, file->getFileName(), file->getFilePath());
			file->unmap();
			return result;
		}
	}
	return std::shared_ptr<FormatBase>(nullptr);
}

List<Import> PEFormat::getImports()
{
	return imports_;
}

Image PEFormat::serialize()
{
	Image image;
	image.fileName = containerToDataStorage(getFilename());
	image.info = info_;
	image.imports = containerToDataStorage(imports_);
	image.sections = containerToDataStorage(sections_);
	image.relocations = containerToDataStorage(relocations_);
	image.extendedData = containerToDataStorage(extendedData_);

	return std::move(image);
}

bool PEFormat::isSystemLibrary(const std::string &filename)
{
	std::string lowered;
	std::transform(filename.begin(), filename.end(), std::back_inserter(lowered), ::tolower);
	const char *systemFiles[] = {"kernel32.dll", "user32.dll", nullptr};
	for(int i = 0; systemFiles[i] != nullptr; i ++)
		if(lowered.compare(systemFiles[i]) == 0)
			return true;
	return false;
}
