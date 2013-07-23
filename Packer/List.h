#pragma once

//replacement of std::list, to get rid of external dependency.


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

		ListIterator operator ++()
		{
			ListIterator result(item_->next);
			item_ = item_->next;
			return result;
		}

		ListIterator operator ++(int)
		{
			ListIterator result(item_);
			item_ = item_->next;
			return result;
		}
	};
public:
	typedef ValueType value_type;
	typedef ListIterator<const ListNodeBase, const ListNode, const value_type> const_iterator;
	typedef ListIterator<ListNodeBase, ListNode, value_type> iterator;
	List() 
	{
		head_ = new ListNodeBase();
		head_->next = head_;
		head_->prev = head_;
	}

	~List()
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

	List(const List &other)
	{
		head_ = new ListNodeBase();
		head_->next = head_;
		head_->prev = head_;

		ListNodeBase *item = other.head_->next;
		while(item != other.head_)
		{
			ListNode *item_ = static_cast<ListNode *>(item);
			push_back(item_->data);
			item = item->next;
		}
	}

	void push_back(const value_type &data)
	{
		ListNode *item = new ListNode();
		item->next = head_;
		item->data = data;
		item->prev = head_->prev;
		head_->prev->next = item;
		head_->prev = item;
	}

	void push_back(value_type &&data)
	{
		ListNode *item = new ListNode();
		item->next = head_;
		item->data = std::move(data);
		item->prev = head_->prev;
		head_->prev->next = item;
		head_->prev = item;
	}

	template<typename InputIterator>
	iterator insert(iterator at, InputIterator start, InputIterator end)
	{
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
		return const_iterator(head_->next);
	}

	const_iterator end() const
	{
		return const_iterator(head_);
	}

	iterator begin()
	{
		return iterator(head_->next);
	}

	iterator end()
	{
		return iterator(head_);
	}

	size_t size() const
	{
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