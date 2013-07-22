#pragma once

#include <cstdint>

template<typename T>
class Vector
{
private:
	int32_t size_;
	T *data_;
public:
	Vector() : size_(-1), data_(nullptr) {}
	Vector(uint32_t size) : size_(size), data_(new T[size]) {}
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

	T *get() const
	{
		return data_;
	}

	void resize(uint32_t size)
	{
		size_ = size;
		if(data_)
			delete data_;
		data_ = new T[size];
	}

	void assign(T *data, uint32_t size)
	{
		resize(size);
		for(uint32_t i = 0; i < size; i ++)
			data_[i] = T(data[i]);
	}

	int32_t size() const
	{
		return size_;
	}

	T &operator[](uint32_t operand)
	{
		return data_[operand];
	}

	const T &operator[](uint32_t operand) const
	{
		return data_[operand];
	}

	T *begin()
	{
		return data_;
	}

	T *end()
	{
		return data_ + size_;
	}

	const T *begin() const
	{
		return data_;
	}

	const T *end() const
	{
		return data_ + size_;
	}
};
