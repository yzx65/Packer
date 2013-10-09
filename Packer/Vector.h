#pragma once

#include <cstdint>
#include "Runtime.h"
#include "Util.h"

template<typename ValueType>
class Vector
{
private:
	size_t alloc_;
	size_t size_;
	ValueType *data_;

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
					copyMemory(data_, oldData, sizeof(value_type) * size_);
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

	void insert(size_t pos, ValueType data)
	{
		size_t originalSize = size_;
		resize_(size_ + 1, true);

		for(size_t i = originalSize; i >= pos + 1; i --)
			data_[i] = data_[i - 1];
		data_[pos] = data;
	}

	void insert(size_t pos, const Vector &data)
	{
		size_t originalSize = size_;
		resize_(size_ + data.size(), true);

		for(size_t i = originalSize + data.size() - 1; i >= pos + data.size(); i --)
			data_[i] = data_[i - data.size()];
		for(size_t i = 0; i < data.size(); i ++)
			data_[i + pos] = data[i];
	}

	void append(const Vector &data)
	{
		insert(size_, data);
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
};
