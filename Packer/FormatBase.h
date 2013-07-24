#pragma once

#include <cstdint>
#include <utility>
#include <memory>

#include "Vector.h"
#include "List.h"
#include "String.h"
#include "Image.h"

template<class ContainerType>
Vector<typename ContainerType::value_type> containerToDataStorage(const ContainerType &src)
{
	Vector<typename ContainerType::value_type> result;
	size_t cnt;
	result.resize(src.size());
	cnt = 0;
	for(auto &i : src)
		result.get()[cnt ++] = i;
	return result;
}

template<class ContainerType>
Vector<typename ContainerType::value_type> containerToDataStorage(ContainerType &&src)
{
	Vector<typename ContainerType::value_type> result;
	size_t cnt;
	result.resize(src.size());
	cnt = 0;
	for(auto &i : src)
		result.get()[cnt ++] = std::move(i);
	return result;
}

inline Vector<char> containerToDataStorage(String &&src)
{
	Vector<char> result;
	size_t cnt;
	result.resize(src.length() + 1);
	cnt = 0;
	for(auto &i : src)
		result.get()[cnt ++] = i;
	result.get()[cnt ++] = '\0';
	return result;
}

inline Vector<char> containerToDataStorage(const String &src)
{
	Vector<char> result;
	size_t cnt;
	result.resize(src.length() + 1);
	cnt = 0;
	for(auto &i : src)
		result.get()[cnt ++] = i;
	result.get()[cnt ++] = '\0';
	return result;
}

class FormatBase
{
public:
	FormatBase() {}
	virtual ~FormatBase() {}

	virtual Image serialize() = 0;
	virtual String getFilename() = 0;
	virtual std::shared_ptr<FormatBase> loadImport(const String &filename) = 0;
	virtual List<Import> getImports() = 0;

	virtual bool isSystemLibrary(const String &filename) = 0;
};