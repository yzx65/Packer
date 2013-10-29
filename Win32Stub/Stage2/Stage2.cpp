#include "../../Packer/PEFormat.h"
#include "../../Packer/Win32Runtime.h"
#include "../Win32Stub.h"

int Entry()
{
	Win32NativeHelper::get()->init(nullptr);
	Win32NativeHelper::get()->unmapViewOfSection(reinterpret_cast<void *>(WIN32_STUB_BASE_ADDRESS));


	return 0;
}