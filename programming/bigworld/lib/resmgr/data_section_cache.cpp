/**
 *	data_section_cache.cpp
 */

#include "pch.hpp"

#include "data_section_cache.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/dprintf.hpp"
#include "cstdmf/watcher.hpp"

#include <stdio.h>

BW_BEGIN_NAMESPACE

// Statics
int DataSectionCache::s_maxBytes_ = 0;
int DataSectionCache::s_currentBytes_ = 0;
int DataSectionCache::s_hits_ = 0;
int DataSectionCache::s_misses_ = 0;

/**
 * DataSectionCache constructor
 */
DataSectionCache::DataSectionCache() :
	cacheHead_(NULL),
	cacheTail_(NULL)
{
	MF_WATCH("cache/maximum bytes", s_maxBytes_, Watcher::WT_READ_WRITE, "Maximum size for the data cache." );
	MF_WATCH("cache/current bytes", s_currentBytes_, Watcher::WT_READ_WRITE, "Current size of the data cache." );
	MF_WATCH("cache/hits", s_hits_, Watcher::WT_READ_WRITE, "Number of data cache hits." );
	MF_WATCH("cache/misses", s_misses_, Watcher::WT_READ_WRITE, "Number of data cache misses." );
}

/**
 * DataSectionCache destructor. Just deletes the cache chain.
 */
DataSectionCache::~DataSectionCache()
{
	while (cacheHead_ != NULL)
	{
		this->purgeLRU();
	}
#if ENABLE_WATCHERS
	if (Watcher::hasRootWatcher())
	{
		MF_VERIFY( Watcher::rootWatcher().removeChild( "cache/maximum bytes" ) );
		MF_VERIFY( Watcher::rootWatcher().removeChild( "cache/current bytes" ) );
		MF_VERIFY( Watcher::rootWatcher().removeChild( "cache/hits" ) );
		MF_VERIFY( Watcher::rootWatcher().removeChild( "cache/misses" ) );
	}
#endif // ENABLE_WATCHERS
}

/*static*/ DataSectionCache* DataSectionCache::s_instance = NULL;

/*static*/ DataSectionCache* DataSectionCache::instance()
{
	//static DataSectionCache s_dcs;
	//return &s_dcs;
	if (s_instance == NULL)
	{
		s_instance = new DataSectionCache();
	}
	return s_instance;
}

DataSectionCache* DataSectionCache::setSize( int maxBytes )
{
	SimpleMutexHolder permission( accessControl_ );
	
	s_maxBytes_ = maxBytes;
	return this;
}

/*static*/ void DataSectionCache::fini()
{
	bw_safe_delete(s_instance);
}

/**
 *	This method adds a DataSection to the cache. If the node is already there,
 *	it will be replaced.
 *
 *	@param name			Full pathname of the DataSection
 *	@param dataSection	The DataSection to be added
 *
 *	@return None
 */
void DataSectionCache::add( const BW::string & name,
	DataSectionPtr dataSection )
{
	// If the cached object size is greater than the cache size, do not cache
	// it. Also, in some special cases a datasection's size can be zero, in
	// which case we don't want to cache it.
	int bytes = dataSection->bytes();
	if (bytes == 0 || bytes > s_maxBytes_)
	{
		return;
	}

	SimpleMutexHolder permission( accessControl_ );

	DataSectionMap::iterator it;
	CacheNode* pNode;

	// Purge entries from the cache until we are below our desired size.
	while (cacheHead_ != NULL && (s_currentBytes_ + bytes > s_maxBytes_))
	{
		purgeLRU();
	}

	// If there is an existing entry, just replace the smart pointer.

	it = map_.find( name );

	if (it != map_.end())
	{
		pNode = it->second;
		s_currentBytes_ -= pNode->bytes_;
		MF_ASSERT_DEBUG(
			( s_currentBytes_ <= s_maxBytes_ ) && ( s_currentBytes_ >= 0 ) ); 
		pNode->dataSection_ = dataSection;
		pNode->bytes_ = bytes;
		s_currentBytes_ += pNode->bytes_;
		MF_ASSERT_DEBUG(
			( s_currentBytes_ <= s_maxBytes_ ) && ( s_currentBytes_ >= 0 ) ); 
		return;
	}

	// Allocate a new cache node, place it at the head of the cache
	// chain, and add it to the map.

	pNode = new CacheNode;
	pNode->path_ = name;
	pNode->dataSection_ = dataSection;
	pNode->bytes_ = bytes;
	pNode->prev_ = NULL;
	pNode->next_ = cacheHead_;

    if(cacheHead_)
		cacheHead_->prev_ = pNode;

	cacheHead_ = pNode;

	if(cacheTail_ == NULL)
		cacheTail_ = pNode;

	map_.insert( DataSectionMap::value_type( name, pNode ) );

	s_currentBytes_ += bytes;
	MF_ASSERT_DEBUG(
		( s_currentBytes_ <= s_maxBytes_ ) && ( s_currentBytes_ >= 0 ) ); 
}

#if defined( _WIN32 )
BW_END_NAMESPACE
#include <xtree>
BW_BEGIN_NAMESPACE
#endif

