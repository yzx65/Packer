#pragma once

#define WIN32_STUB_STAGE2_MAGIC 0xf00df00d

struct Win32StubStage2Header
{
	uint32_t magic;
	uint32_t signature;
	uint32_t imageSize;
	uint32_t numberOfRelocations;
	uint32_t entryPoint;
	uint32_t originalBase;
};

inline uint32_t buildSignature(const uint8_t *data, size_t size)
{
	uint32_t check = 0;
	for(size_t i = 0; i < size / 4; i ++)
		check += reinterpret_cast<const uint32_t *>(data)[i];
	return check;
}

inline void simpleCrypt(uint32_t seed, uint8_t *data, size_t size)
{
	size_t i;
	for(i = 0; i < size - 4; i += 4)
	{
		reinterpret_cast<uint32_t *>(data)[i / 4] ^= seed;
		seed += reinterpret_cast<uint32_t *>(data)[i / 4];
		seed = (seed >> 16) | (seed << 16);
		seed ^= reinterpret_cast<uint32_t *>(data)[i / 4];
	}
	for(; i < size; i ++)
		data[i] ^= seed;
}

inline void simpleDecrypt(uint32_t seed, uint8_t *data, size_t size)
{
	size_t i = 0;
	size_t lastData = reinterpret_cast<uint32_t *>(data)[i / 4];

	reinterpret_cast<uint32_t *>(data)[i / 4] ^= seed;
	for(i = 4; i < size - 4; i += 4)
	{
		seed += lastData;
		seed = (seed >> 16) | (seed << 16);
		seed ^= lastData;
		lastData = reinterpret_cast<uint32_t *>(data)[i / 4];
		reinterpret_cast<uint32_t *>(data)[i / 4] ^= seed;
	}
	seed += lastData;
	seed = (seed >> 16) | (seed << 16);
	seed ^= lastData;
	for(; i < size; i ++)
		data[i] ^= seed;
}