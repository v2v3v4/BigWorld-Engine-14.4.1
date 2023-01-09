#include "pch.hpp"

#include "editor_chunk_navmesh_cache_base.hpp"
#include "girth.hpp"
#include "recast_processor.hpp"
#include "navmesh_processor.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"
#include "cstdmf/watcher.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/geometry_mapping.hpp"

BW_BEGIN_NAMESPACE

int EditorChunkNavmeshCacheBase_token = 0;


static const Girths& girths()
{
	static Girths s_girths;

	return s_girths;
}


/// Time when dirty flag remains untouched to calculate a new nav mesh
const double EditorChunkNavmeshCacheBase::IDLE_NAV_CALC_TIME = 1.0;
bool EditorChunkNavmeshCacheBase::s_doWaitBeforeCalculate = false;


ChunkCache::Instance<EditorChunkNavmeshCacheBase> EditorChunkNavmeshCacheBase::instance;


void EditorChunkNavmeshCacheBase::touch( Chunk& chunk )
{
	BW_GUARD;

	EditorChunkNavmeshCacheBase::instance( chunk );
}


bool EditorChunkNavmeshCacheBase::load( DataSectionPtr, DataSectionPtr cdata )
{
	BW_GUARD;

	dirty_ = true;

	// Read dirty status (bool written as binary).
	// This matches navgen's ChunkWaypointGenerator ctor.
	DataSectionPtr dirtySec = 
		cdata->findChild( "navmeshDirty" );
	if (dirtySec)
	{
		BinaryPtr bp = dirtySec->asBinary();
		if (bp->len() == sizeof(bool))
		{
			dirty_ = *((bool *)bp->cdata());
		}
	}

	if (!dirty_ && !cdata->findChild( "worldNavmesh" ))
	{
		dirty_ = true;
	}


	if (s_doWaitBeforeCalculate)
	{
		dirtyTime_ = timestamp();
	}

	DataSectionPtr ds = cdata ? cdata->openSection( "worldNavmesh" ) : NULL;

	if (ds)
	{
		BinaryPtr bp = ds->asBinary();

		navmesh_.assign( (unsigned char*)bp->data(),
			(unsigned char*)bp->data() + bp->len() );
	}

	// its not really an error to not have navigation data 
	// (you may not want a navmesh on spaces or chunks). 
	// If you want to see if a navmesh exists for a chunk for 
	// debugging purposes, just turn on nav	mesh visualisation in WE.
	return true;
}


void EditorChunkNavmeshCacheBase::saveChunk( DataSectionPtr chunk )
{
	BW_GUARD;

	chunk->deleteSection( "worldNavmesh" );

	if (chunk_.space()->navmeshGenerator() !=
			BaseChunkSpace::NAVMESH_GENERATOR_NONE)
	{
		chunk->openSection( "worldNavmesh", true )->
			writeString( "resource", chunk_.identifier() +
			".cdata/worldNavmesh" );
	}
}


void EditorChunkNavmeshCacheBase::saveCData( DataSectionPtr cdata )
{
	BW_GUARD;

	cdata->deleteSections( "worldNavmesh" );
	cdata->deleteSections( "navmeshDirty" );

	if (chunk_.space()->navmeshGenerator() !=
			BaseChunkSpace::NAVMESH_GENERATOR_NONE)
	{
		// Save dirty status (bool written as binary).
		// This matches navgen's "outputDirtyFlag" method.
		bool dirty = this->dirty();
		DataSectionPtr dirtySec = cdata->openSection( "navmeshDirty", true );
		dirtySec->setBinary( new BinaryBlock( &dirty, sizeof( dirty ), 
			"BinaryBlock/EditorChunkNavmeshCacheBase" ) );
		dirtySec->setParent( cdata );
		dirtySec->save();
		dirtySec->setParent( NULL );
	
		if (!navmesh_.empty())
		{
			cdata->writeBinary( "worldNavmesh", new BinaryBlock(
				&navmesh_[0], (int)navmesh_.size(), "navmesh" ) );
		}
	}
}

/**
 *	Check if navmesh is ready to be processed.
 *	Not already processing and chunk is loaded.
 *	@param manager processor manager.
 *	@return true if ready.
 */
bool EditorChunkNavmeshCacheBase::readyToCalculate(
	ChunkProcessorManager* manager )
{
	BW_GUARD;

	if (s_doWaitBeforeCalculate)
	{
		double time = TimeStamp::toSeconds( timestamp() - dirtyTime_ );

		if ( isBeingCalculated() || time < IDLE_NAV_CALC_TIME )
		{
			return false;
		}
	}

	if ( chunk_.isOutsideChunk() )
	{
		return manager->isChunkReadyToProcess( &chunk_, 1, 1 );
	}

	return manager->isChunkReadyToProcess( &chunk_, 1 );
}

/**
 *	Need to load chunks before processing them.
 *	@param manager the processor manager.
 *	@return true on successfully loading the chunk.
 */
bool EditorChunkNavmeshCacheBase::loadChunkForCalculate(
	ChunkProcessorManager* manager )
{
	BW_GUARD;

	if (chunk_.isOutsideChunk())
	{
		return manager->loadChunkForProcessing( &chunk_, 1, 1 );
	}

	return manager->loadChunkForProcessing( &chunk_, 1 );
}

