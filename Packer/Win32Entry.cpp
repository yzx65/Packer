#include <Windows.h>

#include "PackerMain.h"

void WindowsEntry()
{
	LPWSTR cmdLine = GetCommandLine();
	int argc;
	LPWSTR *argv;

	argv = CommandLineToArgvW(cmdLine, &argc);
	PackerMain(Option(argc, argv)).process();
}