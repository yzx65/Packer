#include "PEFormat.h"

#include "Signature.h"
#include "PEHeader.h"
#include "../Util/Util.h"
#include "../Util/Map.h"

#ifdef _WIN32
#include "../Win32/Win32Runtime.h" //for path search.
#endif

template<typename T>
inline T *getStructureAtOffset(uint8_t *data, size_t offset)
{
	return reinterpret_cast<T *>(data + offset);
}

PEFormat::PEFormat() : processedExport_(false), processedImport_(false), processedRelocation_(false)
{
	
}

PEFormat::~PEFormat()
{

}

bool PEFormat::load(SharedPtr<DataSource> source, bool fromMemory)
{
	size_t headerSize = loadHeader(source, fromMemory);
	if(headerSize == 0)
		return false;
	header_ = source->getView(0, headerSize);

	return true;
}

size_t PEFormat::loadHeader(SharedPtr<DataSource> source, bool fromMemory)
{
	IMAGE_DOS_HEADER *dosHeader;
	uint32_t *ntSignature;
	IMAGE_FILE_HEADER *fileHeader;
	IMAGE_OPTIONAL_HEADER_BASE *optionalHeaderBase;
	IMAGE_DATA_DIRECTORY *dataDirectory = nullptr;
	size_t offset;
	size_t headerSize;
	SharedPtr<DataView> view = source->getView(0, 0);
	uint8_t *data = view->get();

	dosHeader = getStructureAtOffset<IMAGE_DOS_HEADER>(data, 0);
	if(!dosHeader->e_lfanew)
		return 0; //not PE

	ntSignature = getStructureAtOffset<uint32_t>(data, dosHeader->e_lfanew);
	if(*ntSignature != IMAGE_NT_SIGNATURE)
		return 0; //not PE
	fileHeader = getStructureAtOffset<IMAGE_FILE_HEADER>(data, dosHeader->e_lfanew + sizeof(uint32_t));
	optionalHeaderBase = getStructureAtOffset<IMAGE_OPTIONAL_HEADER_BASE>(data, dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER));

	offset = dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER);
	info_.entryPoint = optionalHeaderBase->AddressOfEntryPoint;
	info_.flag = 0;
	if(fileHeader->Characteristics & IMAGE_FILE_DLL)
		info_.flag |= ImageFlagLibrary;

	if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER32 *optionalHeader = getStructureAtOffset<IMAGE_OPTIONAL_HEADER32>(data, offset);
		dataDirectory = optionalHeader->DataDirectory;
		dataDirectoryBase_ = reinterpret_cast<uint8_t *>(dataDirectory) - data;
		info_.baseAddress = optionalHeader->ImageBase;
		info_.size = optionalHeader->SizeOfImage;
		info_.architecture = ArchitectureWin32;
		headerSize = optionalHeader->SizeOfHeaders;
	}
	else if(optionalHeaderBase->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER64 *optionalHeader = getStructureAtOffset<IMAGE_OPTIONAL_HEADER64>(data, offset);
		dataDirectory = optionalHeader->DataDirectory;
		dataDirectoryBase_ = reinterpret_cast<uint8_t *>(dataDirectory) - data;
		info_.baseAddress = optionalHeader->ImageBase;
		info_.size = optionalHeader->SizeOfImage;
		headerSize = optionalHeader->SizeOfHeaders;
		info_.architecture = ArchitectureWin32AMD64;
	}
	offset += fileHeader->SizeOfOptionalHeader;

	IMAGE_SECTION_HEADER *sectionHeaders = getStructureAtOffset<IMAGE_SECTION_HEADER>(data, offset);
	for(int i = 0; i < fileHeader->NumberOfSections; i ++)
	{
		Section section;
		section.baseAddress = sectionHeaders[i].VirtualAddress;
		section.name.assign(sectionHeaders[i].Name, sectionHeaders[i].Name + 8);
		section.size = sectionHeaders[i].VirtualSize;
		section.flag = 0;

		if(sectionHeaders[i].Characteristics & IMAGE_SCN_CNT_CODE)
			section.flag |= SectionFlagCode;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
			section.flag |= SectionFlagData;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
			section.flag |= SectionFlagUninitializedData;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_MEM_READ)
			section.flag |= SectionFlagRead;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_MEM_WRITE)
			section.flag |= SectionFlagWrite;
		if(sectionHeaders[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)
			section.flag |= SectionFlagExecute;

		if(!fromMemory)
			section.data = source->getView(sectionHeaders[i].PointerToRawData, sectionHeaders[i].SizeOfRawData);
		else
			section.data = source->getView(sectionHeaders[i].VirtualAddress, sectionHeaders[i].SizeOfRawData);

		sections_.push_back(std::move(section));
	}

	uint8_t *loadConfig = getDataPointerOfRVA(dataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress);
	if(loadConfig)
	{
		if(info_.architecture == ArchitectureWin32)
			info_.platformData = reinterpret_cast<IMAGE_LOAD_CONFIG_DIRECTORY32 *>(loadConfig)->SecurityCookie;
		else
			info_.platformData = reinterpret_cast<IMAGE_LOAD_CONFIG_DIRECTORY64 *>(loadConfig)->SecurityCookie;
	}
	else
		info_.platformData = 0;
	info_.platformData1 = dataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;

	return headerSize;
}

