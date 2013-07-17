#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <list>

#include "DataStorage.h"
#include "Executable.h"

template<typename T, typename ContainerType>
void containerToDataStorage(DataStorage<T> &dest, const ContainerType &src)
{
	size_t cnt;
	dest.resize(src.size());
	cnt = 0;
	for(auto &i : src)
		dest.get()[cnt ++] = i;
}

template<typename T, typename ContainerType>
void containerToDataStorage(DataStorage<T> &dest, const ContainerType &&src)
{
	size_t cnt;
	dest.resize(src.size());
	cnt = 0;
	for(auto &i : src)
		dest.get()[cnt ++] = std::move(i);
}

template<typename T>
void containerToDataStorage(DataStorage<T> &dest, const std::string &&src)
{
	size_t cnt;
	dest.resize(src.size() + 1);
	cnt = 0;
	for(auto &i : src)
		dest.get()[cnt ++] = i;
	dest.get()[cnt ++] = '\0';
}

template<typename T>
void containerToDataStorage(DataStorage<T> &dest, const std::string &src)
{
	size_t cnt;
	dest.resize(src.size() + 1);
	cnt = 0;
	for(auto &i : src)
		dest.get()[cnt ++] = i;
	dest.get()[cnt ++] = '\0';
}

class FormatBase
{
public:
	FormatBase() {}
	virtual ~FormatBase() {}

	virtual Executable serialize() = 0;
	virtual std::string getFilename() = 0;
	virtual std::shared_ptr<FormatBase> loadImport(const std::string &filename) = 0;
	virtual std::list<Import> getImports() = 0;

	virtual bool isSystemLibrary(const std::string &filename) = 0;
};