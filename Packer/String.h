#pragma once

#include <cstdint>

#include "Runtime.h"
#include "Vector.h"

template<typename CharacterType=char>
class StringBase : private Vector<CharacterType>
{
private:
	static size_t length_(const CharacterType *str)
	{
		size_t result = 0;
		while(*str ++)
			result ++;
		return result;
	}
public:
	typedef typename Vector<CharacterType>::iterator iterator;
	typedef typename Vector<CharacterType>::const_iterator const_iterator;
	typedef CharacterType value_type;

	StringBase()
	{

	}

	StringBase(const CharacterType *string)
	{
		assign(string);
	}

	StringBase(StringBase &&operand) : Vector<CharacterType>(std::move(operand)) {}
	StringBase(const StringBase &operand) : Vector<CharacterType>(operand) {}

	template<typename IteratorType>
	StringBase(IteratorType start, IteratorType end)
	{
		assign(start, end);
	}

	template<typename IteratorType>
	void assign(IteratorType start, IteratorType end)
	{
		size_t length = end - start;
		Vector<CharacterType>::resize(length + 1);

		size_t i = 0;
		for(IteratorType it = start; it != end; it ++, i ++)
			get()[i] = *it;
		get()[length] = 0;
	}

	void assign(const CharacterType *string)
	{
		size_t length = length_(string);
		if(length)
		{
			Vector<CharacterType>::resize(length + 1);
			Vector<CharacterType>::assign(const_cast<CharacterType *>(string), length + 1);
		}
	}

	void push_back(CharacterType item)
	{
		if(size())
			Vector<CharacterType>::insert(length(), item);
		else
		{
			Vector<CharacterType>::push_back(item);
			Vector<CharacterType>::push_back(0);
		}
	}

	void append(const StringBase &operand)
	{
		if(size())
		{
			Vector<CharacterType>::insert(length(), operand);
			Vector<CharacterType>::resize(size() - 1);
		}
		else
			assign(operand.begin(), operand.end());
	}

	CharacterType at(size_t pos) const
	{
		return get()[pos];
	}

	CharacterType &operator [](size_t pos) const
	{
		return get()[pos];
	}

	void resize(size_t size)
	{
		Vector<CharacterType>::resize(size + 1);
		get()[size] = 0;
	}

	size_t length() const
	{
		size_t size = Vector<CharacterType>::size();
		return (size != 0 ? size - 1 : 0);
	}

	iterator begin()
	{
		return Vector<CharacterType>::begin();
	}

	iterator end()
	{
		return Vector<CharacterType>::end();
	}

	const_iterator begin() const
	{
		return Vector<CharacterType>::begin();
	}

	const_iterator end() const
	{
		return Vector<CharacterType>::end();
	}

	StringBase substr(size_t start, int len = -1) const
	{
		if(len == -1)
			len = length() - start;
		return StringBase(begin() + start, begin() + start + len);
	}

	const CharacterType *c_str() const
	{
		return const_cast<const CharacterType *>(get());
	}

	int find(CharacterType pattern, size_t start = 0) const
	{
		for(size_t i = start; i < length(); i ++)
			if(get()[i] == pattern)
				return i;
		return -1;
	}

	int rfind(CharacterType pattern, int start = -1) const
	{
		if(start == -1)
			start = length();
		for(int i = start - 1; i >= 0; i --)
			if(get()[i] == pattern)
				return i;
		return -1;
	}

	int compare(const CharacterType *operand) const
	{
		if(!get())
			return 1;
		size_t i;
		for(i = 0; operand[i] != 0 && get()[i] != 0; i ++)
			if(operand[i] != get()[i])
				return get()[i] - operand[i];
		return get()[i] - operand[i];
	}

	int compare(const StringBase &operand) const
	{
		return compare(operand.c_str());
	}

	static CharacterType to_lower(CharacterType x)
	{
		return (x >= CharacterType('A') && x <= CharacterType('Z') ? x - (CharacterType('A') - CharacterType('a')) : x);
	}

	int icompare(const CharacterType *operand) const
	{
		if(!get())
			return 1;
		size_t i;
		for(i = 0; operand[i] != 0 && get()[i] != 0; i ++)
			if(to_lower(operand[i]) != to_lower(get()[i]))
				return to_lower(get()[i]) - to_lower(operand[i]);
		return to_lower(get()[i]) - to_lower(operand[i]);
	}

	int icompare(const StringBase &operand) const
	{
		return icompare(operand.c_str());
	}

	bool operator ==(const StringBase &operand) const
	{
		return compare(operand.c_str()) == 0;
	}

	bool operator ==(const char *operand) const
	{
		return compare(operand) == 0;
	}

	const StringBase &operator =(const StringBase &operand)
	{
		Vector<CharacterType>::operator =(operand);
		return *this;
	}

	const StringBase &operator =(StringBase &&operand)
	{
		Vector<CharacterType>::operator =(std::move(operand));
		return *this;
	}

	bool operator <(const StringBase &operand) const
	{
		return compare(operand.c_str()) < 0;
	}

	bool operator !=(const StringBase &operand) const
	{
		return compare(operand.c_str()) != 0;
	}

	StringBase operator +(CharacterType operand) const
	{
		String result(*this);
		result.push_back(operand);
		return result;
	}

	StringBase operator +(const StringBase &operand) const
	{
		StringBase result(*this);
		result.append(operand);
		return result;
	}
};

typedef StringBase<char> String;
typedef StringBase<wchar_t> WString;

template<typename ValueType>
class CaseInsensitiveStringComparator
{
public:
	bool operator ()(const ValueType &a, const ValueType &b)
	{
		return a.icompare(b) < 0;
	}
};

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
