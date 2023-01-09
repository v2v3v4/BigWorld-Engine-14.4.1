#include "pch.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/vlo_manager.hpp"
#include "worldeditor/world/items/editor_chunk_model.hpp"
#include "worldeditor/world/items/editor_chunk_portal.hpp"
#include "worldeditor/world/editor_chunk_overlapper.hpp"
#include "worldeditor/world/item_info_db.hpp"
#include "worldeditor/editor/autosnap.hpp"
#include "worldeditor/editor/chunk_editor.hpp"
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/misc/cvswrapper.hpp"
#include "worldeditor/project/chunk_photographer.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "common/lighting_influence.hpp"
#include "common/bwlockd_connection.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_light.hpp"
#include "chunk/chunk_terrain.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/data_section_census.hpp"
#include "resmgr/string_provider.hpp"
#include "appmgr/options.hpp"
#include "gizmo/general_properties.hpp"
#include "moo/geometrics.hpp"
#include "moo/texture_manager.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/bw_set.hpp"

#include "worldeditor/editor/item_properties.hpp"


DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

BW_BEGIN_NAMESPACE

int EditorChunkCache::s_readOnlyMark_ = 0;


namespace
{
	/**
	 *	Find the outside chunk that includes the given world position.
	 *
	 *	@param pos		The position to get the chunk at.
	 *	@param mustAlreadyBeLoaded If true then the chunk must be loaded, if false 
	 *					then a dummy chunk is created.
	 *	@returns		The outside chunk at the given location.  If the 
	 *					position is outside of the space or if the chunk is
	 *					not loaded then NULL is returned.
	 */
	ChunkPtr getOutsideChunk(Vector3 const &pos, bool mustAlreadyBeLoaded)
	{
		BW_GUARD;

		GeometryMapping *mapping = WorldManager::instance().geometryMapping();
		BW::string chunkName = mapping->outsideChunkIdentifier(pos);
		if (!chunkName.empty())
		{
			Chunk* result = ChunkManager::instance().findChunkByName
				(
					chunkName, 
					mapping, 
					!mustAlreadyBeLoaded 
				);
			if (!mustAlreadyBeLoaded || (result && result->loaded()))
				return result;
		}
		return NULL;
	}


	/**
	 *	Find the outside chunk at the given grid coordinates.
	 *
	 *	@param gx		The x grid coordinate.
	 *	@param gz		The z grid coordinate.
	 *	@param mustAlreadyBeLoaded If true then the chunk must be loaded, if false 
	 *					then a dummy chunk is created.
	 *	@returns		The outside chunk at the given location.  If the 
	 *					position is outside of the space or if the chunk is
	 *					not loaded then NULL is returned.
	 */
	ChunkPtr getOutsideChunk(int gx, int gz, bool mustAlreadyBeLoaded)
	{
		BW_GUARD;
		ChunkSpacePtr pSpace = WorldManager::instance().geometryMapping()->pSpace();
		const float gridSize = pSpace->gridSize();
		Vector3 pos = 
			Vector3
			( 
			pSpace->gridToPoint( gx ) + gridSize*0.5f,
			0.0f, 
			pSpace->gridToPoint( gz ) + gridSize*0.5f 
			);
		return getOutsideChunk(pos, mustAlreadyBeLoaded);
	}
}


// -----------------------------------------------------------------------------
// Section: EditorChunk
// -----------------------------------------------------------------------------

/**
 *	This method finds the outside chunk at the given position if it is focussed.
 */
ChunkPtr EditorChunk::findOutsideChunk( const Vector3 & position,
	bool assertExistence )
{
	BW_GUARD;

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if (!pSpace) return NULL;

	ChunkSpace::Column * pColumn = pSpace->column( position, false );

	if (pColumn != NULL && pColumn->pOutsideChunk() != NULL)
		return pColumn->pOutsideChunk();
	else if (assertExistence)
	{
		CRITICAL_MSG( "EditorChunk::findOutsideChunk: "
			"No focussed outside chunk at (%f,%f,%f) when required\n",
			position.x, position.y, position.z );
	}

	return NULL;
}


/**
 *	This method finds all the focussed outside chunks within the given bounding
 *	box, and adds them to the input vector. The vector is cleared first.
 *	@return The count of chunks in the vector.
 */
int EditorChunk::findOutsideChunks(
		const BoundingBox & bb,
		ChunkPtrVector & outVector,
		bool assertExistence )
{
	BW_GUARD;

	outVector.clear();

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if (!pSpace) return 0;

	// go through all the columns that overlap this bounding box
	for (int x = pSpace->pointToGrid( bb.minBounds().x );
		x <= pSpace->pointToGrid( bb.maxBounds().x );
		x++)
	{
		for (int z = pSpace->pointToGrid( bb.minBounds().z );
			z <= pSpace->pointToGrid( bb.maxBounds().z );
			z++)
		{
			const Vector3 apt( pSpace->gridToPoint( x ) + pSpace->gridSize()*0.5f,
				0, pSpace->gridToPoint( z ) + pSpace->gridSize()*0.5f );

			// extract their outside chunk
			ChunkSpace::Column * pColumn = pSpace->column( apt, false );
			if (pColumn != NULL && pColumn->pOutsideChunk() != NULL)
			{
				outVector.push_back( pColumn->pOutsideChunk() );
			}
			else if (assertExistence)
			{
					CRITICAL_MSG( "EditorChunk::findOutsideChunks: "
					"No focussed outside chunk at (%f,%f,%f) when required\n",
					apt.x, apt.y, apt.z );
			}
		}
	}

	return static_cast<int>(outVector.size());
}

/**
 *	This method determines whether or not the given bounding box falls
 *	within valid, focussed outside chunks.
 */
