#pragma once

#include "TypeTraits.h"

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
		MapNode(const KeyType &key_, const ValueType &value_, MapNode *parent_) : left(nullptr), right(nullptr), key(key_), value(value_), parent(parent_) {}
		MapNode(const KeyType &key_, ValueType &&value_, MapNode *parent_) : left(nullptr), right(nullptr), key(key_), value(std::move(value_)), parent(parent_) {}
		MapNode *left;
		MapNode *right;
		MapNode *parent;
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
			head_ = new MapNode(key, value, nullptr);
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
				item->right = new MapNode(key, value, item);
			}
			else
			{
				if(item->left)
				{
					item = item->left;
					continue;
				}
				item->left = new MapNode(key, value, item);
			}
			break;
		}
		return iterator(item, this);
	}
	
	template<typename KeyType>
	MapNode *find_(const KeyType &key)
	{
		if(!head_)
			return nullptr;
		MapNode *item = head_;
		while(Comparator()(item->key, key) || Comparator()(key, item->key)) //item->key < key or key > item->key => key != item->key
		{
			if(Comparator()(item->key, key))
			{
				if(item->right)
				{
					item = item->right;
					continue;
				}
				else
					return nullptr;
			}
			else
			{
				if(item->left)
				{
					item = item->left;
					continue;
				}
				else
					return nullptr;
			}
		}
		return item;
	}

	//find lowest value in tree bigger than key.
	MapNode *upper_bound_(MapNode *current, MapNode *lowest, MapNode *bound)
	{
		if(!Comparator()(bound->key, current->key)) //!(bound->key < current->key) => bound->key >= current->key
			return lowest;
		if(Comparator()(current->key, lowest->key)) //current->key < lowest->key
			lowest = current;
		if(current->left)
			lowest = upper_bound_(current->left, lowest, bound);
		if(current->right)
			lowest = upper_bound_(current->right, lowest, bound);
		return lowest;
	}
	void clear_(MapNode *node)
	{
		if(!node)
			return;
		clear_(node->left);
		clear_(node->right);
		delete node;
	}
public:

	Map() : head_(nullptr)
	{
	}

	~Map()
	{
		clear();
	}

	void clear()
	{
		clear_(head_);
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
		MapNode *node = find_(key);
		if(!node)
			return iterator(nullptr, this);
		if(!node->parent && !node->right)
			//this is root node and highest node
			return iterator(nullptr, this);
		
		if(node->right) //right node is always lower than parent node.
			return iterator(upper_bound_(node->right, node->right, node), this);

		MapNode *search = node->parent;
		while(search)
		{
			if(!Comparator()(search->key, node->key))//!(search->key < node->key) => search->key >= node->key
				return iterator(search, this);
			search = search->parent;
		}
		return iterator(nullptr, this);
	}

	iterator find(const KeyType &key)
	{
		return iterator(find_(key), this);
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
