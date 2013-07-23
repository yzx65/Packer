#pragma once

#include <cstdint>
#include "Win32Runtime.h"

template<typename ValueType>
class Vector
{
private:
	int32_t size_;
	ValueType *data_;
public:
	Vector() : size_(-1), data_(nullptr) {}
	Vector(uint32_t size) : size_(size), data_(new ValueType[size]) {}
	~Vector()
	{
		if(data_)
			delete [] data_;
	}

	Vector(const Vector &operand)
	{
		*this = operand;
	}

	Vector(Vector &&operand) : size_(operand.size_), data_(operand.data_)
	{
		operand.size_ = -1;
		operand.data_ = nullptr;
	}

	const Vector &operator =(const Vector &operand)
	{
		data_ = nullptr;
		assign(operand.data_, operand.size_);
		return *this;
	}

	const Vector &operator =(Vector &&operand)
	{
		data_ = operand.data_;
		size_ = operand.size_;

		operand.data_ = nullptr;
		operand.size_ = -1;
		return *this;
	}

	Vector copy()
	{
		Vector result;
		result.assign(data_, size_);
		return result;
	}

	ValueType *get() const
	{
		return data_;
	}

	void resize(uint32_t size)
	{
		size_ = size;
		if(data_)
			delete data_;
		data_ = new ValueType[size];
	}

	void assign(ValueType *data, uint32_t size)
	{
		resize(size);
		for(uint32_t i = 0; i < size; i ++)
			data_[i] = ValueType(data[i]);
	}

	int32_t size() const
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

	ValueType *begin()
	{
		return data_;
	}

	ValueType *end()
	{
		return data_ + size_;
	}

	const ValueType *begin() const
	{
		return data_;
	}

	const ValueType *end() const
	{
		return data_ + size_;
	}
};
