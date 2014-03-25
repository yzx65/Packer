#pragma once

#include <cstdint>

template<typename IteratorType, typename Comparator>
typename IteratorType binarySearch(IteratorType begin, IteratorType end, Comparator comparator)
{
	int s = 0;
	int e = end - begin;

	while(s <= e)
	{
		int m = (s + e) / 2;
		int cmp = comparator(begin + m);
		if(cmp > 0)
			e = m - 1;
		else if(cmp < 0)
			s = m + 1;
		else
			return begin + m;
	}
	return end;
}

template<int t>
size_t makePattern(uint8_t val);

template<>
static size_t makePattern<4>(uint8_t val)
{
	return (val << 24) | (val << 16) | (val << 8) | val;
}

template<>
static size_t makePattern<8>(uint8_t val)
{
	return ((uint64_t)val << 58) | ((uint64_t)val << 52) | ((uint64_t)val << 48) | ((uint64_t)val << 40) | ((uint64_t)val << 32) | (val << 24) | (val << 16) | (val << 8) | val;;
}

template<typename DestinationType>
static void setMemory(DestinationType *dest_, uint8_t val, size_t size)
{
	uint8_t *dest = reinterpret_cast<uint8_t *>(dest_);
	if(!size)
		return;
	size_t i;
	
	for(i = 0; i < reinterpret_cast<size_t>(dest) % sizeof(size_t); i ++)
		*(dest + i) = val; //align to boundary
	if(size > sizeof(size_t))
	{
		size_t v = makePattern<sizeof(v)>(val);
		for(; i <= size - sizeof(size_t); i += sizeof(size_t))
			*reinterpret_cast<size_t *>(dest + i) = v;
	}
	for(; i < size; i ++)
		*(dest + i) = val; //remaining
}

template<typename DestinationType, typename SourceType>
static void copyMemory(DestinationType *dest_, const SourceType *src_, size_t size)
{
	uint8_t *dest = reinterpret_cast<uint8_t *>(dest_);
	const uint8_t *src = reinterpret_cast<const uint8_t *>(src_);
	if(!size)
		return;
	size_t i;
	for(i = 0; i < reinterpret_cast<size_t>(src) % sizeof(size_t); i ++)
		*(dest + i) = *(src + i); //align to boundary
	if(size > sizeof(size_t))
		for(; i <= size - sizeof(size_t); i += sizeof(size_t))
			*reinterpret_cast<size_t *>(dest + i) = *reinterpret_cast<const size_t *>(src + i);
	for(; i < size; i ++)
		*(dest + i) = *(src + i); //remaining
}

template<typename DestinationType, typename SourceType>
static void moveMemory(DestinationType *dest_, const SourceType *src_, size_t size)
{
	uint8_t *dest = reinterpret_cast<uint8_t *>(dest_);
	const uint8_t *src = reinterpret_cast<const uint8_t *>(src_);

	if(!size)
		return;

	if(dest <= src || dest >= src + size) //non-overlapping
		return copyMemory(dest_, src_, size);
	 
	//overlapping, copy from higher address
	dest = dest + size - 1;
	src = src + size - 1;

	while(size--)
	{
		*dest = *src;
		dest --;
		src --;
	}
}

template<typename DestinationType>
static void zeroMemory(DestinationType *dest_, size_t size)
{
	setMemory(dest_, 0, size);
}

static size_t multipleOf(size_t value, size_t n)
{
	return ((value + n - 1) / n) * n;
}

static const uint8_t *decodeVarInt(const uint8_t *ptr, uint8_t *flag, uint32_t *size)
{
	//f1xxxxxx
	//f01xxxxx xxxxxxxx
	//f001xxxx xxxxxxxx xxxxxxxx
	//f0001xxx xxxxxxxx xxxxxxxx xxxxxxxx
	//f0000100 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
	*size = *ptr ++;
	*flag = (*size & 0x80) >> 7;
	if(*size & 0x40)
		*size &= 0x3f;
	else if(*size & 0x20)
	{
		*size &= 0x1f;
		*size <<= 8;
		*size |= *(ptr ++);
	}
	else if(*size & 0x10)
	{
		*size &= 0x0f;
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
	}
	else if(*size & 0x08)
	{
		*size &= 0x07;
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
	}
	else if(*size & 0x04)
	{
		*size = *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
		*size <<= 8;
		*size |= *(ptr ++);
	}

	return ptr;
}

static uint32_t simpleRLEDecompress(const uint8_t *compressedData, uint8_t *decompressedData)
{
	uint32_t controlSize = *reinterpret_cast<const uint32_t *>(compressedData);
	const uint8_t *data = compressedData + controlSize + 4;
	const uint8_t *control = compressedData + 4;
	const uint8_t *controlPtr = control;
	uint32_t resultSize = 0;

	uint8_t flag;
	size_t size;
	while(controlPtr < control + controlSize)
	{
		controlPtr = decodeVarInt(controlPtr, &flag, &size);
		resultSize += size;
		if(flag)
		{
			//succession
			for(; size > 0; size --)
				*decompressedData ++ = *data;
			data ++;
		}
		else
		{
			//nonsuccession
			for(; size > 0; size --)
				*decompressedData ++ = *data ++;
		}
	}

	return resultSize;
}

static uint32_t fnv1a(const uint8_t *data, size_t size)
{
	uint32_t hash = 0x811c9dc5;
	for(size_t i = 0; i < size; i ++)
	{
		hash ^= data[i];
		hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
	}
	return hash;
}

template<typename T>
uint32_t fnv1a(const T *data, size_t size)
{
	return fnv1a(reinterpret_cast<const uint8_t *>(data), size);
}

#define max(a, b) ((a) < (b) ? (b) : (a))