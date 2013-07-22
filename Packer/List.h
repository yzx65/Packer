#pragma once

//replacement of std::list, to get rid of external dependency.

template<typename T>
class List
{
private:
	struct ListTag
	{
		ListTag *next;
		ListTag *prev;
	};
	struct ListItem : public ListTag
	{
		T data;
	};
	ListTag *head_;
	
	template<typename TagType, typename ItemType, typename ValueType>
	class ListIterator
	{
	private:
		TagType *item_;
	public:
		ListIterator(TagType *item) : item_(item) {}

		ValueType &operator *()
		{
			return static_cast<ItemType *>(item_)->data;
		}

		bool operator !=(const ListIterator &operand) const
		{
			return item_ != operand.item_;
		}

		const ListIterator &operator ++()
		{
			item_ = item_->next;
			return *this;
		}
	};
public:
	typedef T value_type;
	typedef ListIterator<const ListTag, const ListItem, const value_type> const_iterator;
	typedef ListIterator<ListTag, ListItem, value_type> iterator;
	List() 
	{
		head_ = new ListTag();
		head_->next = head_;
		head_->prev = head_;
	}

	~List()
	{
		ListTag *item = head_->next;
		while(item->next != head_)
		{
			ListItem *item_ = static_cast<ListItem *>(item);
			item = item->next;
			delete item_;
		}
		delete head_;
	}

	List(const List &other)
	{
		head_ = new ListTag();
		head_->next = head_;
		head_->prev = head_;

		ListTag *item = other.head_->next;
		while(item->next != other.head_)
		{
			ListItem *item_ = static_cast<ListItem *>(item);
			push_back(item_->data);
			item = item->next;
		}
	}

	void push_back(const value_type &data)
	{
		ListItem *item = new ListItem();
		item->next = head_;
		item->data = data;
		item->prev = head_->prev;
		head_->prev->next = item;
		head_->prev = item;
	}

	void push_back(value_type &&data)
	{
		ListItem *item = new ListItem();
		item->next = head_;
		item->data = std::move(data);
		item->prev = head_->prev;
		head_->prev->next = item;
		head_->prev = item;
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
		const ListTag *item = head_;
		while(item->next != head_)
		{
			size ++;
			item = item->next;
		}
		return size;
	}
};