/**
 *	This method finds a DataSection in the cache. It returns a smart pointer to
 *	the DataSection, or a NULL smart pointer if not found.
 *
 *	@param name			Full pathname of the DataSection
 *
 *	@return				Smart pointer to the DataSection
 */
DataSectionPtr DataSectionCache::find( const BW::string & name )
{
	SimpleMutexHolder permission( accessControl_ );

	DataSectionMap::iterator it;
	CacheNode* pNode;

	it = map_.find( name );

	// If we found it, move it to the head of the cache chain,
	// since it is now the most recently accessed node.

	if(it != map_.end())
	{
		pNode = it->second;
		moveToHead(pNode);
		s_hits_++;
		return pNode->dataSection_;
	}
	
	s_misses_++;
	return (DataSection *)NULL;
}


/**
 *	This method removes a DataSection from the cache. Note that the actual
 *	DataSection will not be deleted until all references to it are removed.
 *
 *	@param name			Full pathname of the DataSection
 *
 *	@return				None
 */
void DataSectionCache::remove( const BW::string & name )
{
	SimpleMutexHolder permission( accessControl_ );

	DataSectionMap::iterator it;
	CacheNode* pNode;

	it = map_.find(name);

	if (it != map_.end())
	{
		pNode = it->second;
		s_currentBytes_ -= pNode->bytes_;
		MF_ASSERT_DEBUG(
			( s_currentBytes_ <= s_maxBytes_ ) && ( s_currentBytes_ >= 0 ) ); 
		unlinkNode(pNode);

		bw_safe_delete(pNode);
		map_.erase( it );
	}
}

/**
 *	This method clears the whole cache.
 */
void DataSectionCache::clear()
{
	SimpleMutexHolder permission( accessControl_ );

	while (cacheHead_ != NULL)
		this->purgeLRU();

	// Reset all the stats
	s_currentBytes_ = 0;
	s_hits_ = 0;
	s_misses_ = 0;
}


/**
 *	This method purges the least recently used element from the cache.
 *	It does nothing if the cache is empty.
 *
 *	@return				None
 */
void DataSectionCache::purgeLRU()
{
	if (cacheTail_)
	{
		CacheNode* pNode = cacheTail_;
		s_currentBytes_ -= pNode->bytes_;
		MF_ASSERT_DEBUG(
			( s_currentBytes_ <= s_maxBytes_ ) && ( s_currentBytes_ >= 0 ) ); 
		this->unlinkNode(pNode);

		DataSectionMap::iterator it = map_.find( pNode->path_ );

		bw_safe_delete(pNode);

		if (it != map_.end())
		{
			map_.erase( it );
		}
	}
}


/**
 *	This method moves the specified node to the head of the cache chain.
 *	This indicates that it is the most recently used.
 *
 *	@return				None
 */
void DataSectionCache::moveToHead(CacheNode* pNode)
{
	if(pNode != cacheHead_)
	{
		this->unlinkNode(pNode);
		pNode->next_ = cacheHead_;
		cacheHead_->prev_ = pNode;
		cacheHead_ = pNode;
	}
}


/**
 *	This method unlinks a node from the cache chain, and sets
 *	its next and prev pointers to NULL.
 *
 *	@param pNode		The node to unlink
 *
 *	@return				None
 */
void DataSectionCache::unlinkNode(CacheNode* pNode)
{
	// Case 1: Only node

	if(pNode == cacheHead_ && pNode == cacheTail_)
	{
		cacheHead_ = NULL;
		cacheTail_ = NULL;
	}

	// Case 2: Head

	else if(pNode == cacheHead_)
	{
		cacheHead_ = cacheHead_->next_;
		cacheHead_->prev_ = NULL;
	}

	// Case 3: Tail

	else if(pNode == cacheTail_)
	{
		cacheTail_ = cacheTail_->prev_;
		cacheTail_->next_ = NULL;
	}

	// Case 4: In the middle

	else
	{
		pNode->next_->prev_ = pNode->prev_;
		pNode->prev_->next_ = pNode->next_;
	}

	pNode->next_ = NULL;
	pNode->prev_ = NULL;
}

/**
 *	This method is for debugging.
 *	It dumps the state of the cache to stdout.
 *	Entries are sorted by access time.
 *
 *	@return				None bytes
 */

void DataSectionCache::dumpCacheState()
{
	SimpleMutexHolder permission( accessControl_ );

	CacheNode* pNode;

	dprintf("Cached items, ordered by last access time:\n");
	dprintf("------------------------------------------\n");

	for(pNode = cacheHead_; pNode; pNode = pNode->next_)
	{
		dprintf("Name:               %s\n", pNode->path_.c_str());
		dprintf("Size (original):    %d bytes\n", pNode->bytes_);
		dprintf("Size (DataSection): %d bytes\n", pNode->dataSection_->bytes());
		dprintf("References:         %d\n", pNode->dataSection_->refCount());
		dprintf("Next: 		        %p\n", pNode->next_);
		dprintf("Prev: 		        %p\n", pNode->prev_);
		dprintf("\n");
	}

	dprintf("Total cache size: %d bytes (Max %d bytes)\n", s_currentBytes_, s_maxBytes_);
	dprintf("\n");
}

BW_END_NAMESPACE

// data_section_cache.cpp
