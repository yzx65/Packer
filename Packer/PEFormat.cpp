#include "PEFormat.h"

#include "Signature.h"
#include "PEHeader.h"
#include "Util.h"

#ifdef _WIN32
#include "Win32Runtime.h" //for path search.
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

PEFormat::PEFormat(uint8_t *data, bool fromLoaded, bool fullLoad)
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
	info_.flag = 0;
	if(fileHeader->Characteristics & IMAGE_FILE_DLL)
		info_.flag |= ImageFlagLibrary;

	if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER32 *optionalHeader = structureAtOffset(data, offset);
		dataDirectories_ = optionalHeader->DataDirectory;
		info_.baseAddress = optionalHeader->ImageBase;
		info_.size = optionalHeader->SizeOfImage;
		info_.architecture = ArchitectureWin32;
		headerSize = optionalHeader->SizeOfHeaders;
		offset += sizeof(IMAGE_OPTIONAL_HEADER32);
	}
	else if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER64 *optionalHeader = structureAtOffset(data, offset);
		dataDirectories_ = optionalHeader->DataDirectory;
		info_.baseAddress = optionalHeader->ImageBase;
		info_.size = optionalHeader->SizeOfImage;
		headerSize = optionalHeader->SizeOfHeaders;
		info_.architecture = ArchitectureWin32AMD64;
		offset += sizeof(IMAGE_OPTIONAL_HEADER64);
	}

	if(fullLoad)
	{
		loadSectionData(fileHeader->NumberOfSections, data, offset, fromLoaded);
		processExtra(data, headerSize);
	}
}

PEFormat::~PEFormat()
{

}

