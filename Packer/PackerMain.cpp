#include "PackerMain.h"

#include "Vector.h"
#include "FormatBase.h"
#include "PEFormat.h"
#include "Signature.h"
#include "../Win32Stub/Win32Stub.h"
#include "../Win32Stub/StubData.h"

PackerMain::PackerMain(const Option &option) : option_(option)
{
}

int PackerMain::process()
{
	processFile(option_.getInputFile(), option_.getOutputFile());

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
		result.push_back(import->toImage());

		List<Image> dependencies = loadImport(import);
		result.insert(result.end(), dependencies.begin(), dependencies.end());
	}

	return result;
}

void PackerMain::processFile(SharedPtr<File> inputf, SharedPtr<File> output)
{
	SharedPtr<FormatBase> input;
	{
		SharedPtr<DataView> view = inputf->getView(0, 0);
		uint8_t *fileData = view->get();
		if(*(reinterpret_cast<uint16_t *>(fileData)) == IMAGE_DOS_SIGNATURE)
			input = MakeShared<PEFormat>();
		else
			return;
	}
	input->load(inputf, false);
	input->setFileName(inputf->getFileName());
	input->setFilePath(inputf->getFilePath());
	loadedFiles_.push_back(input->getFileName());
	List<Image> imports = loadImport(input);
	
	outputPE(input->toImage(), imports, output);
}

void PackerMain::outputPE(Image &image, const List<Image> imports, SharedPtr<File> output)
{
	PEFormat resultFormat;
	Vector<uint8_t> stub(win32StubSize);
	decompress(win32StubData, stub.get());
	resultFormat.load(stub.asDataSource(), false);
	
	List<Section> resultSections(resultFormat.getSections());
	uint64_t lastAddress;
	for(auto &i : resultSections)
		lastAddress = i.baseAddress + i.size;

	Vector<uint8_t> mainData(image.serialize());

	Section mainSection;
	mainSection.baseAddress = multipleOf(static_cast<size_t>(lastAddress), 0x1000);
	mainSection.flag = SectionFlagData | SectionFlagRead;
	mainSection.name = WIN32_STUB_MAIN_SECTION_NAME;
	mainSection.data = mainData.getView(0);
	mainSection.size = multipleOf(mainData.size(), 0x100);
	resultSections.push_back(mainSection);

	lastAddress = mainSection.baseAddress + mainSection.size;

	Vector<uint8_t> impData;
	for(auto &i : imports)
		impData.append(i.serialize());

	Section importSection;
	importSection.baseAddress = multipleOf(static_cast<size_t>(lastAddress), 0x1000);
	importSection.flag = SectionFlagData | SectionFlagRead;
	importSection.name = WIN32_STUB_IMP_SECTION_NAME;
	importSection.data = impData.getView(0);
	importSection.size = multipleOf(impData.size(), 0x100);
	resultSections.push_back(importSection);

	resultFormat.setSections(resultSections);

	output->resize(resultFormat.estimateSize());
	resultFormat.save(output);
}