bool EditorChunk::outsideChunksExist( const BoundingBox & bb )
{
	BW_GUARD;

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if (!pSpace) 
	{
		return false;
	}

	// go through all the columns that overlap this bounding box
	for (int x = pSpace->pointToGrid( bb.minBounds().x );
		x <= pSpace->pointToGrid( bb.maxBounds().x );
		x++)
	{
		for (int z = pSpace->pointToGrid( bb.minBounds().z );
			z <= pSpace->pointToGrid( bb.maxBounds().z );
			z++)
		{
			const Vector3 apt( pSpace->gridToPoint( x ) + pSpace->gridSize()*0.5f,
				0, pSpace->gridToPoint( z ) + pSpace->gridSize()*0.5f );

			// extract their outside chunk
			ChunkSpace::Column * pColumn = pSpace->column( apt, false );
			if (pColumn == NULL || pColumn->pOutsideChunk() == NULL)
			{
				return false;
			}
		}
	}

	return true;
}


/**
 *	This method determines whether or not the given chunk is writeable.
 *	If bCheckSurroundings is true, it also checks that we hold a lock
 *	on the surrounding chunks.
 */
bool EditorChunk::chunkWriteable( const Chunk & chunk, bool bCheckSurroundings /*= true*/,
								 ChunkNotWritableReason* retReason /*= NULL*/ )
{
	BW_GUARD;

	if (chunk.loaded())
	{
		return EditorChunkCache::instance( const_cast<Chunk &>( chunk ) )
			.edIsWriteable( bCheckSurroundings, retReason );
	}
	if (chunkFilesReadOnly( chunk ))
	{
		if (retReason)
		{
			*retReason = REASON_FILEREADONLY;
		}
		return false;
	}

	if (!chunkIsLockedByMe( chunk, bCheckSurroundings ))
	{
		if (retReason)
		{
			*retReason = REASON_NOTLOCKEDBYME;
		}
		return false;
	}

	return true;
}


/**
 *	This method determines whether or not the outside chunks at the given
 *	position exists and is writeable.  If mustAlreadyBeLoaded is true
 *	then the chunks must also be loaded too, if false, then the chunk does
 *	not have to be loaded yet.
 */
bool EditorChunk::outsideChunkWriteable( const Vector3 & position, bool mustAlreadyBeLoaded )
{
	BW_GUARD;

	ChunkPtr chunk = getOutsideChunk( position, mustAlreadyBeLoaded );
	if (chunk == NULL)
	{
		return false;
	}
	return chunkWriteable( *chunk, mustAlreadyBeLoaded );
}

/**
 *	This method determines whether or not the outside chunks at the given
 *	grid exists and is writeable.  If mustAlreadyBeLoaded is true
 *	then the chunk must also be loaded too, if false, then the chunk does
 *	not have to be loaded yet.
 */
bool EditorChunk::outsideChunkWriteable( int16 gridX, int16 gridZ, bool mustAlreadyBeLoaded )
{
	BW_GUARD;

	ChunkPtr chunk = getOutsideChunk( gridX, gridZ, mustAlreadyBeLoaded );
	if (chunk == NULL)
	{
		return false;
	}
	return chunkWriteable( *chunk, mustAlreadyBeLoaded );
}

/**
 *	This method determines whether or not all the outside chunks in the given
 *	bounding box exist and are writeable.  If mustAlreadyBeLoaded is true
 *	then the chunks must also be loaded too, if false, then the chunk does
 *	not have to be loaded yet.
 */
bool EditorChunk::outsideChunksWriteable( const BoundingBox & bb, bool mustAlreadyBeLoaded )
{
	BW_GUARD;

	ChunkSpacePtr pSpace = WorldManager::instance().geometryMapping()->pSpace();
	// go through all the columns that overlap this bounding box
	for 
	(
		int x = pSpace->pointToGrid( bb.minBounds().x );
		x <= pSpace->pointToGrid( bb.maxBounds().x );
		++x
	)
	{
		for 
		(
			int z = pSpace->pointToGrid( bb.minBounds().z );
			z <= pSpace->pointToGrid( bb.maxBounds().z );
			++z
		)
		{
			if (!outsideChunkWriteable( x, z, mustAlreadyBeLoaded ))
				return false;
		}
	}

	return true;
}

/**
 *	This method determines whether or not all the outside chunks in the given
 *	bounding box exist and are writeable and are already loaded into space.  
 */
bool EditorChunk::outsideChunksWriteableInSpace( const BoundingBox & bb )
{
	BW_GUARD;

	ChunkSpacePtr pSpace = WorldManager::instance().geometryMapping()->pSpace();
	// go through all the columns that overlap this bounding box
	for 
	(
		int x = pSpace->pointToGrid( bb.minBounds().x );
		x <= pSpace->pointToGrid( bb.maxBounds().x );
		++x
	)
	{
		for 
		(
			int z = pSpace->pointToGrid( bb.minBounds().z );
			z <= pSpace->pointToGrid( bb.maxBounds().z );
			++z
		)
		{
			ChunkPtr chunk = getOutsideChunk( x, z, true );
			if (chunk == NULL)
				return false;
			if (!chunk->isBound())
				return false;
			if (!chunkWriteable( *chunk ))
				return false;
		}
	}

	return true;
}

/*static*/ bool EditorChunk::chunkFilesReadOnly( const Chunk & chunk )
{
	BW_GUARD;

	BW::string prefix = BWResource::resolveFilename(
		WorldManager::instance().geometryMapping()->path() + chunk.identifier() );
	BW::wstring wchunk, wcdata;
	bw_utf8tow( prefix + ".chunk", wchunk );
	bw_utf8tow( prefix + ".cdata", wcdata );
	DWORD chunkAttr = GetFileAttributes( wchunk.c_str() );
	DWORD cdataAttr = GetFileAttributes( wcdata.c_str() );
	if( chunkAttr != INVALID_FILE_ATTRIBUTES && ( chunkAttr & FILE_ATTRIBUTE_READONLY ) )
		return true;
	else if( cdataAttr != INVALID_FILE_ATTRIBUTES && ( cdataAttr & FILE_ATTRIBUTE_READONLY ) )
		return true;
	else
		return false;
}

namespace
{
	int worldToGridCoord( float w, float gridSize )
	{
		int g = int(w / gridSize);

		if (w < 0.f)
			g--;

		return g;
	}
}


