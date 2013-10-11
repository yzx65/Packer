#pragma once

//replacement of std::list, to get rid of external dependency.

#include "Runtime.h"

template<typename ValueType>
class List
{
private:
	struct ListNodeBase
	{
		ListNodeBase *next;
		ListNodeBase *prev;
	};
	struct ListNode : public ListNodeBase
	{
		ValueType data;
	};
	ListNodeBase *head_;
	
	template<typename BaseType, typename NodeType, typename ValueType>
	class ListIterator
	{
		friend class List;
	private:
		BaseType *item_;
	public:
		ListIterator(BaseType *item) : item_(item) {}

		ValueType &operator *()
		{
			return static_cast<NodeType *>(item_)->data;
		}

		bool operator !=(const ListIterator &operand) const
		{
			return item_ != operand.item_;
		}

		ListIterator operator ++(int)
		{
			ListIterator result(item_->next);
			item_ = item_->next;
			return result;
		}

		ListIterator &operator ++()
		{
			item_ = item_->next;
			return *this;
		}

		ValueType *operator ->()
		{
			return &static_cast<NodeType *>(item_)->data;
		}

		size_t operator -(const ListIterator &operand) const
		{
			size_t cnt = 0;
			BaseType *node = operand.item_;
			while(node != item_)
			{
				cnt ++;
				node = node->next;
			}
			return cnt;
		}
	};

	void ensureHead_()
	{
		if(!head_)
		{
			head_ = new ListNodeBase();
			head_->next = head_;
			head_->prev = head_;
		}
	}

	ListNode* appendNode_()
	{
		ensureHead_();
		ListNode *item = new ListNode();
		item->next = head_;
		item->prev = head_->prev;
		head_->prev->next = item;
		head_->prev = item;

		return item;
	}
public:
	typedef ValueType value_type;
	typedef ListIterator<const ListNodeBase, const ListNode, const value_type> const_iterator;
	typedef ListIterator<ListNodeBase, ListNode, value_type> iterator;
	List()  : head_(nullptr) {}

	~List()
	{
		clear();
	}

	List(const List &other)
	{
		*this = other;
	}

	List(List &&other) : head_(other.head_)
	{
		other.head_ = nullptr;
	}

	template<typename InputIterator>
	List(InputIterator s, InputIterator e)
	{
		head_ = new ListNodeBase();
		head_->next = head_;
		head_->prev = head_;

		for(; s != e; s ++)
			push_back(*s);
	}

	void clear()
	{
		if(head_)
		{
			ListNodeBase *item = head_->next;
			while(item->next != head_)
			{
				ListNode *item_ = static_cast<ListNode *>(item);
				item = item->next;
				delete item_;
			}
			delete head_;
		}
	}

	const List &operator =(List &&operand)
	{
		head_ = operand.head_;
		operand.head_ = nullptr;

		return *this;
	}

	const List &operator =(const List &operand)
	{
		head_ = new ListNodeBase();
		head_->next = head_;
		head_->prev = head_;

		ListNodeBase *item = operand.head_->next;
		while(item != operand.head_)
		{
			ListNode *item_ = static_cast<ListNode *>(item);
			push_back(item_->data);
			item = item->next;
		}

		return *this;
	}

	iterator push_back(const value_type &data)
	{
		ListNode *item = appendNode_();
		item->data = data;

		return iterator(item);
	}

	iterator push_back(value_type &&data)
	{
		ListNode *item = appendNode_();
		item->data = std::move(data);

		return iterator(item);
	}

	void remove(iterator at)
	{
		if(at.item_->prev == nullptr && at.item_->next == nullptr)
		{
			delete at.item_;
			delete head_;
			head_ = nullptr;
			return;
		}
		ListNodeBase *item = at.item_;
		item->prev->next = item->next;
		item->next->prev = item->prev;
		delete static_cast<ListNode *>(item);
	}

	template<typename InputIterator>
	iterator insert(iterator at, InputIterator start, InputIterator end)
	{
		ensureHead_();
		ListNode *position = static_cast<ListNode *>(at.item_);
		for(; start != end; start ++)
		{
			ListNode *item = new ListNode();
			item->next = position;
			item->data = *start;
			item->prev = position->prev;
			position->prev->next = item;
			position->prev = item;
		}
		return iterator(position);
	}

	const_iterator begin() const
	{
		if(!head_)
			return const_iterator(nullptr);
		return const_iterator(head_->next);
	}

	const_iterator end() const
	{
		if(!head_)
			return const_iterator(nullptr);
		return const_iterator(head_);
	}

	iterator begin()
	{
		if(!head_)
			return iterator(nullptr);
		return iterator(head_->next);
	}

	iterator end()
	{
		if(!head_)
			return iterator(nullptr);
		return iterator(head_);
	}

	size_t size() const
	{
		if(!head_)
			return 0;
		size_t size = 0;
		const ListNodeBase *item = head_;
		while(item->next != head_)
		{
			size ++;
			item = item->next;
		}
		return size;
	}
};