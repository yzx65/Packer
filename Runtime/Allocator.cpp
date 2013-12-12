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
struct Bucket
{
	Bucket *nextBucket;
	size_t capacity;
	uint8_t bucketData[1];
};
#pragma pack(pop)

#define BUCKET_COUNT 13
const uint16_t bucketSizes[BUCKET_COUNT] = {4, 16, 32, 64, 128, 512, 1024, 2048, 4096, 8192, 16384, 32768, 0};
Bucket *bucket[BUCKET_COUNT];
const size_t bucketCapacity[BUCKET_COUNT] = {0x1000, 0x1000, 0x1000, 0x1000, 0x10000, 0x10000, 0x10000, 0x100000, 0x100000, 0x100000, 0x100000, 0x100000, 0};
Bucket *lastBucket[BUCKET_COUNT];

uint8_t *allocateVirtual(size_t size)
{
	return reinterpret_cast<uint8_t *>(Win32NativeHelper::get()->allocateVirtual(0, multipleOf(size, 4096), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
}

bool freeVirtual(void *ptr)
{
	return Win32NativeHelper::get()->freeVirtual(ptr);
}

uint8_t *searchEmpty(Bucket *bucket, size_t size, size_t bucketSize)
{
	uint8_t *ptr = bucket->bucketData;
	while(true)
	{
		MemoryInfo *info = reinterpret_cast<MemoryInfo *>(ptr);
		if(!info->inUse && (bucketSize != 0 || (bucketSize == 0 && info->size >= size)))
		{
			info->inUse = 1;
			info->size = size;
			return ptr + sizeof(MemoryInfo);
		}
		ptr += sizeof(MemoryInfo) + bucketSize;
		if(ptr + sizeof(MemoryInfo) + bucketSize > reinterpret_cast<uint8_t *>(bucket) + bucket->capacity)
			return nullptr;
	}
}

void *heapAlloc(size_t size)
{
	size_t bucketNo;
	for(bucketNo = 0; bucketNo < BUCKET_COUNT; bucketNo ++)
		if(bucketSizes[bucketNo] >= size || bucketSizes[bucketNo] == 0)
			break;

	if(bucketSizes[bucketNo] == 0)
		return allocateVirtual(multipleOf(size, 4096));
	if(lastBucket[bucketNo])
	{
		uint8_t *result = searchEmpty(lastBucket[bucketNo], size, bucketSizes[bucketNo]);
		if(result)
			return result;
	}
	Bucket **currentBucket = &bucket[bucketNo];
	while(true)
	{
		if(!*currentBucket)
		{
			size_t newSize = multipleOf(bucketCapacity[bucketNo], 4096);
			*currentBucket = reinterpret_cast<Bucket *>(allocateVirtual(newSize));
			(*currentBucket)->capacity = newSize;
			lastBucket[bucketNo] = (*currentBucket);
		}

		uint8_t *result = searchEmpty(*currentBucket, size, bucketSizes[bucketNo]);
		if(result)
			return result;
		currentBucket = &((*currentBucket)->nextBucket);
	}
}

void heapFree(void *ptr)
{
	if((reinterpret_cast<size_t>(ptr) & 0xFFFF) == 0 && freeVirtual(ptr))
		return;

	reinterpret_cast<MemoryInfo *>(reinterpret_cast<uint8_t *>(ptr) - sizeof(MemoryInfo))->inUse = 0;
}