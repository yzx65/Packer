#include <windows.h>

#include <iostream>

int main()
{
	std::cout << "Hello, World";
	MessageBox(NULL, L"test", L"test", MB_OK);
	return 0;
}
