#ifndef DATA_SECTION_CACHE_HPP
#define DATA_SECTION_CACHE_HPP

#include "datasection.hpp"
#include "cstdmf/concurrency.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

/**
 *	A cache for DataSection objects. It stores the absolute path for each
 *	entry that is cached, and maps to a smart pointer. When the cache exceeds
 *	a certain amount, entries are removed from the cache on a LRU basis.
 */	

class DataSectionCache
{
public:
	~DataSectionCache();
	
	static DataSectionCache* instance();

	DataSectionCache* setSize( int maxBytes );

	static void fini();

	/// Add an entry to the cache
	void				add( const BW::string & name,
		DataSectionPtr dataSection );
	
	/// Search the cache for an entry
	DataSectionPtr		find( const BW::string & name );
	
	/// Remove an entry from the cache (release our reference count to it)
	void				remove( const BW::string & name );

	// Clear the whole cache
	void				clear();

	/// Dump the state of the cache, for debugging
	void 				dumpCacheState();

private:
	DataSectionCache();

	/// CacheNode is used to provide a doubly-linked cache chain.
	struct CacheNode
	{
		BW::string		path_;
		DataSectionPtr	dataSection_;
		int				bytes_;
		CacheNode*		prev_;
		CacheNode*		next_;
	};
	
	typedef BW::map<BW::string, CacheNode*> DataSectionMap;
	
	DataSectionMap		map_;
	static int			s_maxBytes_;
	static int			s_currentBytes_;
	CacheNode*			cacheHead_;
	CacheNode*			cacheTail_;
	static int			s_hits_;
	static int			s_misses_;

	SimpleMutex			accessControl_;

	static DataSectionCache*	s_instance;

private:

	// Helper functions
	void purgeLRU();
	void moveToHead( CacheNode * pNode );
	void unlinkNode( CacheNode * pNode );
	
	// Prevent copying
	DataSectionCache(const DataSectionCache &);
	DataSectionCache& operator=(const DataSectionCache &);
};

BW_END_NAMESPACE

#endif // DATA_SECTION_CACHE_HPP


