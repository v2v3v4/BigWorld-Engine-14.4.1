#ifndef CHUNK_SPACE_HPP
#define CHUNK_SPACE_HPP

#include "cstdmf/bw_namespace.hpp"

#ifdef MF_SERVER
#include "server_chunk_space.hpp"

BW_BEGIN_NAMESPACE

typedef ServerChunkSpace ConfigChunkSpace;

BW_END_NAMESPACE

#else
#include "client_chunk_space.hpp"

BW_BEGIN_NAMESPACE

class ClientChunkSpace;
typedef ClientChunkSpace ConfigChunkSpace;

BW_END_NAMESPACE

#endif

#include "chunk_item.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/guard.hpp"

#include "math/matrix.hpp"
#include "physics2/worldtri.hpp" // For WorldTriangle::Flags
#include "terrain/base_terrain_block.hpp"
#include "terrain/terrain_settings.hpp"

#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

//	Forward declarations relating to chunk obstacles
class CollisionCallback;
extern CollisionCallback & CollisionCallback_s_default;
class ChunkObstacle;

typedef uint32 ChunkSpaceID;

class SpaceCache;
class ChunkSpace;
class GeometryMapping;
class GeometryMappingFactory;
typedef SmartPointer<ChunkSpace> ChunkSpacePtr;



/**
 *	This class is used by addMappingAsync to notify the result of
 *	loading.
 */
class AddMappingAsyncCallback : public SafeReferenceCount 
{
public:
	enum AddMappingResult
	{
		RESULT_SUCCESS,
		RESULT_FAILURE,
		RESULT_INVALIDATED
	};

	AddMappingAsyncCallback() {}
	virtual ~AddMappingAsyncCallback() {}

	virtual void onAddMappingAsyncCompleted( 
			SpaceEntryID spaceEntryID,
			AddMappingResult result ) = 0;
};

typedef SmartPointer< AddMappingAsyncCallback > AddMappingAsyncCallbackPtr;

/**
 *	This class defines a space and maintains the chunks that live in it.
 *
 *	A space is a continuous three dimensional Cartesian medium. Each space
 *	is divided piecewise into chunks, which occupy the entire space but
 *	do not overlap. i.e. every point in the space is in exactly one chunk.
 *	Examples include: planets, parallel spaces, spacestations, 'detached'
 *	apartments / dungeon levels, etc.
 */
class ChunkSpace : public ConfigChunkSpace
{
public:
	ChunkSpace( ChunkSpaceID id );
	~ChunkSpace();

	bool hasChunksInMapping( GeometryMapping * pMapping ) const;

	typedef BW::map< SpaceEntryID, GeometryMapping * > GeometryMappings;

	GeometryMapping * addMapping( SpaceEntryID mappingID, float * matrix,
		const BW::string & path, DataSectionPtr pSettings = NULL );
	void addMappingAsync( SpaceEntryID mappingID, float * matrix,
		const BW::string & path,
		const AddMappingAsyncCallbackPtr & pCB = NULL );

	GeometryMapping * getMapping( SpaceEntryID mappingID );
	void delMapping( SpaceEntryID mappingID );
	const GeometryMappings & getMappings();

	bool isMapped() const;
	bool isMappingPending() const;

	void clear();
	bool cleared() const { return cleared_; }

	Chunk * findOutsideChunkFromPoint( const Vector3 & point );
	Chunk * findChunkFromPoint( const Vector3 & point );
	Chunk * findChunkFromPointExact( const Vector3 & point );

	Column * column( const Vector3 & point, bool canCreate = true );

	// Collision related methods
	float collide( const Vector3 & start, const Vector3 & end,
		CollisionCallback & cc = CollisionCallback_s_default ) const;

	float collide( const WorldTriangle & start, const Vector3 & end,
		CollisionCallback & cc = CollisionCallback_s_default ) const;

	const Vector3 findDropPoint( const Vector3 & position );

	bool setClosestPortalState( const Vector3 & point,
			bool isPermissive, WorldTriangle::Flags collisionFlags = 0 );

	void dumpDebug() const;

	BoundingBox gridBounds() const;

	BoundingBox subGridBounds() const;

	Chunk * guessChunk( const Vector3 & point, bool lookInside = false );

