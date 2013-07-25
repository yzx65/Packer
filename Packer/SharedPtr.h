#pragma once

#include <cstdint>

namespace Impl
{
	class RefCounter
	{
	private:
		size_t refCount_;
	public:
		RefCounter() : refCount_(0) {}

		void decRef()
		{
			refCount_ --;
		}

		void addRef()
		{
			refCount_ ++;
		}

		size_t refCount()
		{
			return refCount_;
		}
	};

	class DeleterBase
	{
	public:
		virtual void execute(void *) = 0;
	};

	template<typename T>
	class Deleter : public DeleterBase
	{
		virtual void execute(void *ptr)
		{
			delete reinterpret_cast<T *>(ptr);
		}
	};
}

template<typename PointerType>
class SharedPtr
{
	friend class SharedPtr;
private:
	PointerType *item_;
	Impl::RefCounter *refCounter_;
	Impl::DeleterBase *deleter_;
public:
	SharedPtr(PointerType *item) : item_(nullptr), refCounter_(nullptr), deleter_(nullptr)
	{
		reset(item, new Impl::RefCounter(), new Impl::Deleter<PointerType>());
	}

	SharedPtr() : item_(nullptr), refCounter_(nullptr), deleter_(nullptr) {}

	SharedPtr(const SharedPtr &other) : item_(nullptr), refCounter_(nullptr), deleter_(nullptr)
	{
		reset(other.item_, other.refCounter_, other.deleter_);
	}

	template<typename PointerType2>
	SharedPtr(const SharedPtr<PointerType2> &other) : item_(nullptr), refCounter_(nullptr), deleter_(nullptr)
	{
		reset(static_cast<PointerType *>(other.item_), other.refCounter_, other.deleter_);
	}

	~SharedPtr()
	{
		reset(nullptr, nullptr, nullptr);
	}

	const SharedPtr &operator =(const SharedPtr &other)
	{
		reset(other.item_, other.refCounter_, other.deleter_);
		return *this;
	}

	void reset(PointerType *item, Impl::RefCounter *refCounter, Impl::DeleterBase *deleter)
	{
		if(item_)
		{
			refCounter_->decRef();
			if(refCounter_->refCount() == 0)
			{
				deleter_->execute(item_);
				delete deleter_;
				delete refCounter_;
			}
		}
		item_ = item;
		refCounter_ = refCounter;
		deleter_ = deleter;
		if(item_)
			refCounter->addRef();
	}

	PointerType *operator ->()
	{
		return item_;
	}
};

//VC11 doesn't support variadic templates.. damn
template<typename PointerType>
SharedPtr<PointerType> MakeShared()
{
	return SharedPtr<PointerType>(new PointerType());
}

template<typename PointerType, typename T1>
SharedPtr<PointerType> MakeShared(T1 a1)
{
	return SharedPtr<PointerType>(new PointerType(a1));
}

template<typename PointerType, typename T1, typename T2>
SharedPtr<PointerType> MakeShared(T1 a1, T2 a2)
{
	return SharedPtr<PointerType>(new PointerType(a1, a2));
}

template<typename PointerType, typename T1, typename T2, typename T3>
SharedPtr<PointerType> MakeShared(T1 a1, T2 a2, T3 a3)
{
	return SharedPtr<PointerType>(new PointerType(a1, a2, a3));
}

template<typename PointerType, typename T1, typename T2, typename T3, typename T4>
SharedPtr<PointerType> MakeShared(T1 a1, T2 a2, T3 a3, T4 a4)
{
	return SharedPtr<PointerType>(new PointerType(a1, a2, a3, a4));
}

template<typename PointerType, typename T1, typename T2, typename T3, typename T4, typename T5>
SharedPtr<PointerType> MakeShared(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
{
	return SharedPtr<PointerType>(new PointerType(a1, a2, a3, a4, a5));
}

template<typename PointerType, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
SharedPtr<PointerType> MakeShared(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
{
	return SharedPtr<PointerType>(new PointerType(a1, a2, a3, a4, a5, a6));
}