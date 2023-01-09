#include "loading_column.hpp"

#include "edge_geometry_mapping.hpp"
#include "chunk_loading_ref_count.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_loader.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_space.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
LoadingColumn::LoadingColumn():
		pos_( 0 ),
		pOutsideChunk_( NULL ),
		state_( UNLOADED )
{
}


/**
 *	Destructor.
 */
LoadingColumn::~LoadingColumn()
{
}


// -----------------------------------------------------------------------------
// Section: Loading
// -----------------------------------------------------------------------------

// Note about chunk states: 'loading' is controlled solely by us, to keep track
// of whether or not a chunk has been submitted to be loaded and had not yet
// come back. Whereas 'loaded' is set by the chunk when it loads itself.

/**
 *	Take a loading step in the given column.
 *	Returns true if the next one should be progressed to
 */
bool LoadingColumn::stepLoad( EdgeGeometryMapping * pMapping,
		int edgePos, bool isVerticalEdge, bool & rAnyColumnsLoaded )
{
	PROFILER_SCOPED( LoadingColumn_stepLoad );
	switch (state_)
	{
		case UNLOADED:
		{
			this->loadOutsideChunk( pMapping, edgePos, isVerticalEdge );
			state_ = LOADING_OUTSIDE;
			break;
		}

		case LOADING_OUTSIDE:
		{
			if (pOutsideChunk_->loaded())
			{
				this->loadOverlappers();
				state_ = LOADING_INSIDE;
				// Deliberate fall-through. This is useful when there are no
				// overlappers. We will skip straight past the LOADING_INSIDE
				// state to LOADED.
			}
			else
			{
				// Need to keep trying until it is loaded.
				break;
			}
		}

		case LOADING_INSIDE:
		{
			if (this->areOverlappersLoaded())
			{
				this->trimClientOnlyBoundaries( pMapping );
				this->bindOutsideChunk( pMapping );
				this->bindOverlappers( pMapping );
				// They're all loaded and bound
				state_ = LOADED;

				rAnyColumnsLoaded = true;
			}

			break;
		}

		case LOADED:
		{
			// __kyl__ (16/7/2007) Commented out error message since this can
			// occur if we suddenly change from unloading a line to loading it
			// again. And it's not really an error.
			// ERROR_MSG( "EdgeGeometryMapping::stepLoad: Stepping a loaded column\n" );
			return true;
		}

		default:
		{
			CRITICAL_MSG( "EdgeGeometryMapping::stepLoad: "
				"Illegal case in LoadingColumn record\n" );
			break;
		}
	}

	return state_ == LOADED;
}


/**
 *	This method loads the current outside chunk of this column.
 */
void LoadingColumn::loadOutsideChunk( EdgeGeometryMapping * pMapping,
		int edgePos, bool isVerticalEdge )
{
	pOutsideChunk_ = pMapping->findOrAddOutsideChunk(
			isVerticalEdge ? edgePos : pos_,
			isVerticalEdge ? pos_ : edgePos );

	MF_ASSERT( !pOutsideChunk_->loading() &&
		!pOutsideChunk_->isBound() );
	ChunkLoader::load( pOutsideChunk_ );
}


/**
 *	This method changes any portals which lead to chunks we are not loading into
 *	extern portals instead.
 */
void LoadingColumn::trimClientOnlyBoundaries( EdgeGeometryMapping * pMapping )
{
	this->trimClientOnlyBoundaries( pOutsideChunk_, pMapping );

	// TODO: Catch when an inside chunk has pulled in an outside chunk that
	// will never be loaded.
	// This is complicated because the inside chunk may have already been
	// bound, which would have put that outside chunk into the space map.
}


/**
 *	This method changes any portals in the given chunk which lead to chunks we
 *	are not loading into extern portals instead.
 */
