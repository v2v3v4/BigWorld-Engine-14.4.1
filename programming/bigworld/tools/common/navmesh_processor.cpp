#include "pch.hpp"

#include "navmesh_processor.hpp"
#include "editor_chunk_entity_base.hpp"
#include "editor_chunk_navmesh_cache_base.hpp"
#include "physics_handler.hpp"

BW_BEGIN_NAMESPACE

NavmeshProcessor::NavmeshProcessor(
	EditorChunkNavmeshCacheBase* navmeshCache,
	Chunk& chunk,
	UnsavedList& unsavedList,
	Girths girths,
	ChunkProcessorManager* manager ) :

	girths_( girths ),
	navmeshCache_( navmeshCache ),
	chunk_( chunk ),
	unsavedList_( unsavedList ),
	lockedChunks_( manager ),
	bgtaskFinished_( false ),
	flooder_( &chunk_, "" )
{
	BW_GUARD;

	if (chunk_.isOutsideChunk())
	{
		lockedChunks_.lock( &chunk, 1, 1 );
	}
	else
	{
		lockedChunks_.lock( &chunk, 1 );
	}
}

NavmeshProcessor::~NavmeshProcessor()
{
	// Check chunks are unlocked
	if ( !lockedChunks_.empty() )
	{
		ERROR_MSG( "NavmeshProcessor is being destroyed when it has chunks"
			"locked\n" );
	}
}

bool NavmeshProcessor::processInBackground( ChunkProcessorManager& manager )
{
	BW_GUARD;

	// Start processing
	for ( size_t i = 0; i < girths_.size(); ++i )
	{
		// Check if we have stopped before going on to next girth
		if (BgTaskManager::shouldAbortTask())
		{
			return false;
		}

		// Get ready to flood
		const Girth& girth = girths_[ i ];

		PhysicsHandler phand( chunk_.space(), girth );
		BW::vector<Vector3> entityPts;

		EditorChunkEntityCache::instance( chunk_ ).getEntityPositions(
			entityPts );

		// Try to flood
		if ( !flooder_.flood( girth, entityPts, NULL, 0, false ) )
		{
			// Flood was stopped or failed
			return false;
		}

		// Finish up and save
		int w = flooder_.width(), h = flooder_.height();

		gener_.init( w, h, flooder_.minBounds(), flooder_.resolution() );

		for (int g = 0; g < 16; ++g)
		{
			memcpy( gener_.adjGrids()[g], flooder_.adjGrids()[g], w*h*4 );
			memcpy( gener_.hgtGrids()[g], flooder_.hgtGrids()[g], w*h*4 );
		}

		gener_.generate();

		gener_.determineEdgesAdjacentToOtherChunks( &chunk_, &phand );
		gener_.streamline();
		gener_.extendThroughUnboundPortals( &chunk_ );
		gener_.calculateSetMembership( entityPts, chunk_.identifier() );

		BinaryPtr bp = gener_.asBinary( Matrix::identity, girth.girth() );

		navmesh_.insert( navmesh_.end(), (unsigned char*)bp->data(),
			(unsigned char*)bp->data() + bp->len() );
	}

	// Success
	return true;
}


bool NavmeshProcessor::processInMainThread(
	ChunkProcessorManager& manager,
	bool backgroundTaskResult )
{
	BW_GUARD;

	navmeshCache_->calcDone(
		this, backgroundTaskResult, navmesh_, unsavedList_ );

	return true;
}


void NavmeshProcessor::cleanup( ChunkProcessorManager& manager )
{
	BW_GUARD;
	ChunkProcessor::cleanup( manager );
	lockedChunks_.clear();
}
BW_END_NAMESPACE

