#include "PackerMain.h"
#include "Win32Runtime.h"
#include "Util.h"

void WindowsEntry()
{
	Win32NativeHelper::init();
	List<String> arguments = Win32NativeHelper::get()->getArgumentList();

	PackerMain(Option(arguments)).process();
}