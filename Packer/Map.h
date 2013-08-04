#pragma once

#include "Runtime.h"

template<typename ValueType>
class DefaultComparator
{
public:
	bool operator ()(const ValueType &a, const ValueType &b)
	{
		return a < b;
	}
};

template<typename KeyType, typename ValueType, typename Comparator = DefaultComparator<KeyType>>
class Map
{
private:
	struct MapNode
	{
		MapNode(const KeyType &key_, const ValueType &value) : left(nullptr), right(nullptr), key(key_), value(value) {}
		MapNode(const KeyType &key_, ValueType &&value) : left(nullptr), right(nullptr), key(key_), value(std::move(value)) {}
		MapNode *left;
		MapNode *right;
		KeyType key;
		ValueType value;
	};
	MapNode *head_;

	template<typename NodeType, typename ValueType>
	class MapIterator
	{
	private:
		NodeType *node_;
		Map *map_;
	public:
		MapIterator(NodeType *node, Map *map) : node_(node), map_(map) {}

		MapNode &operator *()
		{
			return *node_;
		}

		MapNode *operator ->()
		{
			return node_;
		}

		bool operator ==(const MapIterator &operand)
		{
			return node_ == operand.node_;
		}

		bool operator !=(const MapIterator &operand)
		{
			return node_ != operand.node_;
		}

		MapIterator &operator ++()
		{
			*this = map_->upper_bound(node_->key);
			return *this;
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
			return iterator(nullptr, this);
		}
		MapNode *item = head_;
		while(true)
		{
			if(Comparator()(item->key, key))
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
		return iterator(item, this);
	}
	
	template<typename IteratorType, typename KeyType>
	IteratorType find_(const KeyType &key)
	{
		MapNode *item = head_;
		while(item->key != key)
		{
			if(Comparator()(item->key, key))
			{
				if(item->right)
				{
					item = item->right;
					continue;
				}
				else
					return IteratorType(nullptr, this);
			}
			else
			{
				if(item->left)
				{
					item = item->left;
					continue;
				}
				else
					return IteratorType(nullptr, this);
			}
		}
		return IteratorType(item, this);
	}

	//find lowest value in tree bigger than key.
	MapNode *find_upper_bound_(const KeyType &key, MapNode *node, MapNode *lowest)
	{
		if(Comparator()(node->key, lowest->key) && (Comparator()(node->key, key) || Comparator()(key, node->key)) && Comparator()(key, node->key)) // node < lowest && node != key && node > key
			lowest = node;
		
		MapNode *left = nullptr;
		MapNode *right = nullptr;
		if(node->left)
			left = find_upper_bound_(key, node->left, lowest);
		if(node->right)
			right = find_upper_bound_(key, node->right, lowest);
		if(!left && !right)
			return lowest;
		if(!left)
			return right;
		if(!right)
			return left;
		if(Comparator()(left->key, right->key))
			return left;
		else
			return right;
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
			return insert(key, ValueType())->value;
		return it->value;
	}

	iterator upper_bound(const KeyType &key)
	{
		MapNode *highest = head_;
		while(true)
		{
			if(highest->right)
				highest = highest->right;
			else
				break;
		}
		if(key == highest->key)
			return iterator(nullptr, this);
		return iterator(find_upper_bound_(key, head_, highest), this);
	}

	iterator find(const KeyType &key)
	{
		return find_<iterator>(key);
	}

	iterator end()
	{
		return iterator(nullptr, this);
	}

	iterator begin()
	{
		MapNode *item = head_;
		while(true)
		{
			if(item->left)
				item = item->left;
			else
				break;
		}
		return iterator(item, this);
	}
};
