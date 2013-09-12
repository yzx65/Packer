#pragma once

#include <cstdint>
#include "SharedPtr.h"

class DataSource;
class DataView
{
private:
	SharedPtr<DataSource> source_;
	uint8_t *baseAddress_;
	size_t size_;
public:
	DataView(SharedPtr<DataSource> source, uint8_t *baseAddress, size_t size) : source_(source), baseAddress_(baseAddress), size_(size) {}
	virtual ~DataView() {}

	uint8_t *get() const
	{
		return baseAddress_;
	}

	size_t getSize() const
	{
		return size_;
	}
};

class DataSource
{
public:
	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size) = 0;
};

class MemoryDataSource : public DataSource, public EnableSharedFromThis<MemoryDataSource>
{
private:
	uint8_t *memory_;
public:
	MemoryDataSource(uint8_t *memory) : memory_(memory) {}
	virtual ~MemoryDataSource() {}

	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size)
	{
		return MakeShared<DataView>(sharedFromThis(), memory_ + offset, size);
	}
};