#include "pch.hpp"

#include "recast_processor.hpp"
#include "editor_chunk_navmesh_cache_base.hpp"

BW_BEGIN_NAMESPACE

RecastProcessor::RecastProcessor(
	EditorChunkNavmeshCacheBase* navmeshCache,
	Chunk* chunk,
	UnsavedList& unsavedList,
	Girths girths,
	ChunkProcessorManager* manager ) :

	girths_( girths ),
	navmeshCache_( navmeshCache ),
	unsavedList_( unsavedList ),
	lockedChunks_( manager ),
	bgtaskFinished_( false )
{
	BW_GUARD;

	collisionSceneProvider_ = RecastGenerator::collectTriangles( chunk );
	chunk_ = chunk;

	if (chunk_->isOutsideChunk())
	{
		lockedChunks_.lock( chunk, 1, 1 );
	}
	else
	{
		lockedChunks_.lock( chunk, 1 );
	}
}

RecastProcessor::~RecastProcessor()
{
	// Check chunks are unlocked
	if ( !lockedChunks_.empty() )
	{
		ERROR_MSG( "RecastProcessor is being destroyed when it has "
			"chunks locked\n" );
	}
}

bool RecastProcessor::processInBackground( ChunkProcessorManager& manager )
{
	BW_GUARD;

	// it might be stopped, ie chunk_ is about to unbind.
	if (BgTaskManager::shouldAbortTask())
	{
		return false;
	}

	Vector3 centre = chunk_->boundingBox().centre();
	BoundingBox visibilityBB = chunk_->visibilityBox();

	if (chunk_->isOutsideChunk())
	{
		// This is accessed by multiple threads
		// Make sure that overlappers are fully loaded and no longer changing
		const ChunkOverlappers::Overlappers& overlappers =
			ChunkOverlappers::instance( *chunk_ ).overlappers();

		ChunkOverlappers::Overlappers::const_iterator iter;
		for (iter = overlappers.begin(); iter != overlappers.end(); ++iter)
		{
			ChunkOverlapperPtr pChunkOverlapper = *iter;
			MF_ASSERT( pChunkOverlapper.exists() );

			Chunk* pChunk = pChunkOverlapper->pOverlappingChunk();

			if (pChunk)
			{
				visibilityBB.addBounds( pChunk->visibilityBox() );
			}
		}
	}

	int gridX = chunk_->space()->pointToGrid(
		centre.x - chunk_->space()->gridBounds().minBounds().x );

	int gridZ = chunk_->space()->pointToGrid(
		centre.z - chunk_->space()->gridBounds().minBounds().z );

	if (chunk_->isOutsideChunk())
	{
		Vector3 min( chunk_->boundingBox().minBounds() );
		Vector3 max( chunk_->boundingBox().maxBounds() );

		min.y = visibilityBB.minBounds().y - 1.f;
		max.y = visibilityBB.maxBounds().y + 1.f;

		visibilityBB.setBounds( min, max );
	}
	else
	{
		visibilityBB.expandSymmetrically( 0.f, 0.2f, 0.f );
	}

	for (size_t i = 0; i < girths_.size(); ++i)
	{
		// it might be stopped, ie chunk_ is about to unbind.
		if (BgTaskManager::shouldAbortTask())
		{
			return false;
		}

		const Girth& girth = girths_[ i ];
		RecastConfig config( visibilityBB, girth.maxSlope(), girth.maxClimb(),
			girth.height(), girth.radius(), chunk_->space()->scaleNavigation(),
			chunk_->space()->gridSize());

		if (!generator_.generate( collisionSceneProvider_, config, gridX, gridZ, 
			/* useMono */ !chunk_->isOutsideChunk() ))
		{
			return false;
		}

		if (!generator_.navmesh().empty())
		{
			BW::vector<unsigned char> mesh;
			generator_.convertRecastMeshToNavgenMesh(
				girth.girth(), *chunk_, mesh );

			navmesh_.insert( navmesh_.end(), mesh.begin(), mesh.end() );
		}
	}

	return true;
}


bool RecastProcessor::processInMainThread( ChunkProcessorManager& manager,
	bool backgroundTaskResult )
{
	BW_GUARD;

	navmeshCache_->calcDone(
		this, backgroundTaskResult, navmesh_, unsavedList_ );

	return true;
}


void RecastProcessor::cleanup( ChunkProcessorManager& manager )
{
	BW_GUARD;
	ChunkProcessor::cleanup( manager );
	lockedChunks_.clear();
}


void RecastProcessor::stop()
{
	BW_GUARD;
	BackgroundTask::stop();
	generator_.stop();
}
BW_END_NAMESPACE