void PEFormat::processExtra(uint8_t *data, size_t headerSize)
{
	IMAGE_DATA_DIRECTORY *dataDirectories = reinterpret_cast<IMAGE_DATA_DIRECTORY *>(dataDirectories_);
	processImport(getDataPointerOfRVA(dataDirectories[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress));
	processExport(getDataPointerOfRVA(dataDirectories[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
	processRelocation(getDataPointerOfRVA(dataDirectories[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress));

	//copy header
	ExtendedData extendedData;
	extendedData.baseAddress = 0;
	extendedData.data.assign(data, headerSize);

	extendedData_.push_back(std::move(extendedData));

	if(dataDirectories[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size)
	{
		ExtendedData extendedData;
		extendedData.baseAddress = dataDirectories[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
		extendedData.data.assign(getDataPointerOfRVA(dataDirectories[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress), dataDirectories[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);

		extendedData_.push_back(std::move(extendedData));
	}
}

void PEFormat::loadSectionData(int numberOfSections, uint8_t *data, size_t offset, bool fromLoaded)
{
	IMAGE_SECTION_HEADER *sectionHeaders = structureAtOffset(data, offset);
	for(int i = 0; i < numberOfSections; i ++)
	{
		Section section;
		section.baseAddress = sectionHeaders[i].VirtualAddress;
		section.name.assign(reinterpret_cast<const char *>(sectionHeaders[i].Name));
		section.size = sectionHeaders[i].VirtualSize;
		section.flag = 0;

		if(sectionHeaders[i].Characteristics & IMAGE_SCN_CNT_CODE)
			section.flag |= SectionFlagCode;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
			section.flag |= SectionFlagData;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
			section.flag |= SectionFlagData;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_MEM_READ)
			section.flag |= SectionFlagRead;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_MEM_WRITE)
			section.flag |= SectionFlagWrite;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)
			section.flag |= SectionFlagExecute;

		if(!fromLoaded)
			section.data.assign(data + sectionHeaders[i].PointerToRawData, sectionHeaders[i].SizeOfRawData);
		else
			section.data.assign(data + sectionHeaders[i].VirtualAddress, sectionHeaders[i].SizeOfRawData);

		sections_.push_back(std::move(section));
	}
}

uint8_t *PEFormat::getDataPointerOfRVA(uint32_t rva)
{
	for(auto &section : sections_)
		if(rva >= section.baseAddress && rva < section.baseAddress + section.size)
			return &section.data[static_cast<size_t>(rva - section.baseAddress)];
	return nullptr;
}

void PEFormat::processRelocation(uint8_t *info_)
{
	IMAGE_BASE_RELOCATION *info = reinterpret_cast<IMAGE_BASE_RELOCATION *>(info_);
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

void PEFormat::processImport(uint8_t *descriptor_)
{
	IMAGE_IMPORT_DESCRIPTOR *descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(descriptor_);
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

String PEFormat::checkExportForwarder(uint64_t address)
{
	IMAGE_DATA_DIRECTORY *dataDirectories = reinterpret_cast<IMAGE_DATA_DIRECTORY *>(dataDirectories_);
	if(address >= dataDirectories[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress && 
		address < dataDirectories[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + dataDirectories[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)
		for(auto &i : sections_)
			if(address >= i.baseAddress && address < i.baseAddress + i.size)
				return String(reinterpret_cast<char *>(&i.data[static_cast<uint32_t>(address - i.baseAddress)]));
	return "";
}

void PEFormat::processExport(uint8_t *directory_)
{
	IMAGE_EXPORT_DIRECTORY *directory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(directory_);
	if(!directory)
		return;

	bool *checker = new bool[directory->NumberOfFunctions];
	for(size_t i = 0; i < directory->NumberOfFunctions; i ++)
		checker[i] = false;

	uint32_t *addressOfFunctions = reinterpret_cast<uint32_t *>(getDataPointerOfRVA(directory->AddressOfFunctions));
	uint32_t *addressOfNames = reinterpret_cast<uint32_t *>(getDataPointerOfRVA(directory->AddressOfNames));
	uint16_t *ordinals = reinterpret_cast<uint16_t *>(getDataPointerOfRVA(directory->AddressOfNameOrdinals));
	for(size_t i = 0; i < directory->NumberOfNames; i ++)
	{
		ExportFunction entry;
		if(addressOfNames && addressOfNames[i])
			entry.name.assign(reinterpret_cast<const char *>(getDataPointerOfRVA(addressOfNames[i])));

		entry.ordinal = ordinals[i];
		entry.address = addressOfFunctions[entry.ordinal];
		checker[entry.ordinal] = true;
		entry.ordinal += directory->Base;
		entry.forward = checkExportForwarder(entry.address);

		exports_.push_back(entry);
	}

	for(size_t i = 0; i < directory->NumberOfNames; i ++)
	{
		if(checker[i] == true)
			continue;
		//entry without names
		ExportFunction entry;
		entry.ordinal = i + directory->Base;
		entry.address = addressOfFunctions[i];
		entry.forward = checkExportForwarder(entry.address);

		exports_.push_back(entry);
	}
	delete [] checker;
	nameExportLen_ = directory->NumberOfNames;
}

void PEFormat::setFileName(const String &fileName)
{
	fileName_ = fileName;
}

void PEFormat::setFilePath(const String &filePath)
{
	filePath_ = filePath;
}

String PEFormat::getFileName()
{
	return fileName_;
}

String PEFormat::getFilePath()
{
	return filePath_;
}

List<Import> PEFormat::getImports()
{
	return imports_;
}

List<ExportFunction> PEFormat::getExports()
{
	return exports_;
}

Image PEFormat::serialize()
{
	Image image;
	image.fileName = getFileName();
	image.info = info_;
	image.imports.assign_move(imports_.begin(), imports_.end());
	image.sections.assign_move(sections_.begin(), sections_.end());
	image.relocations.assign_move(relocations_.begin(), relocations_.end());
	image.extendedData.assign_move(extendedData_.begin(), extendedData_.end());
	image.exports.assign_move(exports_.begin(), exports_.end());
	image.nameExportLen = nameExportLen_;

	return image;
}

bool PEFormat::isSystemLibrary(const String &filename)
{
	const char *systemFiles[] = {"kernel32.dll", "user32.dll", nullptr};
	for(int i = 0; systemFiles[i] != nullptr; i ++)
		if(filename.icompare(systemFiles[i]) == 0)
			return true;
	return false;
}

void *PEFormat::getDataDirectories()
{
	return dataDirectories_;
}

SharedPtr<FormatBase> FormatBase::loadImport(const String &filename, SharedPtr<FormatBase> hint)
{
	List<String> searchPaths;
	if(hint.get() && hint->getFilePath().length())
		searchPaths.push_back(hint->getFilePath());
#ifdef _WIN32
	wchar_t *environmentBlock = Win32NativeHelper::get()->getEnvironments();
	WString path;
	while(*environmentBlock)
	{
		size_t equal = 0;
		size_t currentLength = 0;
		wchar_t *start = environmentBlock;
		while(*environmentBlock ++) 
		{
			if(*environmentBlock == L'=')
				equal = currentLength;
			currentLength ++;
		}
		if(equal >= 3 && WString::to_lower(start[0]) == L'p'
			 && WString::to_lower(start[1]) == L'a'
			  && WString::to_lower(start[2]) == L't'
			   && WString::to_lower(start[3]) == L'h'
			    && start[4] == L'=')
		{
			path.assign(start + equal + 2);
			break;
		}
	}
	int s = 0, e = 0;
	while(true)
	{
		e = path.find(L';', e + 1);
		if(e == -1)
			break;
		searchPaths.push_back(WStringToString(path.substr(s, e - s)));
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
			SharedPtr<FormatBase> result = MakeShared<PEFormat>(map);
			result->setFileName(file->getFileName());
			result->setFilePath(file->getFilePath());
			file->unmap();
			return result;
		}
	}
	return SharedPtr<FormatBase>(nullptr);
}
