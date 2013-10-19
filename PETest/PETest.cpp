#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main()
{
	wchar_t test[] = L"D:\\work\\packer\\Win32Stub\\StubData - Copy.h.exe";
	STARTUPINFO si = {0, };
	PROCESS_INFORMATION pi = {0, };
	si.cb = sizeof(STARTUPINFO);
	CreateProcess(test, test, 0, 0, 0, 0, 0, 0, &si, &pi);
	return 0;
}
