#pragma once

#include <cstdint>
#include <type_traits> //for is_pod
#include "Runtime.h"
#include "Util.h"
#include "SharedPtr.h"

#include "DataSource.h"

template<typename ValueType>
class Vector
{
private:
	template<typename ValueType>
	struct VectorData : public DataSource, public EnableSharedFromThis<VectorData<ValueType>>
	{
		size_t alloc;
		size_t size;
		ValueType *data;

		VectorData() : alloc(0), size(0), data(nullptr)
		{

		}

		virtual ~VectorData()
		{
			if(data)
				delete [] data;
		}

		virtual SharedPtr<DataView> getView(uint64_t offset, size_t size)
		{
			if(size == 0)
				size = this->size;
			return MakeShared<DataView>(sharedFromThis(), offset, size);
		}

		virtual uint8_t *map(uint64_t offset)
		{
			return reinterpret_cast<uint8_t *>(data) + offset;
		}

		virtual void unmap()
		{

		}
	};
	SharedPtr<VectorData<ValueType>> data_;

	template<typename T = ValueType>
	static typename std::enable_if<std::is_pod<T>::value>::type
		_copyData(ValueType *target, ValueType *oldData, size_t size)
	{
		copyMemory(target, oldData, sizeof(value_type) * size);
	}

	template<typename T = ValueType>
	static typename std::enable_if<!std::is_pod<T>::value>::type
		_copyData(ValueType *target, ValueType *oldData, size_t size)
	{
		for(size_t i = 0; i < size; i ++)
			target[i] = std::move(oldData[i]);
	}

	void resize_(uint32_t size, bool preserve)
	{
		if(data_->alloc <= size)
		{
			clone_();
			data_->alloc *= 2;
			if(data_->alloc < size)
				data_->alloc = size + 10;
			ValueType *oldData = data_->data;
			data_->data = new ValueType[data_->alloc];
			if(oldData)
			{
				if(preserve)
					_copyData(data_->data, oldData, data_->size);
				delete[] oldData;
			}
		}
		data_->size = size;
	}

	void clone_()
	{
		if(!data_)
		{
			data_ = new VectorData<ValueType>();
			return;
		}
		if(data_.getRef() == 1)
			return;
		VectorData<ValueType> *newData = new VectorData<ValueType>();
		newData->alloc = data_->alloc;
		newData->size = data_->size;
		newData->data = new ValueType[newData->alloc];
		_copyData(newData->data, data_->data, data_->size);
		data_ = SharedPtr<VectorData<ValueType>>(newData);
	}
public:
	typedef ValueType value_type;
	typedef ValueType *iterator;
	typedef ValueType *const const_iterator;

	Vector() : data_(nullptr)
	{
	}

	Vector(uint32_t size) : data_(new VectorData<ValueType>())
	{
		 data_->size = size;
		 data_->alloc = size;
		 data_->data = new ValueType[size];
	}

	~Vector()
	{
	}

	Vector(const Vector &operand) : data_(operand.data_)
	{
	}

	Vector(Vector &&operand) : data_(std::move(operand.data_))
	{
	}

	template<typename IteratorType>
	Vector(IteratorType start, IteratorType end) : data_(new VectorData<ValueType>())
	{
		assign(start, end);
	}

	const Vector &operator =(const Vector &operand)
	{
		data_ = operand.data_;
		return *this;
	}

	const Vector &operator =(Vector &&operand)
	{
		data_ = std::move(operand.data_);
		return *this;
	}

	ValueType *get() const
	{
		if(!data_)
			return nullptr;
		return data_->data;
	}

	void reserve(size_t size)
	{
		clone_();
		size_t oldSize = data_->size;
		resize_(size, true);
		data_->size = oldSize;
	}

	void resize(size_t size)
	{
		clone_();
		resize_(size, true);
	}

	void assign(ValueType *data, size_t size)
	{
		clone_();
		resize_(size, false);
		for(size_t i = 0; i < size; i ++)
			data_->data[i] = data[i];
	}

	template<typename IteratorType>
	void assign(IteratorType start, IteratorType end)
	{
		clone_();
		size_t length = end - start;
		resize_(length, false);

		size_t i = 0;
		for(IteratorType it = start; it != end; it ++, i ++)
			data_->data[i] = *it;
	}

	template<typename IteratorType>
	void assign_move(IteratorType start, IteratorType end)
	{
		clone_();
		size_t length = end - start;
		resize_(length, false);

		size_t i = 0;
		for(IteratorType it = start; it != end; it ++, i ++)
			data_->data[i] = std::move(*it);
	}

	iterator push_back(const ValueType &data)
	{
		clone_();
		resize_(data_->size + 1, true);
		data_->data[data_->size - 1] = data;

		return &data_->data[data_->size - 1];
	}

	iterator push_back(ValueType &&data)
	{
		clone_();
		resize_(data_->size + 1, true);
		data_->data[data_->size - 1] = std::move(data);

		return &data_->data[data_->size - 1];
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
		clone_();
		size_t originalSize = data_->size;
		resize_(data_->size + size, true);

		for(size_t i = originalSize + size - 1; i >= pos + size; i --)
			data_->data[i] = data_->data[i - size];
		for(size_t i = 0; i < size; i ++)
			data_->data[i + pos] = data[i];
	}

	void append(const Vector &data)
	{
		clone_();
		insert(data_->size, data);
	}

	void append(const value_type *data, size_t size)
	{
		clone_();
		insert(data_->size, data, size);
	}

	size_t size() const
	{
		if(!data_)
			return 0;
		return data_->size;
	}

	ValueType &operator[](uint32_t operand)
	{
		clone_();
		return data_->data[operand];
	}

	const ValueType &operator[](uint32_t operand) const
	{
		return data_->data[operand];
	}

	iterator begin()
	{
		clone_();
		return data_->data;
	}

	iterator end()
	{
		clone_();
		return data_->data + data_->size;
	}

	const_iterator begin() const
	{
		return data_->data;
	}

	const_iterator end() const
	{
		return data_->data + data_->size;
	}

	SharedPtr<DataView> getView(uint64_t offset, size_t size = 0)
	{
		clone_();
		return data_->getView(offset, size);
	}

	SharedPtr<DataSource> asDataSource()
	{
		return data_;
	}
};
