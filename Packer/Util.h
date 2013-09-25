#pragma once

#include "String.h"

inline String WStringToString(const WString &input)
{
	String result;
	if(sizeof(wchar_t) == 2)
	{
		//utf16
		for(size_t i = 0; i < input.length();)
		{
			if(input[i] < 0xD800 || input[i] > 0xDFFF)
			{ //no surrogate pair
				if(input[i] < 0x80) //1 byte
				{
					result.push_back((char)input[i]);
				}
				else if(input[i] < 0x800)
				{
					result.push_back((char)(0xC0 | ((input[i] & 0x7C0) >> 6)));
					result.push_back((char)(0x80 | (input[i] & 0x3F)));
				}
				else
				{
					result.push_back((char)(0xE0 | ((input[i] & 0xF000) >> 12)));
					result.push_back((char)(0x80 | ((input[i] & 0xFC0) >> 6)));
					result.push_back((char)(0x80 | (input[i] & 0x3F)));
				}
			}
			else
			{
				size_t temp;
				temp = (input[i] & 0x3FF << 10) | (input[i + 1] & 0x3FF);
				temp += 0x10000;
				if(temp < 0x80) //1 byte
				{
					result.push_back((char)temp);
				}
				else if(temp < 0x800)
				{
					result.push_back((char)(0xC0 | ((temp & 0x7C0) >> 6)));
					result.push_back((char)(0x80 | (temp & 0x3F)));
				}
				else if(temp < 0x10000)
				{
					result.push_back((char)(0xE0 | ((temp & 0xF000) >> 12)));
					result.push_back((char)(0x80 | ((temp & 0xFC0) >> 6)));
					result.push_back((char)(0x80 | (temp & 0x3F)));
				}
				else
				{
					result.push_back((char)(0xF0 | ((temp & 0x1C0000) >> 18)));
					result.push_back((char)(0x80 | ((temp & 0x3F000) >> 12)));
					result.push_back((char)(0x80 | ((temp & 0xFC0) >> 6)));
					result.push_back((char)(0x80 | (temp & 0x3F)));
				}
				i ++;
			}
			i ++;
		}
	}
	else if(sizeof(wchar_t) == 4)
	{
		//utf-32
		size_t i = 0;
		for(size_t i = 0; i < input.length();)
		{
			if(input[i] < 0x80) //1 byte
			{
				result.push_back((char)input[i]);
			}
			else if(input[i] < 0x800)
			{
				result.push_back((char)(0xC0 | ((input[i] & 0x7C0) >> 6)));
				result.push_back((char)(0x80 | (input[i] & 0x3F)));
			}
			else if(input[i] < 0x10000)
			{
				result.push_back((char)(0xE0 | ((input[i] & 0xF000) >> 12)));
				result.push_back((char)(0x80 | ((input[i] & 0xFC0) >> 6)));
				result.push_back((char)(0x80 | (input[i] & 0x3F)));
			}
			else
			{
				result.push_back((char)(0xF0 | ((input[i] & 0x1C0000) >> 18)));
				result.push_back((char)(0x80 | ((input[i] & 0x3F000) >> 12)));
				result.push_back((char)(0x80 | ((input[i] & 0xFC0) >> 6)));
				result.push_back((char)(0x80 | (input[i] & 0x3F)));
			}
			i ++;
		}
	}
	return result;
}

inline WString StringToWString(const String &input)
{
	WString result;
	if(sizeof(wchar_t) == 2)
	{
		for(size_t i = 0; i < input.length();)
		{
			if(!((input[i] & 0xF0) == 0xF0))
			{
				//1 utf-16 character
				if((input[i] & 0xE0) == 0xE0)
				{
					result.push_back(((input[i] & 0x0f) << 12) | (input[i + 1] & 0x3f) << 6 | (input[i + 2] & 0x3f));
					i += 2;
				}
				else if((input[i] & 0xC0) == 0xC0)
				{
					result.push_back(((input[i] & 0x1f) << 6) | (input[i + 1] & 0x3f));
					i ++;
				}
				else
				{
					result.push_back(input[i]);
				}
			}
			else
			{
				size_t temp = ((input[i] & 0x07) << 18) | ((input[i + 1] & 0x3f) << 12) | ((input[i + 2] & 0x3f) << 6) | (input[i + 3] & 0x3f);
				i += 3;
				temp -= 0x10000;
				result.push_back(0xD800 | (temp & 0xFFC00));
				result.push_back(0xDC00 | (temp & 0x3FF));
			}
			i ++;
		}
	}
	else if(sizeof(wchar_t) == 4)
	{
		for(size_t i = 0; i < input.length();)
		{
			if((input[i] & 0xF0) == 0xF0)
			{
				result.push_back(((input[i] & 0x07) << 18) | ((input[i + 1] & 0x3f) << 12) | ((input[i + 2] & 0x3f) << 6) | (input[i + 3] & 0x3f));
				i += 3;
			}
			else if((input[i] & 0xE0) == 0xE0)
			{
				result.push_back(((input[i] & 0x0f) << 12) | ((input[i + 1] & 0x3f) << 6) | (input[i + 2] & 0x3f));
				i += 2;
			}
			else if((input[i] & 0xC0) == 0xC0)
			{
				result.push_back(((input[i] & 0x1f) << 6) | (input[i + 1] & 0x3f));
				i ++;
			}
			else
			{
				result.push_back(input[i]);
			}
			i ++;
		}
	}

	return result;
}

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

template<typename DestinationType, typename SourceType>
inline void copyMemory(DestinationType *dest_, const SourceType *src_, size_t size)
{
	uint8_t *dest = reinterpret_cast<uint8_t *>(dest_);
	const uint8_t *src = reinterpret_cast<const uint8_t *>(src_);
	if(!size)
		return;
	size_t i;
	for(i = 0; i < reinterpret_cast<size_t>(src) % sizeof(size_t); i ++)
		*(dest + i) = *(src + i); //align to boundary
	if(i > sizeof(size_t))
		for(; i < size - sizeof(size_t); i += sizeof(size_t))
			*reinterpret_cast<size_t *>(dest + i) = *reinterpret_cast<const size_t *>(src + i);
	for(; i < size; i ++)
		*(dest + i) = *(src + i); //remaining
}

inline int StringToInt(const String &str)
{
	int value = 0;
	size_t i = 0;
	if(str[0] == '-')
		i ++;
	for(; i < str.length(); i ++)
	{
		if(str[i] <= '9' && str[i] >= '0')
		{
			value *= 10;
			value += str[i] - '0';
		}
		else
			break;
	}
	if(str[0] == '-')
		return -value;

	return value;
}