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
		Vector<CharacterType>::resize(length + 1);
		Vector<CharacterType>::assign(const_cast<CharacterType *>(string), length + 1);
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

	int rfind(CharacterType pattern, int start = -1)
	{
		if(start == -1)
			start = length();
		for(size_t i = start - 1; i >= 0; i --)
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

	int compare(const String &operand) const
	{
		return compare(operand.c_str());
	}

	static CharacterType to_lower(CharacterType x)
	{
		return (x >= CharacterType('A') && x <= CharacterType('Z') ? x - (CharacterType('A') - CharacterType('a')) : x);
	}

	int icompare(const CharacterType *operand) const
	{
		size_t i;
		for(i = 0; operand[i] != 0 && get()[i] != 0; i ++)
			if(to_lower(operand[i]) != to_lower(get()[i]))
				return to_lower(operand[i]) - to_lower(get()[i]);
		return to_lower(operand[i]) - to_lower(get()[i]);
	}

	int icompare(const String &operand) const
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