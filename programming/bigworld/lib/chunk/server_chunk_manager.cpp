#include "chunk_manager.hpp"
#include "chunk_space.hpp"
#include "chunk_loader.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

/**
 *	@file This file contains the server's implementation of the Chunk Manager.
 */

/**
 *	Constructor.
 */
ChunkManager::ChunkManager() :
	initted_(),
	cameraTrans_( Matrix::identity ),
	pCameraSpace_( NULL ),
	cameraChunk_( NULL ),
	fringeHead_(),
#if ENABLE_RELOAD_MODEL
	floraVisualManager_(),
#endif
	maxLoadPath_(),
	minUnloadPath_(),
	scanSkippedFor_(),
	noneLoadedAtLastScan_(),
	canLoadChunks_(),
	maxUnloadChunks_(),
	workingInSyncMode_(),
	waitingForTerrainLoad_(),
#if UMBRA_ENABLE
	umbraCamera_(),
	useLatentOcclusionQueries_(),
#endif
	tickMark_(),
	totalTickTimeInMS_(),
	dTime_(),
	scanEnabled_(),
	timingState_(),
	timeLoadingCallback_()
{
}

/**
 *	Destructor.
 */
ChunkManager::~ChunkManager()
{
}


/**
 *	This method is called by a chunk space to add itself to our list
 */
void ChunkManager::addSpace( ChunkSpace * pSpace )
{
	spaces_.insert( std::make_pair( pSpace->id(), pSpace ) );
}

/**
 *	This method is called by a chunk space to remove itself from our list
 */
void ChunkManager::delSpace( ChunkSpace * pSpace )
{
	ChunkSpaces::iterator found = spaces_.find( pSpace->id() );
	if (found != spaces_.end())
		spaces_.erase( found );
}


/**
 *	This static method returns the singleton ChunkManager instance.
 */
ChunkManager & ChunkManager::instance()
{
	static	ChunkManager	chunky;
	return chunky;
}


/**
 *	This method returns the space with the input id.
 *
 *	@note	For the server, createIfMissing must be false.
 */
ChunkSpacePtr ChunkManager::space( ChunkSpaceID spaceID, bool createIfMissing )
{
	if (createIfMissing)
	{
		WARNING_MSG( "ChunkManager::space: "
			"createIfMissing must be false for the server\n" );
	}

	ChunkSpaces::iterator iter = spaces_.find( spaceID );
	return (iter != spaces_.end()) ? iter->second : NULL;
}


/**
 *	This method returns the space that the camera is in.
 *
 *	@note	For the server, NULL is always returned.
 */
ChunkSpacePtr ChunkManager::cameraSpace() const
{
	return NULL;
}


/**
 *      This method sets the space that the camera is in.
 *
 *      @note   For the server, do nothing.
 */
void ChunkManager::camera( const Matrix &, ChunkSpacePtr, Chunk*  )
{
}

BW_END_NAMESPACE

// server_chunk_manager.cpp
