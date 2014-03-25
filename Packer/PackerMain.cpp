#include "PackerMain.h"

#include "../Runtime/FormatBase.h"
#include "../Runtime/PEFormat.h"
#include "../Win32/Stub/Win32Stub.h"
#include "../Win32/Stub/StubData.h"
#include "../Util/Vector.h"
#include "../Runtime/Signature.h"
#include "../Win32/Win32Runtime.h"

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
		SharedPtr<FormatBase> import = FormatBase::loadImport(fileName, input->getFilePath(), input->getInfo().architecture);
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
	simpleRLEDecompress(win32StubData, stub.get());
	resultFormat.load(stub.asDataSource(), false);
	
	List<Section> resultSections(resultFormat.getSections());
	uint64_t lastAddress;
	for(auto &i : resultSections)
		lastAddress = i.baseAddress + i.size;

	Vector<uint8_t> mainData(image.serialize());
	uint32_t seed = Win32NativeHelper::get()->getRandomValue();
	simpleCrypt(seed, &mainData[0], mainData.size());

	Section mainSection;
	mainSection.baseAddress = multipleOf(static_cast<size_t>(lastAddress), 0x1000);
	mainSection.flag = SectionFlagData | SectionFlagRead;
	mainSection.name.assign(reinterpret_cast<uint8_t *>(&seed), reinterpret_cast<uint8_t *>(&seed) + 4);
	mainSection.data = mainData.getView(0, mainData.size());
	mainSection.size = mainData.size();
	resultSections.push_back(mainSection);

	lastAddress = mainSection.baseAddress + mainSection.size;

	Vector<uint8_t> impData;
	uint32_t impCount = imports.size();
	impData.append(reinterpret_cast<uint8_t *>(&impCount), sizeof(impCount));
	for(auto &i : imports)
		impData.append(i.serialize());
	seed = Win32NativeHelper::get()->getRandomValue();
	simpleCrypt(seed, &impData[0], impData.size());

	Section importSection;
	importSection.baseAddress = multipleOf(static_cast<size_t>(lastAddress), 0x1000);
	importSection.flag = SectionFlagData | SectionFlagRead;
	importSection.name.assign(reinterpret_cast<uint8_t *>(&seed), reinterpret_cast<uint8_t *>(&seed) + 4);
	importSection.data = impData.getView(0, impData.size());
	importSection.size = impData.size();
	resultSections.push_back(importSection);

	resultFormat.setSections(resultSections);

	output->resize(resultFormat.estimateSize());
	resultFormat.save(output);
}