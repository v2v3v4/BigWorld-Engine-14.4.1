#ifndef CHUNK_CACHE_HPP
#define CHUNK_CACHE_HPP


#include "chunk.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class ChunkProcessorManager;
class Chunk;
class UnsavedList;
class ChunkProcessors;

#ifdef EDITOR_ENABLED

BW_END_NAMESPACE

#include "invalidate_flags.hpp"

BW_BEGIN_NAMESPACE

class ChunkCleanFlags;
#endif //EDITOR_ENABLED

/**
 *	This class is a base class for classes that implement chunk caches
 */
class ChunkCache
{
public:
	ChunkCache();
	virtual ~ChunkCache();

	virtual void draw( Moo::DrawContext& drawContext );
	virtual int focus();
	virtual void bind( bool isUnbind );
	virtual bool load( DataSectionPtr chunk, DataSectionPtr cdata );

	virtual const char* id() const;

#ifdef EDITOR_ENABLED

	void onChunkChanged( InvalidateFlags flags,ChunkProcessorManager* mgr,
		EditorChunkItem* pChangedItem = NULL );

	virtual bool invalidate( ChunkProcessorManager* mgr, bool spread,
		EditorChunkItem* pChangedItem = NULL);

	virtual void saveChunk( DataSectionPtr chunk );
	virtual void saveCData( DataSectionPtr cdata );
	virtual void loadCleanFlags( const ChunkCleanFlags& cf );
	virtual void saveCleanFlags( ChunkCleanFlags& cf );

	virtual bool canCalculateNow( ChunkProcessorManager* mgr );
	virtual bool readyToCalculate( ChunkProcessorManager* mgr );

	virtual bool loadChunkForCalculate( ChunkProcessorManager* mgr );

	virtual bool requireProcessingInMainThread() const;
	virtual bool requireProcessingInBackground() const;

	virtual bool dirty();

	virtual bool isBeingCalculated() const;
	virtual bool cancelAllCalculating( ChunkProcessorManager*,
					ChunkProcessors& outUnfinishedProcessors ) { return true; }

	virtual bool recalc( ChunkProcessorManager* mgr, UnsavedList& );

	void writeBool( DataSectionPtr ds,
		const BW::string& sectionName,
		bool value );

	bool readBool( DataSectionPtr ds,
		const BW::string& sectionName,
		bool defaultValue );
protected:
	InvalidateFlags::Flag invalidateFlag_;
public:

#endif //EDITOR_ENABLE

	static void touch( Chunk & );

	/**
	 *	This template class should be a static member of chunk caches.
	 *	It takes care of the registration of the cache type and retrieval
	 *	of the cache out of the chunk. The template argument should be
	 *	a class type derived from ChunkCache
	 */
	template <class CacheT> class Instance
	{
	public:
		/**
		 *	Constructor.
		 */
		Instance( void* id = NULL )
		{
			if (id)
			{
				index_ = getTouchTypeIndex( id );

				if (index_ == INVALID_CACHE_INDEX)
				{
					index_ = ChunkCache::registerCache( &CacheT::touch );
					ChunkCache::setTouchTypeIndex( id, index_ );
				}
				else
				{
					ChunkCache::touchType()[ index_ ] = &CacheT::touch;
				}
			}
			else
			{
				index_ = getTouchTypeIndex( this );

				if (index_ == INVALID_CACHE_INDEX)
				{
					index_ = ChunkCache::registerCache( &CacheT::touch );
					ChunkCache::setTouchTypeIndex( this, index_ );
				}
			}
		}

		/**
		 *	Access the instance of this cache in the given chunk.
		 */
		CacheT & operator()( Chunk & chunk ) const
		{
			ChunkCache * & cc = chunk.cache( index_ );
			if (cc == NULL) cc = new CacheT( chunk );
			return static_cast<CacheT &>(*cc);
		}

		/**
		 *	Return whether or not there is an instance of this cache.
		 */
		bool exists( Chunk & chunk ) const
		{
			return !!chunk.cache( index_ );
		}

		/**
		 *	Clear the instance of this cache.
		 *	Safe to call even if there is no instance.
		 */
		void clear( Chunk & chunk ) const
		{
			ChunkCache * & cc = chunk.cache( index_ );
			bw_safe_delete(cc);
		}

		int index() const
		{
			return index_;
		}


	private:
		int index_;
	};

	static const int INVALID_CACHE_INDEX = -1;

	typedef void (*TouchFunction)( Chunk & chunk );

	static int registerCache( TouchFunction tf );

	static int cacheNum();

	static BW::vector<TouchFunction>& touchType()
	{
		static BW::vector<TouchFunction> s_touchType;

		return s_touchType;
	}

private:
	static int s_nextCacheIndex_;

	static BW::map<void*, int>& touchTypeIndexMap()
	{
		static BW::map<void*, int> s_touchTypeIndexMap;

		return s_touchTypeIndexMap;
	}

	static int getTouchTypeIndex( void* id );
	static void setTouchTypeIndex( void* id, int index );
};

#ifdef CODE_INLINE
#include "chunk_cache.ipp"
#endif

BW_END_NAMESPACE

#endif // CHUNK_CACHE_HPP
