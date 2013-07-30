#pragma once

#include <cstdint>
#include "Runtime.h"

template<typename ValueType>
class Vector
{
private:
	size_t alloc_;
	size_t size_;
	ValueType *data_;

	void resize_(uint32_t size, bool preserve = false)
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
					for(size_t i = 0; i < size_; i ++)
						data_[i] = oldData[i];
				delete [] oldData;
			}
		}
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
		resize_(size, true);
	}

	void resize(size_t size)
	{
		resize_(size, false);
		size_ = size;
	}

	void assign(ValueType *data, size_t size)
	{
		resize(size);
		for(size_t i = 0; i < size; i ++)
			data_[i] = data[i];
	}

	template<typename IteratorType>
	void assign(IteratorType start, IteratorType end)
	{
		size_t length = end - start;
		resize(length);

		size_t i = 0;
		for(IteratorType it = start; it != end; it ++, i ++)
			data_[i] = *it;
	}

	iterator push_back(ValueType data)
	{
		reserve(size_ + 1);
		data_[size_] = data;
		size_ ++;

		return &data_[size_ - 1];
	}

	void insert(size_t pos, ValueType data)
	{
		reserve(size_ + 1);

		for(size_t i = size(); i >= pos + 1; i --)
			data_[i] = data_[i - 1];
		data_[pos] = data;
		size_ ++;
	}

	void insert(size_t pos, const Vector &data)
	{
		reserve(size_ + data.size());

		for(size_t i = size(); i >= pos + data.size(); i --)
			data_[i] = data_[i - data.size()];
		for(size_t i = 0; i < data.size(); i ++)
			data_[i + pos] = data[i];
		size_ += data.size();
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
