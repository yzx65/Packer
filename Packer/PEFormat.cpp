#include "PEFormat.h"

#include "Signature.h"
#include "PEHeader.h"
#include "Util.h"

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

PEFormat::PEFormat(uint8_t *data, const String &fileName, const String &filePath, bool fromLoaded) : data_(data), fileName_(fileName), filePath_(filePath)
{
	IMAGE_DOS_HEADER *dosHeader;
	uint32_t *ntSignature;
	IMAGE_FILE_HEADER *fileHeader;
	IMAGE_OPTIONAL_HEADER_BASE *optionalHeaderBase;
	uint32_t headerSize;
	size_t offset;

	dosHeader = structureAtOffset(data, 0);
	if(!dosHeader->e_lfanew)
		return; //not PE

	ntSignature = structureAtOffset(data, dosHeader->e_lfanew);
	if(*ntSignature != IMAGE_NT_SIGNATURE)
		return; //not PE
	fileHeader = structureAtOffset(data, dosHeader->e_lfanew + sizeof(uint32_t));
	optionalHeaderBase = structureAtOffset(data, dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER));

	offset = dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER_BASE);
	info_.entryPoint = optionalHeaderBase->AddressOfEntryPoint;
	if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER32 *optionalHeader = structureAtOffset(data, offset);
		for(int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i ++)
			dataDirectories_[i] = optionalHeader->DataDirectory[i];
		info_.baseAddress = optionalHeader->ImageBase;
		info_.size = optionalHeader->SizeOfImage;
		info_.architecture = ArchitectureWin32;
		headerSize = optionalHeader->SizeOfHeaders;
		offset += sizeof(IMAGE_OPTIONAL_HEADER32);
	}
	else if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER64 *optionalHeader = structureAtOffset(data, offset);
		for(int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i ++)
			dataDirectories_[i] = optionalHeader->DataDirectory[i];
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
		section.name.assign(reinterpret_cast<const char *>(sectionHeader.Name));
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

uint8_t *PEFormat::getDataPointerOfRVA(uint32_t rva)
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
		import.libraryName.assign(reinterpret_cast<const char *>(libraryNamePtr));

		uint32_t *nameEntryPtr = reinterpret_cast<uint32_t *>(getDataPointerOfRVA(descriptor->OriginalFirstThunk));
		uint64_t iat = descriptor->FirstThunk;

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
					nameEntry = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(getDataPointerOfRVA(static_cast<uint32_t>(*reinterpret_cast<uint64_t *>(nameEntryPtr))));
				else
					nameEntry = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(getDataPointerOfRVA(*reinterpret_cast<uint32_t *>(nameEntryPtr)));

				
				function.name.assign(reinterpret_cast<const char *>(nameEntry->Name));
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

			import.functions.push_back(std::move(function));
		}

		imports_.push_back(std::move(import));

		descriptor ++;
	}
}

void PEFormat::processExport(IMAGE_EXPORT_DIRECTORY *directory)
{
	if(!directory)
		return;

	uint32_t *addressOfFunctions = reinterpret_cast<uint32_t *>(getDataPointerOfRVA(directory->AddressOfFunctions));
	uint32_t *addressOfNames = reinterpret_cast<uint32_t *>(getDataPointerOfRVA(directory->AddressOfNames));
	uint16_t *ordinals = reinterpret_cast<uint16_t *>(getDataPointerOfRVA(directory->AddressOfNameOrdinals));
	for(size_t i = 0; i < directory->NumberOfFunctions; i ++)
	{
		ExportFunction entry;
		if(addressOfNames[i])
			entry.name.assign(reinterpret_cast<const char *>(getDataPointerOfRVA(addressOfNames[i])));
		entry.ordinal = ordinals[i];
		if(info_.architecture == ArchitectureWin32AMD64)
			entry.address = (reinterpret_cast<uint64_t *>(addressOfFunctions))[i];
		else
			entry.address = addressOfFunctions[i];

		exports_.push_back(entry);
	}
}

void PEFormat::processDataDirectory()
{
	processImport(reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(getDataPointerOfRVA(dataDirectories_[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)));
	processExport(reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(getDataPointerOfRVA(dataDirectories_[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)));
	processRelocation(reinterpret_cast<IMAGE_BASE_RELOCATION *>(getDataPointerOfRVA(dataDirectories_[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)));
}

String PEFormat::getFilename()
{
	return fileName_;
}

SharedPtr<FormatBase> PEFormat::loadImport(const String &filename)
{
	List<String> searchPaths;
	if(filePath_.length())
		searchPaths.push_back(filePath_);
#ifdef _WIN32
	WString temp;
	temp.resize(32768);
	GetEnvironmentVariableW(L"Path", &temp[0], 32768);
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
		String path = File::combinePath(i, filename);
		if(File::isPathExists(path))
		{
			SharedPtr<File> file = File::open(path);
			uint8_t *map = file->map();
			SharedPtr<FormatBase> result = MakeShared<PEFormat>(map, file->getFileName(), file->getFilePath());
			file->unmap();
			return result;
		}
	}
	return SharedPtr<FormatBase>(nullptr);
}

List<Import> PEFormat::getImports()
{
	return imports_;
}

Image PEFormat::serialize()
{
	Image image;
	image.fileName = getFilename();
	image.info = info_;
	image.imports.assign(imports_.begin(), imports_.end());
	image.sections.assign(sections_.begin(), sections_.end());
	image.relocations.assign(relocations_.begin(), relocations_.end());
	image.extendedData.assign(extendedData_.begin(), extendedData_.end());
	image.exports.assign(exports_.begin(), exports_.end());

	return std::move(image);
}

bool PEFormat::isSystemLibrary(const String &filename)
{
	String lowered;
	for(auto &i : filename)
		lowered.push_back((i >= 'A' && i <= 'Z' ? i - ('A' - 'a') : i));
	const char *systemFiles[] = {"kernel32.dll", "user32.dll", nullptr};
	for(int i = 0; systemFiles[i] != nullptr; i ++)
		if(lowered.compare(systemFiles[i]) == 0)
			return true;
	return false;
}
