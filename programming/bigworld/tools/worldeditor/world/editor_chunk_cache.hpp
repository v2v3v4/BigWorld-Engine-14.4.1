#ifndef EDITOR_CHUNK_CACHE_HPP
#define EDITOR_CHUNK_CACHE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_model.hpp"
#include "common/editor_chunk_cache_base.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_terrain.hpp"
#include "gizmo/undoredo.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

enum ChunkNotWritableReason
{
	REASON_DEFAULT,
	REASON_FILEREADONLY,
	REASON_NOTLOCKEDBYME
};

/**
 *	This class is, at the moment, a utility class.
 */
class EditorChunk
{
public:
	static ChunkPtr findOutsideChunk( const Vector3 & position,
		bool assertExistence = false );
	static int findOutsideChunks( const BoundingBox & bb,
		ChunkPtrVector & outVector, bool assertExistence = false );
	static bool outsideChunksExist( const BoundingBox & bb );

	static bool chunkWriteable( const Chunk & chunk, bool bCheckSurroundings = true,
		ChunkNotWritableReason* retReason = NULL ); 

	static bool outsideChunkWriteable( const Vector3 & position, bool mustAlreadyBeLoaded = true );
	static bool outsideChunkWriteable( int16 gridX, int16 gridZ, bool mustAlreadyBeLoaded = true );
	static bool outsideChunksWriteable( const BoundingBox & bb, bool mustAlreadyBeLoaded = true );
	static bool outsideChunksWriteableInSpace( const BoundingBox & bb );

	static bool chunkFilesReadOnly( const Chunk & chunk );
	static bool chunkIsLockedByMe( const Chunk & chunk, bool bCheckSurroundings = true );
};


// We need to specialise EditorChunkCache::instance()
class EditorChunkCache;
template <>
EditorChunkCache & ChunkCache::Instance<EditorChunkCache>::operator()( Chunk & chunk ) const;

/**
 *	This is a chunk cache that caches editor specific chunk information.
 *	It is effectively the editor extensions to the Chunk class.
 *
 *	Note: Things that want to fiddle with datasections in a chunk should
 *	either keep the one they were loaded with (if they're an item) or use
 *	the root datasection stored in this cache, as it may be the only correct
 *	version of it. i.e. you cannot go back and get stuff from the .chunk
 *	file through BWResource as the cache will likely be well stuffed, and
 *	the file may not even be there (if the scene was saved with that chunk
 *	deleted and it's since been undone). i.e. use this datasection! :)
 */
class EditorChunkCache : public EditorChunkCacheBase
{
public:
	EditorChunkCache( Chunk & chunk );
	~EditorChunkCache();

	static BW::set<Chunk*> chunks_;
	static void lock();
	static void unlock();

	virtual void draw();

	virtual bool load( DataSectionPtr pSec, DataSectionPtr pCdata );

	virtual void bind( bool isUnbind );

	void reloadBounds();

	static void touch( Chunk & chunk );

	bool edSave();
	bool edSaveCData();

	bool edTransform( const Matrix & m, bool transient = false );
	bool edTransformClone( const Matrix & m );

	void edArrive( bool fromNowhere = false );
	void edDepart();

	void edEdit( class ChunkEditor & editor );
	bool edReadOnly() const;
	static void forwardReadOnlyMark()	{	++s_readOnlyMark_;	}

	/** 
	 * If the chunk is ok with being deleted
	 */
	bool edCanDelete() const;

	/**
	 * Notification that we were broght back after a delete
	 */
	void edPostUndelete();

	/**
	 * Notification that we're about to delete the chunk
	 */
	void edPreDelete();

	/**
	 * If the chunk is currently deleted
	 */
	bool edIsDeleted() const { return deleted_; }

	/**
	 * If the chunk is about to be deleted
	 */
	bool edIsDeleting() const { return deleting_; }

	/**
	 * Notification that we were just cloned
	 */
	void edPostClone(bool keepLinks = false);

	/** If the user has this chunk locked */
	bool edIsLocked() const;

	/** If the user may modify this chunk or it's contents */
	bool edIsWriteable( bool bCheckSurroundings = true,
		ChunkNotWritableReason* retReason = NULL ) const; 

	/** Inform the terrain cache of the first terrain item in the chunk */
	void fixTerrainBlocks();

	void addInvalidSection( DataSectionPtr section );

	EditorChunkModelPtr getShellModel() const;
	BW::vector<ChunkItemPtr> staticItems() const;
	void allItems( BW::vector<ChunkItemPtr> & items ) const;

	template < typename Visitor >
	static void visitStaticItems( Chunk & chunk, Visitor & visitor )
	{
		RecursiveMutexHolder lock( chunk.chunkMutex_ );
		BW::vector< ChunkItemPtr > items = 
			EditorChunkCache::instance( chunk ).staticItems();
		for (BW::vector< ChunkItemPtr >::iterator itr = items.begin(),
			end = items.end(); itr != end; ++itr)
		{
			bool finished = visitor( *itr );
			if (finished) return;
		}
	}

	static Instance<EditorChunkCache>	instance;

protected:
	bool saveCDataInternal( DataSectionPtr ds, const BW::string& filename );

private:
	void takeOut();
	void putBack();
	bool doTransform( const Matrix & m, bool transient, bool cleanSnappingHistory );

	BW::string chunkResourceID_;
	bool			present_;		///< Is the chunk present in a file
	bool			deleted_;		///< Should the chunk have its file
	mutable bool	readOnly_;
	mutable int		readOnlyMark_;
	static int		s_readOnlyMark_;
	// The deleting flag is for chunk items that want to know if they are being
	// deleted from a chunk (shell).
	bool			deleting_;

	BW::vector<DataSectionPtr> invalidSections_;

	void updateDataSectionWithTransform();
};


class ChunkMatrixOperation : public UndoRedo::Operation
{
public:
	ChunkMatrixOperation( Chunk * pChunk, const Matrix & oldPose );
private:
	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const;

	Chunk *			pChunk_;
	Matrix			oldPose_;
};


class ChunkExistenceOperation : public UndoRedo::Operation
{
public:
	ChunkExistenceOperation( Chunk * pChunk, bool create ) :
		UndoRedo::Operation( 0 ),
		pChunk_( pChunk ),
		create_( create )
	{
		addChunk( pChunk );
	}

private:

	virtual void undo();
	virtual bool iseq( const UndoRedo::Operation & oth ) const;

	Chunk			* pChunk_;
	bool			create_;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_CACHE_HPP