uint8_t *PEFormat::getDataPointerOfRVA(uint32_t rva)
{
	for(auto &section : sections_)
		if(rva >= section.baseAddress && rva < section.baseAddress + section.size)
			return section.data->get() + rva - section.baseAddress;
	return nullptr;
}

IMAGE_DATA_DIRECTORY *PEFormat::getDataDirectory(size_t index)
{
	uint8_t *headerBase = header_->get();
	IMAGE_DATA_DIRECTORY *dataDirectory = reinterpret_cast<IMAGE_DATA_DIRECTORY *>(headerBase + dataDirectoryBase_);
	return dataDirectory + index;
}

void PEFormat::processRelocation()
{
	IMAGE_DATA_DIRECTORY *relocationDirectory = getDataDirectory(IMAGE_DIRECTORY_ENTRY_BASERELOC);
	size_t relocationSize = relocationDirectory->Size;
	IMAGE_BASE_RELOCATION *info = reinterpret_cast<IMAGE_BASE_RELOCATION *>(getDataPointerOfRVA(relocationDirectory->VirtualAddress));

	if(!info)
		return;
	while(true)
	{
		if(reinterpret_cast<size_t>(info) >= reinterpret_cast<size_t>(info) + relocationSize || info->SizeOfBlock == 0)
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

void PEFormat::processImport()
{
	IMAGE_DATA_DIRECTORY *importDirectory = getDataDirectory(IMAGE_DIRECTORY_ENTRY_IMPORT);
	IMAGE_IMPORT_DESCRIPTOR *descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(getDataPointerOfRVA(importDirectory->VirtualAddress));

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
			function.ordinal = -1;
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
				function.nameHash = fnv1a(function.name.c_str(), function.name.length());
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

String PEFormat::checkExportForwarder(uint64_t address, size_t exportTableBase, size_t exportTableSize)
{
	if(address >= exportTableBase && address < exportTableBase + exportTableSize)
		for(auto &i : sections_)
			if(address >= i.baseAddress && address < i.baseAddress + i.size)
				return String(reinterpret_cast<const char *>(i.data->get() + static_cast<uint32_t>(address - i.baseAddress)));
	return String();
}

void PEFormat::processExport()
{
	IMAGE_DATA_DIRECTORY *exportDirectory = getDataDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT);
	IMAGE_EXPORT_DIRECTORY *directory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY *>(getDataPointerOfRVA(exportDirectory->VirtualAddress));

	size_t exportTableBase = exportDirectory->VirtualAddress;
	size_t exportTableSize = exportDirectory->Size;

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
		{
			entry.name.assign(reinterpret_cast<const char *>(getDataPointerOfRVA(addressOfNames[i])));
			entry.nameHash = fnv1a(entry.name.c_str(), entry.name.length());
		}

		entry.ordinal = ordinals[i];
		entry.address = addressOfFunctions[entry.ordinal];
		checker[entry.ordinal] = true;
		entry.ordinal += directory->Base;
		entry.forward = checkExportForwarder(entry.address, exportTableBase, exportTableSize);

		exports_.push_back(std::move(entry));
	}

	for(size_t i = 0; i < directory->NumberOfFunctions; i ++)
	{
		if(checker[i] == true)
			continue;
		//entry without names
		ExportFunction entry;
		entry.ordinal = i + directory->Base;
		entry.address = addressOfFunctions[i];
		entry.forward = checkExportForwarder(entry.address, exportTableBase, exportTableSize);

		exports_.push_back(std::move(entry));
	}
	delete [] checker;
}

void PEFormat::setFileName(const String &fileName)
{
	fileName_ = fileName;
}

void PEFormat::setFilePath(const String &filePath)
{
	filePath_ = filePath;
}

const String &PEFormat::getFileName() const
{
	return fileName_;
}

const String &PEFormat::getFilePath() const
{
	return filePath_;
}

const List<Import> &PEFormat::getImports()
{
	if(processedImport_ == false)
	{
		processedImport_ = true;
		processImport();
	}
	return imports_;
}

const List<ExportFunction> &PEFormat::getExports()
{
	if(processedExport_ == false)
	{
		processedExport_ = true;
		processExport();
	}
	return exports_;
}

const ImageInfo &PEFormat::getInfo() const
{
	return info_;
}

const List<uint64_t> &PEFormat::getRelocations()
{
	if(processedRelocation_ == false)
	{
		processedRelocation_ = true;
		processRelocation();
	}

	return relocations_;
}

const List<Section> &PEFormat::getSections() const
{
	return sections_;
}

Image PEFormat::toImage()
{
	getImports();
	getExports();
	getRelocations();

	Image image;
	image.fileName = getFileName();
	image.filePath = getFilePath();
	image.info = info_;
	image.imports = std::move(imports_);
	image.sections = std::move(sections_);
	image.relocations = std::move(relocations_);
	image.header = std::move(header_);
	image.exports.assign_move(exports_.begin(), exports_.end());

	return image;
}

void PEFormat::setSections(const List<Section> &sections)
{
	sections_ = sections;
}

void PEFormat::setRelocations(const List<uint64_t> &relocations)
{
	relocations_ = relocations;
}

void PEFormat::setImageInfo(const ImageInfo &info)
{
	info_ = info;
}

size_t PEFormat::estimateSize() const
{
	size_t size = 0x400;
	for(auto &i : sections_)
		size += multipleOf(i.data->size(), 0x200);
	return size;
}

void PEFormat::save(SharedPtr<DataSource> target)
{
	//1. Preparation
	List<IMAGE_SECTION_HEADER> sectionHeaders;
	Map<uint32_t, uint32_t> rawDataMap;

	const uint32_t sectionAlignment = 0x1000;
	const uint32_t fileAlignment = 0x200;
	uint32_t imageSize = 0;
	uint32_t dataOffset = 0x400;
	for(auto &i : sections_)
	{
		IMAGE_SECTION_HEADER sectionHeader;
		zeroMemory(&sectionHeader, sizeof(sectionHeader));
		copyMemory(sectionHeader.Name, &i.name[0], i.name.length() + 1);
		sectionHeader.VirtualAddress = static_cast<uint32_t>(i.baseAddress);
		sectionHeader.VirtualSize = static_cast<uint32_t>(i.size);
		sectionHeader.SizeOfRawData = multipleOf(i.data->size(), fileAlignment);
		if(sectionHeader.SizeOfRawData)
			sectionHeader.PointerToRawData = dataOffset;
		else
			sectionHeader.PointerToRawData = 0;
		sectionHeader.Characteristics = 0;
		if(i.flag & SectionFlagData)
			sectionHeader.Characteristics |= IMAGE_SCN_CNT_INITIALIZED_DATA;
		if(i.flag & SectionFlagUninitializedData)
			sectionHeader.Characteristics |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;
		if(i.flag & SectionFlagCode)
			sectionHeader.Characteristics |= IMAGE_SCN_CNT_CODE;
		if(i.flag & SectionFlagRead)
			sectionHeader.Characteristics |= IMAGE_SCN_MEM_READ;
		if(i.flag & SectionFlagWrite)
			sectionHeader.Characteristics |= IMAGE_SCN_MEM_WRITE;
		if(i.flag & SectionFlagExecute)
			sectionHeader.Characteristics |= IMAGE_SCN_MEM_EXECUTE;

		sectionHeaders.push_back(sectionHeader);
		rawDataMap.insert(sectionHeader.VirtualAddress, dataOffset);
		dataOffset += sectionHeader.SizeOfRawData;
		imageSize = multipleOf(sectionHeader.VirtualAddress + sectionHeader.VirtualSize, sectionAlignment);
	}

	//2. Write headers
	uint8_t *originalHeader = header_->get();
	uint8_t *targetMap = target->map(0);
	IMAGE_DOS_HEADER *dosHeader = getStructureAtOffset<IMAGE_DOS_HEADER>(originalHeader, 0);
	IMAGE_FILE_HEADER fileHeader;
	IMAGE_OPTIONAL_HEADER_BASE optionalHeaderBase;
	const uint32_t ntSignature = IMAGE_NT_SIGNATURE;

	copyMemory(&fileHeader, getStructureAtOffset<IMAGE_FILE_HEADER>(originalHeader, dosHeader->e_lfanew + sizeof(uint32_t)), sizeof(IMAGE_FILE_HEADER));
	copyMemory(&optionalHeaderBase, getStructureAtOffset<IMAGE_OPTIONAL_HEADER_BASE>(originalHeader, dosHeader->e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER)), sizeof(IMAGE_OPTIONAL_HEADER_BASE));

	fileHeader.NumberOfSections = sections_.size();

	size_t offset = 0;
	copyMemory(targetMap, originalHeader, dosHeader->e_lfanew); offset += dosHeader->e_lfanew;
	copyMemory(targetMap + offset, &ntSignature, sizeof(ntSignature)); offset += sizeof(ntSignature);
	copyMemory(targetMap + offset, &fileHeader, sizeof(IMAGE_FILE_HEADER)); offset += sizeof(IMAGE_FILE_HEADER);

	if(info_.architecture == ArchitectureWin32)
	{
		IMAGE_OPTIONAL_HEADER32 optionalHeader;
		copyMemory(&optionalHeader, getStructureAtOffset<IMAGE_OPTIONAL_HEADER32>(originalHeader, offset), sizeof(IMAGE_OPTIONAL_HEADER32));
		optionalHeader.SizeOfImage = imageSize;
		optionalHeader.FileAlignment = fileAlignment;
		optionalHeader.SectionAlignment = sectionAlignment;

		copyMemory(targetMap + offset, &optionalHeader, sizeof(IMAGE_OPTIONAL_HEADER32)); offset += sizeof(IMAGE_OPTIONAL_HEADER32);
	}
	else if(info_.architecture == ArchitectureWin32AMD64)
	{
		IMAGE_OPTIONAL_HEADER64 optionalHeader;
		copyMemory(&optionalHeader, getStructureAtOffset<IMAGE_OPTIONAL_HEADER64>(originalHeader, offset), sizeof(IMAGE_OPTIONAL_HEADER64));
		optionalHeader.SizeOfImage = sectionAlignment;
		optionalHeader.FileAlignment = fileAlignment;
		optionalHeader.SectionAlignment = sectionAlignment;

		copyMemory(targetMap + offset, &optionalHeader, sizeof(IMAGE_OPTIONAL_HEADER64)); offset += sizeof(IMAGE_OPTIONAL_HEADER64);
	}
	
	for(auto &i : sectionHeaders)
	{
		copyMemory(targetMap + offset, &i, sizeof(IMAGE_SECTION_HEADER)); 
		offset += sizeof(IMAGE_SECTION_HEADER);
	}

	for(auto &i : sections_)
	{
		dataOffset = rawDataMap[static_cast<uint32_t>(i.baseAddress)];
		copyMemory(targetMap + dataOffset, i.data->get(), i.data->size());
	}

	target->unmap();
}

