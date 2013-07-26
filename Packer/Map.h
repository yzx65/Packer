#pragma once

#include "Runtime.h"

template<typename KeyType, typename ValueType>
class Map
{
private:
	struct MapNode
	{
		MapNode(const KeyType &key_, const ValueType &value) : left(nullptr), right(nullptr), key(key_), data(value) {}
		MapNode(const KeyType &key_, ValueType &&value) : left(nullptr), right(nullptr), key(key_), data(std::move(value)) {}
		MapNode *left;
		MapNode *right;
		KeyType key;
		ValueType data;
	};
	MapNode *head_;

	template<typename NodeType, typename ValueType>
	class MapIterator
	{
	private:
		NodeType *node_;
	public:
		MapIterator(NodeType *node) : node_(node) {}

		ValueType &operator *()
		{
			return node_->data;
		}

		bool operator ==(const MapIterator &operand)
		{
			return node_ == operand.node_;
		}
	};
public:
	typedef ValueType value_type;
	typedef MapIterator<MapNode, value_type> iterator;
	typedef MapIterator<const MapNode, const value_type> const_iterator;

private:
	template<typename InsertType>
	iterator insert_(const KeyType &key, InsertType value)
	{
		if(!head_)
		{
			head_ = new MapNode(key, value);
			return iterator(nullptr);
		}
		MapNode *item = head_;
		while(true)
		{
			if(item->key < key)
			{
				if(item->right)
				{
					item = item->right;
					continue;
				}
				item->right = new MapNode(key, value);
			}
			else
			{
				if(item->left)
				{
					item = item->left;
					continue;
				}
				item->left = new MapNode(key, value);
			}
			break;
		}
		return iterator(item);
	}
	
	template<typename IteratorType, typename KeyType>
	IteratorType find_(const KeyType &key)
	{
		MapNode *item = head_;
		while(item->key != key)
		{
			if(item->key < key)
			{
				if(item->right)
				{
					item = item->right;
					continue;
				}
				else
					return IteratorType(nullptr);
			}
			else
			{
				if(item->left)
				{
					item = item->left;
					continue;
				}
				else
					return IteratorType(nullptr);
			}
		}
		return IteratorType(item);
	}
public:

	Map() : head_(nullptr)
	{
	}

	iterator insert(const KeyType &key, const ValueType &value)
	{
		return insert_<const ValueType &>(key, value);
	}

	iterator insert(const KeyType &key, ValueType &&value)
	{
		return insert_<ValueType &&>(key, std::move(value));
	}

	ValueType &operator [](const KeyType &key)
	{
		iterator it = find(key);
		if(it == end())
			return *insert(key, ValueType());
		return *it;
	}

	iterator find(const KeyType &key)
	{
		return find_<iterator>(key);
	}

	iterator end()
	{
		return iterator(nullptr);
	}

	iterator begin()
	{
		MapNode *item = head_;
		while(true)
		{
			if(item->left)
				item = item->left;
			break;
		}
		return iterator(item);
	}
};