void LoadingColumn::trimClientOnlyBoundaries( Chunk * pChunk,
	EdgeGeometryMapping * pMapping )
{
	MF_ASSERT( pChunk->loaded() );
	MF_ASSERT( !pChunk->isBound() );

	for (ChunkBoundaries::iterator iJoint = pChunk->joints().begin();
		iJoint != pChunk->joints().end(); ++iJoint)
	{
		ChunkBoundary::Portals::iterator iPortal;

		// Check that none of our portals have been bound yet
		for (iPortal = (*iJoint)->boundPortals_.begin();
			iPortal != (*iJoint)->boundPortals_.end();
			++iPortal)
		{
			MF_ASSERT( (*iPortal)->isEarth() );
		}

		for (iPortal = (*iJoint)->unboundPortals_.begin();
			iPortal != (*iJoint)->unboundPortals_.end();
			++iPortal)
		{
			ChunkBoundary::Portal * pPortal = *iPortal;
			if (!pPortal->hasChunk())
			{
				// Doesn't need trimming
				continue;
			}

			Chunk * & rpChunk = pPortal->pChunk;

			if (rpChunk->mapping() != pMapping)
			{
				// An external link
				continue;
			}

			if (!rpChunk->isOutsideChunk())
			{
				// We don't trim inside-chunk portals
				continue;
			}

			if (rpChunk->x() >= pMapping->minLGridX() &&
				rpChunk->x() <= pMapping->maxLGridX() &&
				rpChunk->z() >= pMapping->minLGridY() &&
				rpChunk->z() <= pMapping->maxLGridY())
			{
				// Target chunk is within the space
				continue;
			}

			/*
			DEBUG_MSG( "LoadingColumn( %s )::trimClientOnlyBoundaries: Trimmed "
				"link to %s, which falls outside edgeBounds\n", 
				pOutsideChunk_->identifier().c_str(),
				rpChunk->identifier().c_str() );
			*/

			// This is preempting BaseChunkSpace::findOrAddChunk
			bw_safe_delete( rpChunk );
			rpChunk = (Chunk*)ChunkBoundary::Portal::EXTERN;
		}
	}
}


/**
 *	This method checks the whether the outside chunk has finished loading and
 *	binds it.
 *
 *	@return True if the chunk has finished loading.
 */
void LoadingColumn::bindOutsideChunk( EdgeGeometryMapping * pMapping )
{
	MF_ASSERT( pOutsideChunk_->loaded() );
	MF_ASSERT( !pOutsideChunk_->mapping()->condemned() );

	// ok bind it
	ChunkOverlappers::Overlappers & overlappers =
		ChunkOverlappers::instance( *pOutsideChunk_ ).overlappers();

	size_t i = overlappers.size();

	pMapping->bind( pOutsideChunk_ );

	// After binding, we may get new overlapper chunks from our neighbours, we
	// need to make sure their reference count is updated to take into account
	// this outside chunk. 
	while (i < overlappers.size())
	{
		Chunk * pNewOverlapperChunk = overlappers[i]->pOverlappingChunk();

		DEBUG_MSG( "LoadingColumn::bindOutsideChunk: %s "
				"got new overlapper %s from neighbour\n",
			pOutsideChunk_->identifier().c_str(),
			pNewOverlapperChunk->identifier().c_str() );

		ChunkLoadingRefCount::instance( *pNewOverlapperChunk ).incNumOverlapped();

		++i;
	}
}


/**
 *	This method starts the loading of the chunks overlapping the current
 *	outside chunk.
 */
void LoadingColumn::loadOverlappers()
{
	// Make sure that all the chunks are appointed.
	ChunkOverlappers::instance( *pOutsideChunk_ ).findAppointedChunks();

	ChunkOverlappers::Overlappers & overlappers =
		ChunkOverlappers::instance( *pOutsideChunk_ ).overlappers();
	ChunkOverlappers::Overlappers::const_iterator iter = overlappers.begin();

	while (iter != overlappers.end())
	{
		Chunk * pOverlapper = (*iter)->pOverlappingChunk();

		MF_ASSERT( !pOverlapper->isOutsideChunk() );

		if (pOverlapper->isBound())
		{
			pOverlapper->bindPortals( true, true );
		}
		else if (!pOverlapper->loading())
		{
			ChunkLoader::load( pOverlapper );
		}

		ChunkLoadingRefCount::instance( *pOverlapper ).incNumOverlapped();

		++iter;
	}
}


/**
 *	This method returns whether the overlappers in this column are all loaded.
 */
bool LoadingColumn::areOverlappersLoaded() const
{
	const ChunkOverlappers::Overlappers & overlappers =
		ChunkOverlappers::instance( *pOutsideChunk_ ).overlappers();
	ChunkOverlappers::Overlappers::const_iterator iter = overlappers.begin();

	while (iter != overlappers.end())
	{
		if (!(*iter)->pOverlappingChunk()->loaded())
		{
			return false;
		}

		++iter;
	}

	return true;
}


