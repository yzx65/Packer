#include "PackerMain.h"

#include <exception>

#include "FormatBase.h"
#include "PEFormat.h"
#include "Signature.h"
#include "Win32Loader.h"

struct ImportLibrary
{
public:
	std::shared_ptr<FormatBase> library;
	std::list<ImportLibrary> dependencies;
};

PackerMain::PackerMain(const Option &option) : option_(option)
{
}

int PackerMain::process()
{
	for(std::shared_ptr<File> &file : option_.getInputFiles())
	{
		try
		{
			processFile(file);
		}
		catch(...)
		{

		}
	}

	return 0;
}

std::list<ImportLibrary> PackerMain::loadImport(std::shared_ptr<FormatBase> input)
{
	std::list<ImportLibrary> result;
	for(auto &i : input->getImports())
	{
		bool alreadyLoaded = false;
		const char *libraryName = i.libraryName.get();
		for(auto &j : loadedFiles_)
			if(j->getFilename().compare(libraryName) == 0)
			{
				alreadyLoaded = true;
				break;
			}
		if(alreadyLoaded)
			continue;

		std::shared_ptr<FormatBase> libraryFile = input->loadImport(std::string(libraryName));
		if(libraryFile->isSystemLibrary())
			continue;
		loadedFiles_.push_back(libraryFile);
		ImportLibrary library;
		library.library = libraryFile;
		library.dependencies = loadImport(libraryFile);

		result.push_back(library);
	}

	return result;
}

void PackerMain::processFile(std::shared_ptr<File> file)
{
	std::shared_ptr<FormatBase> input;
	uint16_t magic;
	file->read(&magic);
	file->seek(0);
	if(magic == IMAGE_DOS_SIGNATURE)
		input = std::make_shared<PEFormat>(file);
	else
		throw std::exception();

	loadedFiles_.push_back(input);

	//test
	Executable executable = input->serialize();
	Win32Loader loader(executable);
	loader.load();

	std::list<ImportLibrary> imports = loadImport(input);
}

#if !defined(_UNICODE) || !defined(_WIN32)
int main(int argc, char **argv)
{
	return PackerMain(Option(argc, argv)).process();
}
#else
int wmain(int argc, wchar_t **argv)
{
	return PackerMain(Option(argc, argv)).process();
}
#endif
