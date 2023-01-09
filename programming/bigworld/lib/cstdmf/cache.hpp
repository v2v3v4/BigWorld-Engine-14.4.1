#ifndef _CACHE_HEADER
#define _CACHE_HEADER

#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a simple cache. It stores a maximum number
 *	of elements, and throws out the least recently used element when
 *	necessary.
 */
template<class Key, class Value> class Cache
{
private:

	struct CacheNode
	{
		Key			key_;
		Value 		value_;
		CacheNode*	pNext_;
		CacheNode*	pPrev_;
	};

	typedef BW::map<Key, CacheNode*>	CacheMap;

public:

	/**
	 *	Constructor
	 *
	 *	@param maxSize	Maximum number of elements to store.
	 */
	Cache(unsigned int maxSize = 100) :
		maxSize_(maxSize)
	{
		cacheChain_.pPrev_ = &cacheChain_;
		cacheChain_.pNext_ = &cacheChain_;
	}

	/**
	 *	This method inserts a new element into the cache.
	 *	It will replace an existing element with the same key,
	 *	if one exists. It will also erase LRU elements if the
	 *	cache size is too big.
	 *
	 *	@param key		Key to insert.
	 *	@param value	Value to insert.
	 */
	void insert(Key key, const Value& value)
	{
		// Make sure it doesn't already exist.
		this->erase(key);

		// Purge elements until we are under the limit.
		while(cacheMap_.size() >= maxSize_)
			this->erase(cacheChain_.pPrev_->key_);

		CacheNode* pNode = new CacheNode;
		pNode->key_ = key;
		pNode->value_ = value;
		this->addToChain(pNode);
		cacheMap_[key] = pNode;
	}

	/**
	 *	This method returns a pointer to a value in the cache
	 *	associated with the given key. It returns NULL if the
	 *	key was not found. Note that the element is moved to
	 *	the head of the cache chain, so it will be deleted
	 *	last.
	 *
	 *	@param key	Key to search for.
	 *	@return		Pointer to the value, or NULL.
	 */
	Value* find(Key key)
	{
		typename CacheMap::iterator iter = cacheMap_.find(key);

		if(iter != cacheMap_.end())
		{
			this->removeFromChain(iter->second);
			this->addToChain(iter->second);
			return &iter->second->value_;
		}

		return NULL;
	}

	/**
	 *	This method erases an element in the cache with the given key.
	 *
	 *	@param key	Key of the element to remove.
	 */
	void erase(Key key)
	{
		typename CacheMap::iterator iter = cacheMap_.find(key);

		if(iter != cacheMap_.end())
		{
			this->removeFromChain(iter->second);
			delete iter->second;
			cacheMap_.erase(iter);
		}
	}

private:

	void addToChain(CacheNode* pNode)
	{
		pNode->pPrev_ = &cacheChain_;
		pNode->pNext_ = cacheChain_.pNext_;
		cacheChain_.pNext_->pPrev_ = pNode;
		cacheChain_.pNext_ = pNode;
	}

	void removeFromChain(CacheNode* pNode)
	{
		pNode->pPrev_->pNext_ = pNode->pNext_;
		pNode->pNext_->pPrev_ = pNode->pPrev_;
	}

private:

	CacheMap 		cacheMap_;
	CacheNode 		cacheChain_;
	unsigned int	maxSize_;

	// Not a concrete class.
	Cache(const Cache&) {}
	Cache& operator=(const Cache&) {}
};

BW_END_NAMESPACE

#endif