/**
 *	This method checks the overlappers that are currently loading and binds
 *	them if they have finished.
 *
 *	@return True if all overlappers have been bound.
 */
void LoadingColumn::bindOverlappers( EdgeGeometryMapping * pMapping )
{
	const ChunkOverlappers::Overlappers & overlappers =
		ChunkOverlappers::instance( *pOutsideChunk_ ).overlappers();
	ChunkOverlappers::Overlappers::const_iterator iter = overlappers.begin();

	while (iter != overlappers.end())
	{
		Chunk * pOverlapper = (*iter)->pOverlappingChunk();

		MF_ASSERT( !pOverlapper->isOutsideChunk() );
		MF_ASSERT( !pOverlapper->mapping()->condemned() );

		if (!pOverlapper->isBound())
		{
			// ok bind it
			pMapping->bind( pOverlapper );
		}

		MF_ASSERT( !pOverlapper->loading() );

		++iter;
	}
}


// -----------------------------------------------------------------------------
// Section: Unloading
// -----------------------------------------------------------------------------

/**
 *	This method attempts to progress the unloading of the current column.
 */
bool LoadingColumn::stepUnload( EdgeGeometryMapping * pMapping,
		int edgePos, bool isVerticalEdge )
{
	PROFILER_SCOPED( LoadingColumn_stepUnload );

	switch (state_)
	{
		case LOADED:
			// Chunk may already unloaded by ChunkSpace::clear()
			pOutsideChunk_ = pMapping->findOutsideChunk(
					isVerticalEdge ? edgePos : pos_,
					isVerticalEdge ? pos_ : edgePos );
			if (pOutsideChunk_)
			{
				this->unloadOverlappers( pMapping );
				this->unloadOutsideChunk( pMapping );
			}

			state_ = UNLOADED;
			break;

		case LOADING_INSIDE:
			// Chunk may already unloaded by ChunkSpace::clear()
			pOutsideChunk_ = pMapping->findOutsideChunk(
				isVerticalEdge ? edgePos : pos_,
				isVerticalEdge ? pos_ : edgePos );
			if (!pOutsideChunk_)
			{
				state_ = UNLOADED;
			}
			else if (this->areOverlappersLoaded())
			{
				this->cancelLoadingOverlappers( pMapping );
				this->cancelLoadingOutside( pMapping );

				state_ = UNLOADED;
			}
			break;

		case LOADING_OUTSIDE:
			if (pOutsideChunk_->loaded())
			{
				this->cancelLoadingOutside( pMapping );
				state_ = UNLOADED;
			}
			break;

		case UNLOADED:
			// __kyl__ (16/7/2007) Removed following error message since it is
			// not really an error. This occurs when we are trying to unload
			// the last 2 chunks of a line. If one chunk is unloaded much
			// quicker than the other, it causes the unloaded to chunk to be
			// be "unloaded" repeatedly every tick until the other chunk
			// finishes unloading. This was a rare error since unloading chunks
			// doesn't take longer than 1 tick unless it is not fully loaded.
			// The new faster loading at start-up algorithm now makes it more
			// likely that we try to unload a chunk that we recently wanted
			// to load.
			// ERROR_MSG( "EdgeGeometryMapping::stepUnload: Unloading UNLOADED column\n" );
			return true;

		default:
			CRITICAL_MSG( "EdgeGeometryMapping::stepUnload: "
				"Illegal case in LoadingColumn record\n" );
			break;
	}

	return state_ == UNLOADED;
}


void LoadingColumn::unloadOverlappers( EdgeGeometryMapping * pMapping )
{
	MF_ASSERT( this->areOverlappersLoaded() );

	// unbind and unload all inside chunks not needed by another outside chunk.

	const ChunkOverlappers::Overlappers & overlappers =
		ChunkOverlappers::instance( *pOutsideChunk_ ).overlappers();
	ChunkOverlappers::Overlappers::const_iterator iter = overlappers.begin();

	while (iter != overlappers.end())
	{
		Chunk * pOverlapper = (*iter)->pOverlappingChunk();

		MF_ASSERT( !pOverlapper->isOutsideChunk() );

		ChunkLoadingRefCount & chunkLoadingRefCount =
			ChunkLoadingRefCount::instance( *pOverlapper );

		chunkLoadingRefCount.decNumOverlapped();

		if (chunkLoadingRefCount.numOverlapped() == 0)
		{
			// This is the last outside chunk wanting this overlapper.
			pMapping->unbindAndUnload( pOverlapper );
		}

		++iter;
	}
}


