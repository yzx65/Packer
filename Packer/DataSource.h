#pragma once

#include <cstdint>
#include "SharedPtr.h"

class DataView;
class DataSource
{
public:
	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size) = 0;
	virtual uint8_t *map(uint64_t offset) = 0;
	virtual void unmap() = 0;
};

class DataView
{
private:
	SharedPtr<DataSource> source_;
	uint64_t offset_;
	uint8_t *baseAddress_;
	size_t size_;
public:
	DataView(SharedPtr<DataSource> source, uint64_t offset, size_t size) : source_(source), offset_(offset), size_(size), baseAddress_(0) {}
	virtual ~DataView() 
	{
		if(baseAddress_)
			source_->unmap();
	}

	void unmap()
	{
		if(baseAddress_)
			source_->unmap();
		baseAddress_ = 0;
	}

	uint8_t *map()
	{
		if(!baseAddress_)
			baseAddress_ = source_->map(offset_);
		return baseAddress_;
	}

	uint8_t *get()
	{
		return map();
	}

	size_t size() const
	{
		return size_;
	}
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
		return MakeShared<DataView>(sharedFromThis(), offset, size);
	}

	virtual uint8_t *map(uint64_t offset)
	{
		return memory_ + offset;
	}

	virtual void unmap()
	{

	}
};