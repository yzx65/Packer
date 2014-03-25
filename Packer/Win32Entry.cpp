#include "PackerMain.h"
#include "../Win32/Win32NativeHelper.h"
#include "../Util/Util.h"

void WindowsEntry()
{
	Win32NativeHelper::get()->init();

	List<String> arguments = Win32NativeHelper::get()->getArgumentList();

	PackerMain(Option(arguments)).process();
}