void LoadingColumn::unloadOutsideChunk( EdgeGeometryMapping * pMapping )
{
	pMapping->unbindAndUnload( pOutsideChunk_ );
	pOutsideChunk_->space()->focus();
}


/**
 *	This method cancels the loading of any overlapper of this chunk. It returns
 *	true once all overlappers have been unloaded.
 */
void LoadingColumn::cancelLoadingOverlappers( EdgeGeometryMapping * pMapping )
{
	DEBUG_MSG( "LoadingColumn::cancelLoadingOverlappers: condemned = %d\n",
					pMapping->condemned() );

	// ok they've all loaded, don't bind them, just unload them
	const ChunkOverlappers::Overlappers & overlappers =
		ChunkOverlappers::instance( *pOutsideChunk_ ).overlappers();
	ChunkOverlappers::Overlappers::const_iterator iter = overlappers.begin();

	while (iter != overlappers.end())
	{
		Chunk * pOverlapper = (*iter)->pOverlappingChunk();

		MF_ASSERT( !pOverlapper->isOutsideChunk() );

		ChunkLoadingRefCount & chunkLoadingRefCount =
			ChunkLoadingRefCount::instance( *pOverlapper );

		chunkLoadingRefCount.decNumOverlapped();

		if (chunkLoadingRefCount.numOverlapped() == 0)
		{
			if (pOverlapper->isBound())
			{
				// This chunk was bound by another column.
				pMapping->unbindAndUnload( pOverlapper );
			}
			else
			{
				pMapping->pSpace()->unloadChunkBeforeBinding(
					pOverlapper );
			}
		}

		++iter;
	}
}


void LoadingColumn::cancelLoadingOutside( EdgeGeometryMapping * pMapping )
{
	MF_ASSERT( pOutsideChunk_->loaded() );

	// Delete unappointed overlapping chunks.
	ChunkOverlappers::Overlappers & overlappers =
		ChunkOverlappers::instance( *pOutsideChunk_ ).overlappers();
	ChunkOverlappers::Overlappers::iterator iter = overlappers.begin();

	while (iter != overlappers.end())
	{
		Chunk * pOverlapper = (*iter)->pOverlappingChunk();
		ChunkLoadingRefCount & chunkLoadingRefCount =
			ChunkLoadingRefCount::instance( *pOverlapper );
		if (!pOverlapper->isAppointed())
		{
			DEBUG_MSG( "LoadingColumn::cancelLoadingOutside: "
					"Deleted overlapper!\n" );
			delete pOverlapper;
			iter = overlappers.erase( iter );
		}
		else if (chunkLoadingRefCount.numOverlapped() == 0)
		{
			DEBUG_MSG( "LoadingColumn::cancelLoadingOutside: "
					"Cleaning potentially unreferenced overlapper %s\n", 
					pOverlapper->identifier().c_str() );
			delete pOverlapper;
			iter = overlappers.erase( iter );
		}
		else
		{
			++iter;
		}
	}

	// unload it without binding or unbinding it
	pMapping->pSpace()->unloadChunkBeforeBinding( pOutsideChunk_ );
	bw_safe_delete( pOutsideChunk_ );
}


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/**
 *	This method is called during shutdown to handle the chunks that are in the
 *	process of being loaded.
 */
void LoadingColumn::prepareNewlyLoadedChunksForDelete(
		EdgeGeometryMapping * pMapping )
{
	if (state_ == LOADING_OUTSIDE)
	{
		this->cancelLoadingOutside( pMapping );

		state_ = UNLOADED;
	}
	else if (state_ == LOADING_INSIDE)
	{
		this->cancelLoadingOverlappers( pMapping );
		this->cancelLoadingOutside( pMapping );

		state_ = UNLOADED;
	}

	// All other cases are handled properly by normal
	// shutdown code.
}

BW_END_NAMESPACE

// loading_column.cpp
