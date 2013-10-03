#include "PackerMain.h"

#include "Vector.h"
#include "FormatBase.h"
#include "PEFormat.h"
#include "Signature.h"
#include "../Win32Stub/Win32Stub.h"

PackerMain::PackerMain(const Option &option) : option_(option)
{
}

int PackerMain::process()
{
	for(SharedPtr<File> &file : option_.getInputFiles())
		processFile(file);

	return 0;
}

List<Image> PackerMain::loadImport(SharedPtr<FormatBase> input)
{
	List<Image> result;
	for(auto &i : input->getImports())
	{
		bool alreadyLoaded = false;
		String fileName = i.libraryName;
		for(auto &j : loadedFiles_)
			if(j == fileName)
			{
				alreadyLoaded = true;
				break;
			}
		if(alreadyLoaded)
			continue;

		if(input->isSystemLibrary(fileName))
			continue;
		SharedPtr<FormatBase> import = FormatBase::loadImport(fileName, input->getFilePath());
		loadedFiles_.push_back(import->getFileName());
		result.push_back(import->serialize());

		List<Image> dependencies = loadImport(import);
		result.insert(result.end(), dependencies.begin(), dependencies.end());
	}

	return result;
}

void PackerMain::processFile(SharedPtr<File> file)
{
	SharedPtr<FormatBase> input;
	{
		SharedPtr<DataView> view = file->getView(0, 0);
		uint8_t *fileData = view->get();
		if(*(reinterpret_cast<uint16_t *>(fileData)) == IMAGE_DOS_SIGNATURE)
			input = MakeShared<PEFormat>();
		else
			return;
	}
	input->load(file, false);
	input->setFileName(file->getFileName());
	input->setFilePath(file->getFilePath());
	loadedFiles_.push_back(input->getFileName());
	List<Image> imports = loadImport(input);
	
	outputPE(input->serialize(), imports);
}

void PackerMain::outputPE(const Image &image, const List<Image> imports)
{
	Image outputImage;
	outputImage.header = image.header;
	outputImage.info = image.info;
	outputImage.nameExportLen = 0;

	Section mainSection;
	mainSection.baseAddress = WIN32_STUB_MAIN_SECTION_BASE;
	mainSection.flag = SectionFlagData | SectionFlagRead;
	mainSection.name = WIN32_STUB_MAIN_SECTION_NAME;
	outputImage.sections.push_back(mainSection);

	Section importSection;
	importSection.baseAddress = 0x00200000;
	importSection.flag = SectionFlagData | SectionFlagRead;
	importSection.name = WIN32_STUB_IMP_SECTION_NAME;
	outputImage.sections.push_back(importSection);
}