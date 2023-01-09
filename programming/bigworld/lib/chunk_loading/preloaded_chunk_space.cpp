#include "preloaded_chunk_space.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

const float NEAR_MAX = 1000000000.f;
const Vector3 SPACE_MIN = Vector3( -NEAR_MAX, 0, -NEAR_MAX );
const Vector3 SPACE_MAX = Vector3( NEAR_MAX, 0, NEAR_MAX );

// -----------------------------------------------------------------------------
// Section: PreloadedChunkSpace
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PreloadedChunkSpace::PreloadedChunkSpace( Matrix m, const BW::string & path,
		const SpaceEntryID & entryID, SpaceID spaceID,
		GeometryMapper & mapper ) :
	geometryMappings_( mapper )
{
	pChunkSpace_ = new ChunkSpace( spaceID );
	pChunkSpace_->pMappingFactory( &geometryMappings_ );

	pChunkSpace_->addMappingAsync( entryID, m, path );
}


/**
 *	This method progresses with loading all of the chunks for the whole space
 *	area.
 */
void PreloadedChunkSpace::chunkTick()
{
	geometryMappings_.tickLoading( SPACE_MIN, SPACE_MAX,
			false /*unloadOnly*/, pChunkSpace_->id() );
}


/**
 *	This function goes through any chunks that were submitted to the loading
 *	thread and prepares them for deletion if the loading thread has finished
 *	loading them. Also changes the loading flag to false for those that were
 *	submitted but not loaded.
 */
void PreloadedChunkSpace::prepareNewlyLoadedChunksForDelete()
{
	geometryMappings_.prepareNewlyLoadedChunksForDelete();
}


BW_END_NAMESPACE

// space.cpp