bool PEFormat::isSystemLibrary(const String &filename)
{
	if(filename.substr(0, 4).icompare("api-") == 0)
		return true;
	const char *systemFiles[] = {"kernel32.dll", "user32.dll", "gdi32.dll", "ntdll.dll", "kernelbase.dll", "shell32.dll", nullptr};
	for(int i = 0; systemFiles[i] != nullptr; i ++)
		if(filename.icompare(systemFiles[i]) == 0)
			return true;
	return false;
}

SharedPtr<FormatBase> loadImport(const String &path, int architecture)
{
	String newPath = path;
#ifdef _WIN32
	if(Win32NativeHelper::get()->isWoW64())
	{
		String system32Directory = Win32NativeHelper::get()->getSystem32Directory();
		if(architecture == ArchitectureWin32 && path.substr(0, system32Directory.length()).icompare(system32Directory) == 0)
			newPath = Win32NativeHelper::get()->getSysWOW64Directory() + "\\" + path.substr(system32Directory.length() + 1);
	}
#endif
	if(!File::isPathExists(newPath))
		return SharedPtr<FormatBase>(nullptr);

	SharedPtr<File> file = File::open(newPath);
	SharedPtr<FormatBase> result = MakeShared<PEFormat>();
	result->load(file, false);
	if(architecture != -1 && result->getInfo().architecture != architecture)
		return SharedPtr<FormatBase>(nullptr);
	result->setFileName(file->getFileName());
	result->setFilePath(file->getFilePath());
	return result;
}

SharedPtr<FormatBase> FormatBase::loadImport(const String &filename, const String &hint, int architecture)
{
	if(File::isPathExists(filename))
		return ::loadImport(filename, architecture);

	List<String> searchPaths;
	if(hint.length())
		searchPaths.push_back(hint);
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
		if(equal >= 3 && 
			WString::to_lower(start[0]) == L'p' &&
			WString::to_lower(start[1]) == L'a' &&
			WString::to_lower(start[2]) == L't' &&
			WString::to_lower(start[3]) == L'h' &&
			start[4] == L'=')
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
		String currentPath = WStringToString(path.substr(s, e - s));
		searchPaths.push_back(currentPath);
		s = e + 1;
	}
#endif
	for(auto &i : searchPaths)
	{
		String path = File::combinePath(i, filename);
		SharedPtr<FormatBase> result = ::loadImport(path, architecture);
		if(!result)
		{
			if(path.substr(path.length() - 4).icompare(".dll") != 0)
			{
				path.append(".dll");
				result = ::loadImport(path, architecture);
			}
		}
		if(result.get())
			return result;
	}
	return SharedPtr<FormatBase>(nullptr);
}
