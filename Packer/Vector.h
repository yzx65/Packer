#pragma once

#include <cstdint>
#include <type_traits> //for is_pod
#include "Runtime.h"
#include "Util.h"

#include "DataSource.h"

template<typename ValueType>
class Vector : public DataSource, public EnableSharedFromThis<Vector<ValueType>>
{
private:
	size_t alloc_;
	size_t size_;
	ValueType *data_;

	template<typename T = ValueType>
	typename std::enable_if<std::is_pod<T>::value>::type
		_copyData(ValueType *oldData)
	{
		copyMemory(data_, oldData, sizeof(value_type) * size_);
	}

	template<typename T = ValueType>
	typename std::enable_if<!std::is_pod<T>::value>::type
		_copyData(ValueType *oldData)
	{
		for(size_t i = 0; i < size_; i ++)
			data_[i] = std::move(oldData[i]);
	}

	void resize_(uint32_t size, bool preserve)
	{
		if(alloc_ <= size)
		{
			alloc_ *= 2;
			if(alloc_ < size)
				alloc_ = size + 10;
			ValueType *oldData = data_;
			data_ = new ValueType[alloc_];
			if(oldData)
			{
				if(preserve)
					_copyData(oldData);
				delete [] oldData;
			}
		}
		size_ = size;
	}
public:
	typedef ValueType value_type;
	typedef ValueType *iterator;
	typedef ValueType *const const_iterator;

	Vector() : size_(0), alloc_(0), data_(nullptr) {}
	Vector(uint32_t size) : size_(size), alloc_(size), data_(new ValueType[size]) {}
	~Vector()
	{
		if(data_)
			delete [] data_;
	}

	Vector(const Vector &operand) : alloc_(0), size_(0), data_(nullptr)
	{
		*this = operand;
	}

	Vector(Vector &&operand) : size_(operand.size_), alloc_(operand.alloc_), data_(operand.data_)
	{
		operand.alloc_ = 0;
		operand.size_ = 0;
		operand.data_ = nullptr;
	}

	template<typename IteratorType>
	Vector(IteratorType start, IteratorType end) : alloc_(0), size_(0), data_(nullptr)
	{
		assign(start, end);
	}

	const Vector &operator =(const Vector &operand)
	{
		assign(operand.data_, operand.size_);
		return *this;
	}

	const Vector &operator =(Vector &&operand)
	{
		alloc_ = operand.alloc_;
		data_ = operand.data_;
		size_ = operand.size_;

		operand.data_ = nullptr;
		operand.size_ = 0;
		operand.alloc_ = 0;
		return *this;
	}

	ValueType *get() const
	{
		return data_;
	}
	
	void reserve(size_t size)
	{
		size_t oldSize = size_;
		resize_(size, true);
		size_ = oldSize;
	}

	void resize(size_t size)
	{
		resize_(size, true);
	}

	void assign(ValueType *data, size_t size)
	{
		resize_(size, false);
		for(size_t i = 0; i < size; i ++)
			data_[i] = data[i];
	}

	template<typename IteratorType>
	void assign(IteratorType start, IteratorType end)
	{
		size_t length = end - start;
		resize_(length, false);

		size_t i = 0;
		for(IteratorType it = start; it != end; it ++, i ++)
			data_[i] = *it;
	}

	template<typename IteratorType>
	void assign_move(IteratorType start, IteratorType end)
	{
		size_t length = end - start;
		resize_(length, false);

		size_t i = 0;
		for(IteratorType it = start; it != end; it ++, i ++)
			data_[i] = std::move(*it);
	}

	iterator push_back(const ValueType &data)
	{
		resize_(size_ + 1, true);
		data_[size_ - 1] = data;

		return &data_[size_ - 1];
	}

	iterator push_back(ValueType &&data)
	{
		resize_(size_ + 1, true);
		data_[size_ - 1] = std::move(data);

		return &data_[size_ - 1];
	}

	void insert(size_t pos, const ValueType &data)
	{
		insert(pos, &data, 1);
	}

	void insert(size_t pos, const Vector &data)
	{
		insert(pos, data.get(), data.size());
	}

	void insert(size_t pos, const value_type *data, size_t size)
	{
		size_t originalSize = size_;
		resize_(size_ + size, true);

		for(size_t i = originalSize + size - 1; i >= pos + size; i --)
			data_[i] = data_[i - size];
		for(size_t i = 0; i < size; i ++)
			data_[i + pos] = data[i];
	}

	void append(const Vector &data)
	{
		insert(size_, data);
	}

	void append(const value_type *data, size_t size)
	{
		insert(size_, data, size);
	}

	size_t size() const
	{
		return size_;
	}

	ValueType &operator[](uint32_t operand)
	{
		return data_[operand];
	}

	const ValueType &operator[](uint32_t operand) const
	{
		return data_[operand];
	}

	iterator begin()
	{
		return data_;
	}

	iterator end()
	{
		return data_ + size_;
	}

	const_iterator begin() const
	{
		return data_;
	}

	const_iterator end() const
	{
		return data_ + size_;
	}

	virtual SharedPtr<DataView> getView(uint64_t offset, size_t size = 0)
	{
		if(size == 0)
			size = size_;
		return MakeShared<DataView>(sharedFromThis(), offset, size);
	}

	virtual uint8_t *map(uint64_t offset)
	{
		return reinterpret_cast<uint8_t *>(data_ + offset);
	}

	virtual void unmap()
	{

	}
};