/**
 *	Navmesh is calculated in background.
 *	@return true
 */
bool EditorChunkNavmeshCacheBase::requireProcessingInBackground() const
{
	if (chunk_.space()->navmeshGenerator() == BaseChunkSpace::NAVMESH_GENERATOR_RECAST)
	{
		return true;
	}
	return false;
}

/**
 *	Set the scene for this cache as dirty, needs navmesh calculation.
 *	@param manager			the processor manager.
 *	@param spread			which way to expand the scene.
 *	@param pChangedItem		the changed item that caused the chunk changed,
 *								NULL means it's other reason caused the chunk changed
 *	@return true on success.
 */
bool EditorChunkNavmeshCacheBase::invalidate(
		ChunkProcessorManager* manager,
		bool spread,
		EditorChunkItem* pChangedItem /*= NULL*/ )
{
	BW_GUARD;

	// Although in high level we already only called on the chunks that 
	// the item's bounding box intersect with, but since any object 
	// which is changed or invalidated within 1m of the edge of a chunk	
	// should invalidate the chunk its close to, as there is a 1m overlap 
	// used by recast and navgen to create links between waypoint sets,
	// so we still need spread 1 depth out.

	if (spread)
	{
		// spreadInvalidate(..) will call invalidate( spread = false ) again 
		// on this chunk and it's neighbours
		if (chunk_.isOutsideChunk())
		{
			manager->spreadInvalidate( &chunk_, 1, 1, instance.index(), pChangedItem );
		}
		else
		{
			manager->spreadInvalidate( &chunk_, 1, instance.index(), pChangedItem );
		}

	}
	else
	{

		if ( pChangedItem )
		{
			BoundingBox bb;
			pChangedItem->edWorldBounds( bb );
			// Item within 1m of the edge of a chunk will affect the chunk's navMesh
			bb.expandSymmetrically( 1, 1, 1 );
			if ( !chunk_.boundingBox().intersects( bb ) )
			{
				// the changed item won't affect this chunk, so do nothing.
				return false;
			}
		}


		dirty_ = true;

		if (s_doWaitBeforeCalculate)
		{
			dirtyTime_ = timestamp();
		}

		currentProcessor_ = NULL;

		manager->updateChunkDirtyStatus( &chunk_ );

	}

	return true;
}

/**
 *	Check if the navmesh needs recalculating.
 *	@return true if recalculating is needed.
 */
bool EditorChunkNavmeshCacheBase::dirty()
{
	if (chunk_.space()->navmeshGenerator() == BaseChunkSpace::NAVMESH_GENERATOR_NONE)
	{
		return false;
	}
	return dirty_;
}

/**
 *	Recalculate navmesh for chunks.
 *	Assert that we are ready to calculate.
 *	@param manager the processor manager.
 *	@param unsavedList the chunks to process.
 *	@return true on successfully creating a process (running).
 */
bool EditorChunkNavmeshCacheBase::recalc( ChunkProcessorManager* manager,
	UnsavedList& unsavedList )
{
	BW_GUARD;

	MF_ASSERT( readyToCalculate( manager ) );

	dirty_ = true;
	dirtyTime_ = timestamp();

	navmesh_.clear();

	switch (chunk_.space()->navmeshGenerator())
	{
	case ChunkSpace::NAVMESH_GENERATOR_NAVGEN:
		{
			currentProcessor_ = new NavmeshProcessor( this, chunk_,
				unsavedList, girths(), manager );
			break;
		}
	case ChunkSpace::NAVMESH_GENERATOR_RECAST:
		{
			currentProcessor_ = new RecastProcessor( this, &chunk_,
				unsavedList, girths(), manager );
			break;
		}
	default:
		{
			ERROR_MSG( "EditorChunkNavmeshCacheBase::recalc"
				"invalid navmesh type %d\n", chunk_.space()->navmeshGenerator() );
			return false;
		}
	}

	if (currentProcessor_.get() != NULL)
	{
		currentProcessor_->process( *manager );
	}

	// record this processor in case we want to
	// cancel back ground ChunkProcessors later
	this->registerProcessor( currentProcessor_ );

	return true;
}

/**
 *	Set calculation as finished (for the current processor).
 *	Save the processed result if calculation completed successfully.
 *	Clear the current processor.
 *	@param processor the processor to set as finished.
 *	@param backgroundTaskResult task completed successfully.
 *	@param navmesh resulting navmesh after processing.
 *	@param unsavedList list of unsaved chunks after processing.
 */
void EditorChunkNavmeshCacheBase::calcDone(
	ChunkProcessor* processor,
	bool backgroundTaskResult,
	const BW::vector<unsigned char>& navmesh,
	UnsavedList& unsavedList )
{
	BW_GUARD;

	if (currentProcessor_ == processor)
	{
		if (backgroundTaskResult)
		{
			navmesh_ = navmesh;
			dirty_ = false;
			unsavedList.unsavedChunks_.add( &chunk_ );

			//TRACE_MSG( "Calculated detour mesh for chunk %s\n",
			//	chunk_.identifier().c_str() );

			ChunkProcessorManager::instance().updateChunkDirtyStatus( &chunk_ );
		}

		currentProcessor_ = NULL;
	}
}
BW_END_NAMESPACE

