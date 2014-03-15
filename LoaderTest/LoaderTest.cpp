#include "../Win32/Win32Runtime.h"
#include "../Win32/Win32Loader.h"
#include "../Runtime/PEFormat.h"

List<String> *loadedFiles_;
List<Image> loadImport(SharedPtr<FormatBase> input)
{
	List<Image> result;
	for(auto &i : input->getImports())
	{
		bool alreadyLoaded = false;
		String fileName = i.libraryName;
		for(auto &j : *loadedFiles_)
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
		loadedFiles_->push_back(import->getFileName());
		result.push_back(import->toImage());

		List<Image> dependencies = loadImport(import);
		result.insert(result.end(), dependencies.begin(), dependencies.end());
	}

	return result;
}

void Entry()
{
	Win32NativeHelper::get()->init();
	loadedFiles_ = new List<String>;

	List<String> arguments = Win32NativeHelper::get()->getArgumentList();

	SharedPtr<File> file = File::open(*(++ arguments.begin()));
	SharedPtr<FormatBase> input = MakeShared<PEFormat>();
	input->load(file, false);
	input->setFileName(file->getFileName());
	input->setFilePath(file->getFilePath());

	Win32Loader loader(input->toImage(), loadImport(input));
	loader.execute();
}