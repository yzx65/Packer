#pragma once

#include "Vector.h"

#include <cstdint>

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
		size_t length = length_(string);
		resize(length + 1);
		assign(const_cast<CharacterType *>(string), length + 1);
	}

	StringBase(StringBase &&operand) : Vector<CharacterType>(std::move(operand)) {}

	template<typename IteratorType>
	StringBase(IteratorType start, IteratorType end)
	{
		size_t length = end - start + 1;
		resize(length + 1);
		assign(&*start, length);
		get()[length] = 0;
	}

	void push_back(CharacterType item)
	{
		if(size())
			Vector<CharacterType>::insert(length(), item);
		else
		{
			Vector<CharacterType>::reserve(2);
			Vector<CharacterType>::push_back(item);
			Vector<CharacterType>::push_back(0);
		}
	}

	CharacterType at(size_t pos) const
	{
		return get()[pos];
	}

	CharacterType operator [](size_t pos) const
	{
		return get()[pos];
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

	StringBase substr(size_t start, int len = -1)
	{
		if(len == -1)
			len = length();
		return StringBase(begin() + start, begin() + start + len);
	}

	const CharacterType *c_str() const
	{
		return const_cast<const CharacterType *>(get());
	}

	int find(CharacterType pattern, size_t start = 0)
	{
		for(size_t i = start; i < length(); i ++)
			if(get()[i] == pattern)
				return i;
		return -1;
	}

	int compare(const CharacterType *operand) const
	{
		size_t i;
		for(i = 0; operand[i] != 0 && get()[i] != 0; i ++)
			if(operand[i] != get()[i])
				return operand[i] - get()[i];
		return operand[i] - get()[i];
	}

	bool operator ==(const StringBase &operand) const
	{
		return compare(operand.c_str()) == 0;
	}

	bool operator <(const StringBase &operand) const
	{
		return compare(operand.c_str()) < 0;
	}

	bool operator !=(const StringBase &operand) const
	{
		return compare(operand.c_str()) != 0;
	}
};

typedef StringBase<char> String;
typedef StringBase<wchar_t> WString;
