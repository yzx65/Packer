#include "../Win32/Win32Runtime.h"
#include "../Win32/Win32Loader.h"
#include "../Runtime/PEFormat.h"

void Entry()
{
	Win32NativeHelper::get()->init();

	List<String> arguments = Win32NativeHelper::get()->getArgumentList();

	SharedPtr<File> file = File::open(*(++ arguments.begin()));
	SharedPtr<FormatBase> input = MakeShared<PEFormat>();
	input->load(file, false);
	input->setFileName(file->getFileName());
	input->setFilePath(file->getFilePath());

	Win32Loader loader(input->toImage(), List<Image>());
	loader.execute();
}