#pragma once

#define WIN32_STUB_BASE_ADDRESS 0x00400000
#define WIN32_STUB_MAIN_SECTION_BASE 0x00100000
#define WIN32_STUB_IMP_SECTION_BASE 0x00200000
#define WIN32_STUB_STAGE2_BASE_ADDRESS 0x50000000

#define WIN32_STUB_MAIN_SECTION_NAME ".main"
#define WIN32_STUB_IMP_SECTION_NAME ".imp"
#define WIN32_STUB_STAGE2_SECTION_NAME ".stage2"

#define WIN32_STUB_STAGE2_MAGIC 0xf00df00d

struct Win32StubStage2Header
{
	size_t magic;
	size_t signature;
	size_t imageSize;
	size_t numberOfRelocations;
	size_t entryPoint;
	size_t originalBase;
};

inline size_t buildSignature(const uint8_t *data, size_t size)
{
	size_t check = 0;
	for(size_t i = 0; i < size / 4; i ++)
		check += reinterpret_cast<const uint32_t *>(data)[i];
	return check;
}