#include "Allocator.h"

#include "../Win32/Win32Runtime.h"
#include "../Util/Util.h"

#include <cstdint>

#pragma pack(push, 1)
struct MemoryInfo
{
	unsigned int inUse : 1;
#ifdef _WIN64
	unsigned int size : 63;
#else
	unsigned int size : 31;
#endif
};
#pragma pack(pop)

#define BUCKET_COUNT 13
const uint16_t bucketSizes[BUCKET_COUNT] = {4, 16, 32, 64, 128, 512, 1024, 2048, 4096, 8192, 16384, 32768, 0};
uint8_t *bucket[BUCKET_COUNT];
size_t bucketCapacity[BUCKET_COUNT] = {0, };

uint8_t *allocateVirtual(size_t size)
{
	return reinterpret_cast<uint8_t *>(Win32NativeHelper::get()->allocateVirtual(0, multipleOf(size, 4096), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
}

void freeVirtual(void *ptr)
{
	Win32NativeHelper::get()->freeVirtual(ptr);
}

void *heapAlloc(size_t size)
{
	size_t bucketNo;
	for(bucketNo = 0; bucketNo < BUCKET_COUNT; bucketNo ++)
		if(bucketSizes[bucketNo] >= size || bucketSizes[bucketNo] == 0)
			break;

	if(!bucket[bucketNo])
	{
		bucketCapacity[bucketNo] = 0x00100000;
		size_t newSize = multipleOf(max(bucketCapacity[bucketNo], size + sizeof(MemoryInfo) + sizeof(uint8_t *) + sizeof(uint32_t)), 4096);
		bucket[bucketNo] = allocateVirtual(newSize);
		*reinterpret_cast<uint32_t *>(bucket[bucketNo] + sizeof(uint8_t *)) = newSize;
	}

	uint8_t *base = bucket[bucketNo];
	uint8_t *ptr = base + sizeof(uint8_t *) + sizeof(size_t);
	size_t capacity = *reinterpret_cast<uint32_t *>(base + sizeof(uint8_t *));
	while(true)
	{
		MemoryInfo *info = reinterpret_cast<MemoryInfo *>(ptr);
		if(!info->inUse && (bucketCapacity[bucketNo] != 0 || (bucketCapacity[bucketNo] == 0 && info->size >= size)))
		{
			info->inUse = 1;
			info->size = size;
			return ptr + sizeof(MemoryInfo);
		}
		ptr += sizeof(MemoryInfo) + bucketSizes[bucketNo];
		if(ptr + sizeof(MemoryInfo) + bucketSizes[bucketNo] > base + capacity)
		{
			if(!*reinterpret_cast<uint8_t **>(base))
			{
				size_t newSize = multipleOf(max(bucketCapacity[bucketNo], size + sizeof(MemoryInfo) + sizeof(uint8_t *) + sizeof(uint32_t)), 4096);
				*reinterpret_cast<uint8_t **>(base) = allocateVirtual(newSize);
				*reinterpret_cast<uint32_t *>(base + sizeof(uint8_t *)) = newSize;
				bucketCapacity[bucketNo] *= 2;
				if(bucketCapacity[bucketNo] > 0x01000000)
					bucketCapacity[bucketNo] = 0x01000000;
			}
			base = *reinterpret_cast<uint8_t **>(base);
			ptr = base + sizeof(uint8_t *) + sizeof(size_t);
		}
	}
}

void heapFree(void *ptr)
{
	reinterpret_cast<MemoryInfo *>(reinterpret_cast<uint8_t *>(ptr) - sizeof(MemoryInfo))->inUse = 0;
}