	void unloadChunkBeforeBinding( Chunk * pChunk );
	void unloadChunks() const;
	void unloadChunks( const BW::set< Chunk * >& chunkSet ) const;
	bool unloadChunk( Chunk * pChunk ) const;

	void ignoreChunk( Chunk * pChunk );
	void noticeChunk( Chunk * pChunk );

	void closestUnloadedChunk( float closest ) { closestUnloadedChunk_ = closest; }
	float closestUnloadedChunk()			   { return closestUnloadedChunk_;	}

	bool isNavigationEnabled() const					{ return isNavigationEnabled_; }

	bool validatePendingTask( BackgroundTask * pTask );

	Terrain::TerrainSettingsPtr terrainSettings() const;

	void pMappingFactory( GeometryMappingFactory * pFactory )
	{
		pMappingFactory_ = pFactory;
	}

	typedef void (*TouchFunction)( ChunkSpace & space );
	static int registerCache( TouchFunction tf );
	SpaceCache *& cache( int id )					{ return caches_[id]; }

	static DataSectionPtr getTerrainSettingsDataSection(
		DataSectionPtr spaceSettings, bool createIfDoesntExist = false );

private:
	SpaceCache * *		caches_;
	static int nextCacheID_;
	static BW::vector<TouchFunction>& touchType()
	{
		static BW::vector<TouchFunction> s_touchType;
		return s_touchType;
	};

	void recalcGridBounds();

	/**
	 *	This class is used by addMappingAsync to perform the required background
	 *	loading.
	 */
	class LoadMappingTask : public BackgroundTask
	{
	public:
		LoadMappingTask( ChunkSpacePtr pChunkSpace, SpaceEntryID mappingID,
				float * matrix, const BW::string & path,
				const AddMappingAsyncCallbackPtr & pCB );

		virtual void doBackgroundTask( TaskManager & mgr );
		virtual void doMainThreadTask( TaskManager & mgr );

	private:
		ChunkSpacePtr pChunkSpace_;

		SpaceEntryID mappingID_;
	   	Matrix matrix_;
		BW::string path_;

		DataSectionPtr pSettings_;

		AddMappingAsyncCallbackPtr pCallback_;
	};

	bool cleared_;

	Terrain::TerrainSettingsPtr	terrainSettings_;

	GeometryMappings		mappings_;
	GeometryMappingFactory * pMappingFactory_;

	float					closestUnloadedChunk_;
	BW::set< BackgroundTask * > backgroundTasks_;

	bool					isNavigationEnabled_;

	SimpleMutex				mappingsLock_;
};


/**
 *	This class is a base class for classes that implement space caches
 */
class SpaceCache
{
public:
#ifdef _RELEASE
	virtual ~SpaceCache() {}
#else
	SpaceCache();
	virtual ~SpaceCache();
#endif

	virtual void onAddMapping() {}

	static void touch( Chunk & ) {}			///< space touching this cache type

	/**
	 *	This template class should be a static member of space caches.
	 *	It takes care of the registration of the cache type and retrieval
	 *	of the cache out of the space. The template argument should be
	 *	a class type derived from SpaceCache
	 */
	template <class CacheT> class Instance
	{
	public:
		/**
		 *	Constructor.
		 */
		Instance() : id_( ChunkSpace::registerCache( &CacheT::touch ) ) {}

		/**
		 *	Access the instance of this cache in the given chunk.
		 */
		CacheT & operator()( ChunkSpace & space ) const
		{
			SpaceCache * & sc = space.cache( id_ );
			if (sc == NULL) sc = new CacheT( space );
			return static_cast<CacheT &>(*sc);
		}

		/**
		 *	Return whether or not there is an instance of this cache.
		 */
		bool exists( ChunkSpace & space ) const
		{
			return !!space.cache( id_ );
		}

		/**
		 *	Clear the instance of this cache.
		 *	Safe to call even if there is no instance.
		 */
		void clear( ChunkSpace & space ) const
		{
			SpaceCache * & sc = space.cache( id_ );
			bw_safe_delete(sc);
		}


	private:
		int id_;
	};
};

#ifdef CODE_INLINE
#include "chunk_space.ipp"
#endif

BW_END_NAMESPACE

#endif // CHUNK_SPACE_HPP