/*static*/ bool EditorChunk::chunkIsLockedByMe( const Chunk & chunk, bool bCheckSurroundings /*= true*/ )
{
	BWLock::BWLockDConnection& conn = WorldManager::instance().connection();

	if (!conn.enabled())
	{
		return true;
	}

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	const BoundingBox& bb = chunk.boundingBox();
	if (chunk.isOutsideChunk())
	{
		// Use bb.centre, as the chunk may not be bound, which means it's own
		// centre won't be valid
		Vector3 centre = bb.centre();

		centre = dirMap->invMapper().applyPoint( centre );
		const float gridSize = dirMap->pSpace()->gridSize();
		int gridX = worldToGridCoord( centre.x, gridSize );
		int gridZ = worldToGridCoord( centre.z, gridSize );

		if ( !conn.isLockedByMe( gridX, gridZ ) )
			return false;

		if (bCheckSurroundings)
		{
			for (int x = -conn.xExtent(); x < conn.xExtent() + 1; ++x)
			{
				for (int y = -conn.zExtent(); y < conn.zExtent() + 1; ++y)
				{
					int curX = gridX + x;
					int curY = gridZ + y;

					if (!conn.isLockedByMe(curX,curY))
						return false;
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			Vector3 corner(
				i / 2 ? bb.minBounds().x : bb.maxBounds().x,
				0.f,
				i % 2 ? bb.minBounds().z : bb.maxBounds().z
				);

			GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
			corner = dirMap->invMapper().applyPoint( corner );
			const float gridSize = dirMap->pSpace()->gridSize();

			int gridX = worldToGridCoord( corner.x, gridSize );
			int gridZ = worldToGridCoord( corner.z, gridSize );

			if ( !conn.isLockedByMe( gridX, gridZ ) )
				return false;

			if (bCheckSurroundings)
			{
				for (int x = -conn.xExtent(); x < conn.xExtent() + 1; ++x)
				{
					for (int y = -conn.zExtent(); y < conn.zExtent() + 1; ++y)
					{
						int curX = gridX + x;
						int curY = gridZ + y;

						if (!conn.isLockedByMe(curX,curY))
							return false;
					}
				}
			}
		}
	}

	return true;
}


// -----------------------------------------------------------------------------
// Section: ChunkMatrixOperation
// -----------------------------------------------------------------------------

/**
 *	Cosntructor
 */
ChunkMatrixOperation::ChunkMatrixOperation( Chunk * pChunk, const Matrix & oldPose ) :
		UndoRedo::Operation( size_t(typeid(ChunkMatrixOperation).name()) ),
		pChunk_( pChunk ),
		oldPose_( oldPose )
{
	BW_GUARD;

	addChunk( pChunk_ );
}

void ChunkMatrixOperation::undo()
{
	BW_GUARD;

	// first add the current state of this block to the undo/redo list
	UndoRedo::instance().add( new ChunkMatrixOperation(
		pChunk_, pChunk_->transform() ) );

	// now change the matrix back
	EditorChunkCache::instance( *pChunk_ ).edTransform( oldPose_ );
}

bool ChunkMatrixOperation::iseq( const UndoRedo::Operation & oth ) const
{
	return pChunk_ ==
		static_cast<const ChunkMatrixOperation&>( oth ).pChunk_;
}



// -----------------------------------------------------------------------------
// Section: ChunkMatrix
// -----------------------------------------------------------------------------


/**
 *	This class handles the internals of moving a chunk around
 */
class ChunkMatrix : public MatrixProxy
{
public:
	ChunkMatrix( Chunk * pChunk );
	~ChunkMatrix();

	virtual void getMatrix( Matrix & m, bool world );
	virtual void getMatrixContext( Matrix & m );
	virtual void getMatrixContextInverse( Matrix & m );
	virtual bool setMatrix( const Matrix & m );

	virtual void recordState();
	virtual bool commitState( bool revertToRecord, bool addUndoBarrier );

	virtual bool hasChanged();

private:
	Chunk *			pChunk_;
	Matrix			origPose_;
	Matrix			curPose_;
};


/**
 *	Constructor.
 */
ChunkMatrix::ChunkMatrix( Chunk * pChunk ) :
	pChunk_( pChunk ),
	origPose_( Matrix::identity ),
	curPose_( Matrix::identity )
{
}

/**
 *	Destructor
 */
ChunkMatrix::~ChunkMatrix()
{
}


void ChunkMatrix::getMatrix( Matrix & m, bool world )
{
	m = pChunk_->transform();
}

void ChunkMatrix::getMatrixContext( Matrix & m )
{
	m = Matrix::identity;
}

void ChunkMatrix::getMatrixContextInverse( Matrix & m )
{
	m = Matrix::identity;
}

bool ChunkMatrix::setMatrix( const Matrix & m )
{
	BW_GUARD;

	curPose_ = m;
	return EditorChunkCache::instance( *pChunk_ ).edTransform( m, true );
}

void ChunkMatrix::recordState()
{
	BW_GUARD;

	origPose_ = pChunk_->transform();
	curPose_ = pChunk_->transform();
}

bool ChunkMatrix::commitState( bool revertToRecord, bool addUndoBarrier )
{
	BW_GUARD;

	// reset the transient transform first regardless of what happens next
	EditorChunkCache::instance( *pChunk_ ).edTransform( origPose_, true );

	// ok, see if we're going ahead with this
	if (revertToRecord)
		return false;

	// if we're not reverting check a few things
	bool okToCommit = true;
	{
		BoundingBox spaceBB(ChunkManager::instance().cameraSpace()->gridBounds());
		BoundingBox chunkBB(pChunk_->localBB());
		chunkBB.transformBy(curPose_);
		if ( !(spaceBB.intersects( chunkBB.minBounds() ) &&
			spaceBB.intersects( chunkBB.maxBounds() )) )
		{
			okToCommit = false;
		}

		// make sure it's not an immovable outside chunk
		//  (this test probably belongs somewhere higher)
		if (pChunk_->isOutsideChunk())
		{
			okToCommit = false;
		}
	}

	// add the undo operation for it
	UndoRedo::instance().add(
		new ChunkMatrixOperation( pChunk_, origPose_ ) );

	ChunkItemPtr shell = EditorChunkCache::instance(*pChunk_).getShellModel();

	if (shell)
	{
		shell->edPostModify();
	}

	// set the barrier with a meaningful name
	if (addUndoBarrier)
	{
		UndoRedo::instance().barrier(
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/MOVE_CHUNK", pChunk_->identifier() ), false );
		// TODO: Don't always say 'Move ' ...
		//  figure it out from change in matrix
	}

	// check here, so push on an undo for multiselect
	if ( !okToCommit )
		return false;

	// and finally set the matrix permanently
	return EditorChunkCache::instance( *pChunk_ ).edTransform( curPose_, false );
}


bool ChunkMatrix::hasChanged()
{
	return !(origPose_ == pChunk_->transform());
}


// -----------------------------------------------------------------------------
// Section: EditorChunkCache
// -----------------------------------------------------------------------------
BW::set<Chunk*> EditorChunkCache::chunks_;
static SimpleMutex chunksMutex;
static bool s_watchersInited = false;
static bool s_watchersDrawVlos = false;


void EditorChunkCache::lock()
{
	chunksMutex.grab();
}

void EditorChunkCache::unlock()
{
	chunksMutex.give();
}

/**
 *	Constructor
 */
EditorChunkCache::EditorChunkCache( Chunk & chunk ) :
	EditorChunkCacheBase( chunk ),
	present_( true ),
	deleted_( false ),
	deleting_( false ),
	readOnly_( true ),
	readOnlyMark_( s_readOnlyMark_ - 1 ) 
{
	BW_GUARD;

	SimpleMutexHolder permission( chunksMutex );
	chunks_.insert( &chunk );
	chunkResourceID_ = chunk_.resourceID();
	if ( !s_watchersInited )
	{
		s_watchersInited = true;
		MF_WATCH(
			"Chunks/Very Large Objects/Show VLO References",
			s_watchersDrawVlos, 
			Watcher::WT_READ_WRITE,
			"Highlight chunks with VLO references?" );
	}
}

EditorChunkCache::~EditorChunkCache()
{
	BW_GUARD;

	SimpleMutexHolder permission( chunksMutex );
	chunks_.erase( chunks_.find( &chunk_ ) );
	// Make sure next time the chunk is loaded, it'll be loaded from disk,
	// because the editor changes the chunk's data section in memory while
	// editing.
	BWResource::instance().purge( chunkResourceID_ );
}


void EditorChunkCache::draw()
{
	BW_GUARD;

	if ( WorldManager::instance().drawSelection() )
		return; // don't draw anything if doing frustum drag select.

	// draw watchers
	if ( s_watchersDrawVlos && pChunkSection_ && pChunkSection_->openSection( "vlo" ) )
	{
		// This watcher shows a red bounding box if the chunk has a VLO.
		Moo::rc().push();

		BoundingBox bb;
		if ( chunk_.isOutsideChunk() )
		{
			bb = chunk_.visibilityBox();
			chunk_.nextVisibilityMark();
			Moo::rc().world( Matrix::identity );
			bb.expandSymmetrically( -0.5f, 0.1f, -0.5f );
		}
		else
		{
			bb = chunk_.localBB();
			Moo::rc().world( chunk_.transform() );
		}
		Moo::rc().setVertexShader( NULL );
		Moo::rc().setPixelShader( NULL );
		Moo::rc().setTexture( 0, NULL );
		Moo::rc().setTexture( 1, NULL );

		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

		// draw using the colour, offset and tiling values.
		Geometrics::wireBox( bb, 0xFFFF0000 );

		Moo::rc().pop();
	}
}


/**
 *	Load this chunk. We just save the data section pointer
 */
bool EditorChunkCache::load( DataSectionPtr pSec, DataSectionPtr cdata )
{
	BW_GUARD;

	bool result = this->EditorChunkCacheBase::load( pSec, cdata );

	// Remove the sections marked invalid from the load.
	BW::vector<DataSectionPtr>::iterator it = invalidSections_.begin();
	for (;it!=invalidSections_.end();it++)
	{
		pChunkSection_->delChild( (*it) );
	}
	invalidSections_.clear();

	return result;
}

void EditorChunkCache::addInvalidSection( DataSectionPtr section )
{
	BW_GUARD;

	invalidSections_.push_back( section );
}

void EditorChunkCache::bind( bool isUnbind )
{
	BW_GUARD;

	// Mark us as dirty if we weren't brought fully up to date in a previous session
	if( !isUnbind )
		WorldManager::instance().onLoadChunk( &chunk_ );
	else
		WorldManager::instance().onUnloadChunk( &chunk_ );

	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
	BW::vector<ChunkItemPtr>::iterator i = chunk_.selfItems_.begin();
	for (; i != chunk_.selfItems_.end(); ++i)
		(*i)->edChunkBind();
}

/**
 *	Reload the bounds of this chunk.
 */
void EditorChunkCache::reloadBounds()
{
	BW_GUARD;

	Matrix xform = chunk_.transform();

	this->takeOut();

	Chunk* pChunk = &chunk_;

	// Remove the portal items, as they refer to the boundary objects we're
	// about to delete
	{
		RecursiveMutexHolder lock( chunk_.chunkMutex_ );
		for (int i = static_cast<int>(chunk_.selfItems_.size()-1); i >= 0; i--)
		{
			DataSectionPtr ds = chunk_.selfItems_[i]->pOwnSect();
			if (ds && ds->sectionName() == "portal")
				chunk_.delStaticItem( chunk_.selfItems_[i] );
		}
	}

	chunk_.bounds_.clear();
	chunk_.joints_.clear();

	{
		RecursiveMutexHolder lock( chunk_.chunkMutex_ );
		chunk_.selfItems_.front()->edBounds( chunk_.localBB_ );
	}
	chunk_.boundingBox_ = chunk_.localBB_;
	chunk_.boundingBox_.transformBy( chunk_.transform() );
	chunk_.boundingBox_.transformBy( chunk_.pMapping_->mapper() );

	chunk_.formBoundaries( pChunkSection_ );

	chunk_.transform( xform );
	updateDataSectionWithTransform();

	// ensure the focus grid is up to date
	ChunkManager::instance().camera( Moo::rc().invView(),
		ChunkManager::instance().cameraSpace() );

	ChunkPyCache::instance( chunk_ ).createPortalsAgain();

	this->putBack();
}

/**
 *	Touch this chunk. We make sure there's one of us in every chunk.
 */
void EditorChunkCache::touch( Chunk & chunk )
{
	BW_GUARD;

	EditorChunkCache::instance( chunk );
}


/**
 *	Save this chunk and any items in it back to the XML file
 */
bool EditorChunkCache::edSave()
{
	BW_GUARD;

	if (!pChunkSection_)
	{
		WorldManager::instance().addError( &chunk_, NULL,
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/SAVE_CHUNK_WITHOUT_DATASECTION", chunk_.identifier() ).c_str() );
		return false;
	}

	if (!edIsLocked())
	{
		WorldManager::instance().addError( &chunk_, NULL,
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/SAVE_CHUNK_WITHOUT_LOCK", chunk_.identifier() ).c_str() );
		return false;
	}

	if (edReadOnly())
	{
		WorldManager::instance().addError( &chunk_, NULL,
			LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/SAVE_CHUNK_READONLY", chunk_.identifier() ).c_str() );
		return false;
	}

	// figure out what resource this chunk lives in
	BW::string resourceID = chunk_.resourceID();

	// see if we're deleting it
	if (deleted_ && present_)
	{
		// delete the resource
		WorldManager::instance().eraseAndRemoveFile( resourceID );

		// also check for deletion of the corresponding .cdata file
		BW::string binResourceID = chunk_.binFileName();
		if (BWResource::fileExists( binResourceID ))
		{
			WorldManager::instance().eraseAndRemoveFile( binResourceID );
		}

		// record that it's not here
		present_ = false;
		return true;
	}
	// see if we deleted it in the same session we created it
	else if (deleted_ && !present_)
	{
		return true;
	}
	// see if we're creating it
	else if (!deleted_ && !present_)
	{
		// it'll get saved to the right spot below

		// the data section cache and census will be well out of whack,
		// but that's OK because everything should be using our own
		// stored datasection variable and bypassing those.

		// record that it's here
		present_ = true;
	}

	pChunkSection_->deleteSections( "processed" );

	// first rewrite the boundary information
	//  (due to portal connection changes, etc)

	// delete all existing sections
	pChunkSection_->deleteSections( "boundary" );

	// give the items a chance to save any changes
	{
		RecursiveMutexHolder lock( chunk_.chunkMutex_ );
		BW::vector<ChunkItemPtr>::iterator i = chunk_.selfItems_.begin();
		for (; i != chunk_.selfItems_.end(); ++i)
			(*i)->edChunkSave();
	}

	// update the bounding box and transform
	updateDataSectionWithTransform();

	// if we don't have a .terrain file, make sure to cvs remove it
	if (!ChunkTerrainCache::instance( chunk_ ).pTerrain() && chunk_.isOutsideChunk())
	{
		// TODO: This code seems to be outdated. Needs reviewing. 
		BW::string terrainResource = chunk_.mapping()->path() +
			chunk_.identifier() + ".cdata/terrain";
		Terrain::BaseTerrainBlock::terrainVersion( terrainResource );

		if (BWResource::fileExists( terrainResource ))
			WorldManager::instance().eraseAndRemoveFile( terrainResource );
	}

	// now save out the datasection to the file
	//  (with any changes made by items to themselves)

	bool add = !BWResource::fileExists( resourceID );

	if (add)
		CVSWrapper( WorldManager::instance().getCurrentSpace() ).addFile( chunk_.identifier() + ".chunk", false, false );

	pChunkSection_->save( resourceID );

	if (add)
		CVSWrapper( WorldManager::instance().getCurrentSpace() ).addFile( chunk_.identifier() + ".chunk", false, false );

	return EditorChunkCacheBase::edSave();
}


/*
 *	Save the binary data, such as lighting to the .cdata file
 */
bool EditorChunkCache::edSaveCData()
{
	BW_GUARD;

	DataSectionPtr cData = this->pCDataSection();

	RecursiveMutexHolder lock( chunk_.chunkMutex_ ); 
	BW::vector<ChunkItemPtr>::iterator i = chunk_.selfItems_.begin();
	for (; i != chunk_.selfItems_.end(); ++i) 
	{ 
		(*i)->edChunkSaveCData( cData ); 
	} 

	return EditorChunkCacheBase::edSaveCData(); 
} 


/* 
*  WorldEditor specific implementation of saving the cdata DataSection. 
*/ 
bool EditorChunkCache::saveCDataInternal( DataSectionPtr ds,  
										 const BW::string& filename ) 
{ 
	BW_GUARD; 

	const BW::string fileName = chunk_.binFileName(); 
	bool add = !BWResource::fileExists( fileName ); 

	if (add) 
	{ 
		// Just in case its been deleted without cvs knowledge 
		CVSWrapper( WorldManager::instance().getCurrentSpace() ). 
			addFile( chunk_.identifier() + ".cdata", true, false ); 
	} 

	if (!ds->save( fileName )) 
	{ 
		return false; 
	} 

	if (add) 
	{ 
		// Let cvs know about the file 
		CVSWrapper( WorldManager::instance().getCurrentSpace() ). 
			addFile( chunk_.identifier() + ".cdata", true, false ); 
	} 

	return true;
}

/**
 *	Change the transform of this chunk, either transiently or permanently,
 *  either clear snapping history or not
 */
bool EditorChunkCache::doTransform( const Matrix & m, bool transient, bool cleanSnappingHistory )
{
	BW_GUARD;

	// For chunk items whose position is absolute, store their position for
	// later use if they belong to the chunk. Useful for VLOs that fit entirely
	// inside a shell.
	Matrix invChunkTransform = chunk_.transform();
	invChunkTransform.invert();
	BW::set<ChunkItemPtr> itemsToMoveManually;
	BW::map<ChunkItemPtr, Matrix> itemsMatrices;
	for( Chunk::Items::iterator iter = chunk_.selfItems_.begin();
		iter != chunk_.selfItems_.end(); ++iter )
	{
		if( !(*iter)->edIsPositionRelativeToChunk() &&
			 (*iter)->edBelongToChunk() )
		{
			itemsToMoveManually.insert( *iter );
			itemsMatrices[ *iter ] = (*iter)->edTransform();
		}
	}

	// if it's transient that's easy
	if (transient)
	{
		BoundingBox chunkBB = chunk_.localBB();
		chunkBB.transformBy( m );

		BoundingBox spaceBB(ChunkManager::instance().cameraSpace()->gridBounds());

		if ( !(spaceBB.intersects( chunkBB.minBounds() ) &&
			spaceBB.intersects( chunkBB.maxBounds() )) )
			return false;

		chunk_.transformTransiently( m );
		chunk_.syncInit();
		// Move items that need to be moved manually.
		for( BW::set<ChunkItemPtr>::iterator iter = itemsToMoveManually.begin();
			iter != itemsToMoveManually.end(); ++iter )
		{
			(*iter)->edTransform( itemsMatrices[ *iter ], true );
		}

		return true;
	}

	if( cleanSnappingHistory )
		clearSnapHistory();

	// check that our source and destination are both loaded and writeable
	// (we are currently limited to movements within the focus grid...)
	if (!EditorChunk::outsideChunksWriteable( chunk_.boundingBox() ))
		return false;


	// ok, let's do the whole deal then

	ChunkSpacePtr pSpace = WorldManager::instance().geometryMapping()->pSpace();

	int oldLeft = pSpace->pointToGrid( chunk_.boundingBox().minBounds().x );
	int oldTop = pSpace->pointToGrid( chunk_.boundingBox().minBounds().z );


	BoundingBox newbb = chunk_.localBB();
	newbb.transformBy( m );
	if (!EditorChunk::outsideChunksWriteableInSpace( newbb ))
		return false;

	int newLeft = pSpace->pointToGrid( newbb.minBounds().x );
	int newTop = pSpace->pointToGrid( newbb.minBounds().z );

	WorldManager::instance().connection().linkPoint( oldLeft, oldTop, newLeft, newTop );

	// Disable updating references while moving, so items moved manually don't
	// get tossed out when the 
	VLOManager::instance()->enableUpdateChunkReferences( false );

	// take it out of this space
	this->takeOut();

	// move it
	chunk_.transform( m );

	updateDataSectionWithTransform();


	// put it back in the space
	this->putBack();

	// Move items that need to be moved manually.
	for( BW::set<ChunkItemPtr>::iterator iter = itemsToMoveManually.begin();
		iter != itemsToMoveManually.end(); ++iter )
	{
		(*iter)->edTransform( itemsMatrices[ *iter ], false );
	}

	// Update VLO references and turn on reference check on load again.
	VLOManager::instance()->enableUpdateChunkReferences( true );
	VLOManager::instance()->updateChunkReferences( &chunk_ );
	chunk_.syncInit();


	return true;
}

/**
 *	Change the transform of this chunk, either transiently or permanently,
 */
bool EditorChunkCache::edTransform( const Matrix & m, bool transient )
{
	BW_GUARD;

	return doTransform( m, transient, !transient );
}

/**
 *	Change the transform of this chunk, called from snapping functions
 */
bool EditorChunkCache::edTransformClone( const Matrix & m )
{
	BW_GUARD;

	return doTransform( m, false, false );
}

/** Write the current transform out to the datasection */
void EditorChunkCache::updateDataSectionWithTransform()
{
	BW_GUARD;

	pChunkSection_->delChild( "transform" );
	pChunkSection_->delChild( "boundingBox" );
	if( !chunk_.isOutsideChunk() )
	{
		pChunkSection_->writeMatrix34( "transform", chunk_.transform() );
		DataSectionPtr pDS = pChunkSection_->newSection( "boundingBox" );
		pDS->writeVector3( "min", chunk_.boundingBox().minBounds() );
		pDS->writeVector3( "max", chunk_.boundingBox().maxBounds() );
	}
}


/**
 *	This method is called when a chunk arrives on the scene.
 *	It saves out a file for it from its data section (assumed to already
 *	be in pChunkSection_) then binds it into its space.
 */
void EditorChunkCache::edArrive( bool fromNowhere )
{
	BW_GUARD;

	// clear the present flag if this is a brand new chunk
	if (fromNowhere) present_ = false;

	// clear the delete on save flag
	deleted_ = false;
	deleting_ = false;

	// flag the chunk as dirty
	WorldManager::instance().changedChunk( &chunk_ );

	// and add it back in to the space
	this->putBack();

	// We need to do this, as the chunk may be transformed before
	// being added (ie, when creating it), but we can't call edTransform
	// before edArrive, thus we simply save the transform here
	updateDataSectionWithTransform();


	// We also need to put this here for a hack when creating multiple
	// chunks in a single frame (ie, when undoing a delete operation)
	// otherwise the portals won't be connected
	ChunkManager::instance().camera( Moo::rc().invView(), ChunkManager::instance().cameraSpace() );
}

/**
 *	This method is called when a chunk departs from the scene.
 */
void EditorChunkCache::edDepart()
{
	BW_GUARD;

	// take it out of the space
	this->takeOut();

	// set the chunk to delete on save
	deleted_ = true;
	deleting_ = false;

	// flag the chunk as dirty
	WorldManager::instance().changedChunk( &chunk_ );
}

/**
 * Check that all our items are cool with being deleted
 */
bool EditorChunkCache::edCanDelete() const
{
	BW_GUARD;

	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
	BW::vector<ChunkItemPtr>::const_iterator i = chunk_.selfItems_.begin();
	for (; i != chunk_.selfItems_.end(); ++i)
		if (!(*i)->edCanDelete())
			return false;

	return true;
}


/**
 * Put items back into the item info DB
 */
void EditorChunkCache::edPostUndelete()
{
	BW_GUARD;

	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
    BW::vector<ChunkItemPtr> &items = chunk_.selfItems_;

	for (BW::vector<ChunkItemPtr>::iterator i = items.begin();
        i != items.end(); ++i)
    {
		// temporarily take it out, and the put it back in so it adds itself
		// to the item info DB if necessary.
		(*i)->toss( NULL );
		(*i)->toss( &chunk_ );
    }
}


/**
 * Inform our items that they'll be deleted
 */
void EditorChunkCache::edPreDelete()
{
	BW_GUARD;

    // We cannot simply iterate through selfItems_ and call edPreDelete on 
    // each.  This is because some items (such as patrol path nodes) can delete
    // items (such as links) in selfItems_.  Iterating through selfItems_ then
    // becomes invalid.  Instead we create a second copy of selfItems_ and 
    // iterate through it.  For each item we check that the item is still in 
    // selfItems_ before calling edPreDelete.
	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
    BW::vector<ChunkItemPtr> &items = chunk_.selfItems_;
    BW::vector<ChunkItemPtr> origItems = items;

	// Set this flag to true so chunk items can know if they are being deleted
	// from a shell or not.
	deleting_ = true;

	for (BW::vector<ChunkItemPtr>::iterator i = origItems.begin();
        i != origItems.end(); ++i)
    {
        if (std::find(items.begin(), items.end(), *i) != items.end())
		{
            (*i)->edPreDelete();

			// We need linkable objects to be deleted when the shell is deleted
			// since the linker manager relies on their tossRemove method being
			// called.  The undo redo recreates the items if needed.
			if ((*i)->isEditorEntity() || (*i)->isEditorUserDataObject())
			{
				// delete it now
				chunk_.delStaticItem( (*i) );

				// set up an undo which creates it
				UndoRedo::instance().add(
					new LinkerExistenceOperation( (*i), &chunk_ ) );
			}
			else
			{
				// just take out it of the item info db
				ItemInfoDB::instance().toss( (*i).get(), false );
			}
		}
    }
}

void EditorChunkCache::edPostClone(bool keepLinks)
{
	BW_GUARD;

	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
	if (keepLinks)
	{
		BW::vector<ChunkItemPtr>::iterator i = chunk_.selfItems_.begin();
		for (; i != chunk_.selfItems_.end(); ++i)
		{
			if ( ((*i)->edDescription() != "marker") &&
				((*i)->edDescription() != "marker cluster") &&
				((*i)->edDescription() != "patrol node") )
			{
				(*i)->edPostClone( NULL );
			}
		}
	}
	else
	{
		BW::vector<ChunkItemPtr>::iterator i = chunk_.selfItems_.begin();
		for (; i != chunk_.selfItems_.end(); ++i)
			(*i)->edPostClone( NULL );
	}
}

/**
 *	This method takes a chunk out of its space
 */
void EditorChunkCache::takeOut()
{
	BW_GUARD;

	// flag all chunks it's connected to as dirty
	for (Chunk::piterator pit = chunk_.pbegin(); pit != chunk_.pend(); pit++)
	{
		if (pit->hasChunk())
		{
			// should not be in bound portals list if it's not bound!
			MF_ASSERT( pit->pChunk->isBound() );

			WorldManager::instance().changedChunk( pit->pChunk );
		}
	}

	// go through all the outside chunks we overlap
	ChunkPtrVector outsideChunks;
	EditorChunk::findOutsideChunks( chunk_.boundingBox(), outsideChunks, true );
	for (uint i = 0; i < outsideChunks.size(); i++)
	{
		// delete the overlapper item pointing to it (if present)
		EditorChunkOverlappers::instance( *outsideChunks[i] ).cut( &chunk_ );
	}

	// signal to stop all the processors in the processor thread
	ChunkProcessors unfinishedProcessors;
	for ( int i = 0; i < ChunkCache::cacheNum(); ++i )
	{
		if ( ChunkCache* cc = chunk_.cache( i ) )
		{
			cc->cancelAllCalculating( 
				&ChunkProcessorManager::instance(),
				unfinishedProcessors );
		}
	}

	chunk_.unbind( true );

	// ensure the focus grid is up to date
	ChunkManager::instance().camera( Moo::rc().invView(),
		ChunkManager::instance().cameraSpace() );
}

/**
 *	This method puts a chunk back in its space
 */
void EditorChunkCache::putBack()
{
	BW_GUARD;

	// bind it to its new position (a formative bind)
	chunk_.bind( true );

	// go through all the outside chunks we overlap
	ChunkPtrVector outsideChunks;
	EditorChunk::findOutsideChunks( chunk_.boundingBox(), outsideChunks, true );
	for (uint i = 0; i < outsideChunks.size(); i++)
	{
		// create an overlapper item pointing to it
		EditorChunkOverlappers::instance( *outsideChunks[i] ).form( &chunk_ );
	}

	// flag all the new connections as dirty too
	for (Chunk::piterator pit = chunk_.pbegin(); pit != chunk_.pend(); pit++)
	{
		if (pit->hasChunk())
		{
			// should not be in bound portals list if it's not bound!
			MF_ASSERT( pit->pChunk->isBound() );

			WorldManager::instance().changedChunk( pit->pChunk );
		}
	}

	// ensure the focus grid is up to date
	ChunkManager::instance().camera( Moo::rc().invView(),
		ChunkManager::instance().cameraSpace() );
}


/**
 *	Add the properties of this chunk to the given editor
 */
void EditorChunkCache::edEdit( ChunkEditor & editor )
{
	BW_GUARD;

	if (this->getShellModel() && this->getShellModel()->edFrozen())
		return;

	STATIC_LOCALISE_NAME( s_identifier, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/IDENTIFIER" );
	STATIC_LOCALISE_NAME( s_position, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/POSITION" );
	STATIC_LOCALISE_NAME( s_rotation, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/ROTATION" );
	STATIC_LOCALISE_NAME( s_shellName, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/SHELL_NAME" );
	STATIC_LOCALISE_NAME( s_reflectionVisible, "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK/REFLECTION_VISIBLE" );

	editor.addProperty( new StaticTextProperty( s_identifier,
		new ConstantDataProxy<StringProxy>( chunk_.identifier() ) ) );

	MatrixProxy * pMP = new ChunkMatrix( &chunk_ );
	editor.addProperty( new GenPositionProperty( s_position, pMP ) );
	editor.addProperty( new GenRotationProperty( s_rotation, pMP ) );

	EditorChunkModelPtr shell = getShellModel();

	if (shell)
	{
		editor.addProperty( new StaticTextProperty( s_shellName,
				new ConstantDataProxy<StringProxy>( shell->getModelName() ) ) );

		editor.addProperty( new GenBoolProperty( s_reflectionVisible,
			new AccessorDataProxy< EditorChunkModel, BoolProxy >(
			shell.get(), "reflectionVisible",
			&EditorChunkModel::getReflectionVis,
			&EditorChunkModel::setReflectionVis ) ) );


		shell->edCommonEdit( editor );
	}
}

/**
 *	Return if one of the chunk files is readOnly
 */
bool EditorChunkCache::edReadOnly() const
{
	BW_GUARD;

	if( readOnlyMark_ != s_readOnlyMark_ )
	{
		readOnlyMark_ = s_readOnlyMark_;
		readOnly_ = EditorChunk::chunkFilesReadOnly( chunk_ );
	}
	return readOnly_;
}


/**
 *	Return the first static item
 *	(for internal chunks, this should be the shell model)
 */
EditorChunkModelPtr EditorChunkCache::getShellModel() const
{
	BW_GUARD;

	MF_ASSERT( !chunk_.isOutsideChunk() );
	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
	if (chunk_.selfItems_.empty() ||
		!chunk_.selfItems_.front()->isShellModel()) 
		return NULL;

	return (EditorChunkModel*)chunk_.selfItems_.front().get();
}

/**
 * Return all chunk items in the chunk
 */
BW::vector<ChunkItemPtr> EditorChunkCache::staticItems() const
{
	BW_GUARD;

	return chunk_.selfItems_;
}

/**
 *	Get all the items in this chunk
 */
void EditorChunkCache::allItems( BW::vector<ChunkItemPtr> & items ) const
{
	BW_GUARD;

	items.clear();
	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
	items = chunk_.selfItems_;
	items.insert( items.end(),
		chunk_.dynoItems_.begin(), chunk_.dynoItems_.end() );
}


bool EditorChunkCache::edIsLocked() const
{
#ifdef BIGWORLD_CLIENT_ONLY
	return false;
#else

	BW_GUARD;

	return EditorChunk::chunkIsLockedByMe( chunk_, false );
#endif // BIGWORLD_CLIENT_ONLY
}

bool EditorChunkCache::edIsWriteable( bool bCheckSurroundings /*= true*/,
	ChunkNotWritableReason* retReason /*= NULL*/ ) const
{
#ifdef BIGWORLD_CLIENT_ONLY
	if (retReason)
	{
		*retReason = REASON_DEFAULT;
	}
	return false;
#else

	BW_GUARD;

	if( edReadOnly() )
	{
		if (retReason)
		{
			*retReason = REASON_FILEREADONLY;
		}
		return false;
	}

	bool lockedByMe = EditorChunk::chunkIsLockedByMe( chunk_, bCheckSurroundings );
	if ( !lockedByMe  && retReason)
	{
		*retReason = REASON_NOTLOCKEDBYME;
	}

	return lockedByMe;

#endif // BIGWORLD_CLIENT_ONLY
}


/**
 *	Inform the terrain cache of the first terrain item in the chunk
 *
 *	This is required as the cache only knows of a single item, and
 *	the editor will sometimes allow multiple blocks in the one chunk
 *	(say, when cloning).
 */
void EditorChunkCache::fixTerrainBlocks()
{
	BW_GUARD;

	if (ChunkTerrainCache::instance( chunk_ ).pTerrain())
		return;

	RecursiveMutexHolder lock( chunk_.chunkMutex_ );
	BW::vector<ChunkItemPtr>::iterator i = chunk_.selfItems_.begin();
	for (; i != chunk_.selfItems_.end(); ++i)
	{
		ChunkItemPtr item = *i;
		if (item->edClassName() == Name("ChunkTerrain"))
		{
			item->toss(item->chunk());
			break;
		}
	}
}





/**
 *	Ensure that EditorChunkCache::instance() method always
 *	returns an EditorChunkCache
 */
// This is needed due to the fragility of the interaction between
// EditorChunkCacheBase and EditorChunkCache (in WorldEditor)
// See bug 32433: Reproducible random memory crash in WE using stress tester
// for details.
template <>
EditorChunkCache & ChunkCache::Instance<EditorChunkCache>::operator()( Chunk & chunk ) const
{
	MF_ASSERT( chunk.loading() || chunk.loaded() );
	ChunkCache * & cc = chunk.cache( index_ );
	if (cc == NULL) cc = new EditorChunkCache( chunk );
	MF_ASSERT( dynamic_cast< EditorChunkCache * >( cc ) != NULL );
	return static_cast<EditorChunkCache &>(*cc);
}


/// Static instance accessor initialiser
ChunkCache::Instance<EditorChunkCache>
	EditorChunkCache::instance( &EditorChunkCacheBase::instance );






// -----------------------------------------------------------------------------
// Section: ChunkExistenceOperation
// -----------------------------------------------------------------------------

void ChunkExistenceOperation::undo()
{
	BW_GUARD;

	// first add the redo operation
	UndoRedo::instance().add(
		new ChunkExistenceOperation( pChunk_, !create_ ) );

	BW::vector<ChunkItemPtr> selection = WorldManager::instance().selectedItems();

	// now create or delete it
	if (create_)
	{
		EditorChunkCache::instance( *pChunk_ ).edArrive();
		EditorChunkCache::instance( *pChunk_ ).edPostUndelete();

		selection.push_back( EditorChunkCache::instance( *pChunk_ ).getShellModel() );
	}
	else
	{
		EditorChunkCache::instance( *pChunk_ ).edPreDelete();
		EditorChunkCache::instance( *pChunk_ ).edDepart();
		ChunkItemPtr shellModel = EditorChunkCache::instance( *pChunk_ ).getShellModel();
		BW::vector<ChunkItemPtr>::iterator i =
			std::find( selection.begin(), selection.end(), shellModel);

		if (i != selection.end())
			selection.erase( i );

		WorldManager::instance().cleanChunkDirtyStatus( pChunk_ );
	}

	WorldManager::instance().setSelection( selection, false );
}

bool ChunkExistenceOperation::iseq( const UndoRedo::Operation & oth ) const
{
	// these operations never replace each other
	return false;
}

BW_END_NAMESPACE

