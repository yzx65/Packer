#include "../../Packer/PEFormat.h"
#include "../../Packer/Win32Runtime.h"
#include "../Win32Stub.h"

int Entry()
{
	Win32NativeHelper::get()->init(nullptr);

	uint8_t *mainBase = reinterpret_cast<uint8_t *>(WIN32_STUB_BASE_ADDRESS + WIN32_STUB_MAIN_SECTION_BASE);
	_asm int 3
	
	Win32NativeHelper::get()->unmapViewOfSection(reinterpret_cast<void *>(WIN32_STUB_BASE_ADDRESS));
	return 0;
}