#include "PackerMain.h"

#include "Vector.h"
#include "FormatBase.h"
#include "PEFormat.h"
#include "Signature.h"
#include "Win32Loader.h"

PackerMain::PackerMain(const Option &option) : option_(option)
{
}

int PackerMain::process()
{
	for(SharedPtr<File> &file : option_.getInputFiles())
		processFile(file);

	return 0;
}

List<SharedPtr<FormatBase>> PackerMain::loadImport(SharedPtr<FormatBase> input)
{
	List<SharedPtr<FormatBase>> result;
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
		result.push_back(import);

		List<SharedPtr<FormatBase>> dependencies = loadImport(import);
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
	List<SharedPtr<FormatBase>> imports = loadImport(input);
	
	//test
	Image image = input->serialize();
	Vector<Image> importImages;
	importImages.reserve(imports.size());
	for(auto &i : imports)
		importImages.push_back(i->serialize());
	Win32Loader loader(image, std::move(importImages));
	loader.execute();
}
