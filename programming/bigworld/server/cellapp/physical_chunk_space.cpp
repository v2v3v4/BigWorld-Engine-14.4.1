#include "script/first_include.hpp"

#include "physical_chunk_space.hpp"
#include "cstdmf/bw_namespace.hpp"

#include "cellapp.hpp"
#include "space.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Chunk Items
// -----------------------------------------------------------------------------

extern int ServerChunkModel_token;
extern int ServerChunkTree_token;
extern int ServerChunkWater_token;
extern int ChunkModelVLO_token;
extern int ChunkModelVLORef_token;
extern int ChunkTerrain_token;
extern int ChunkWaypointSet_token;
extern int ChunkStationNode_token;
extern int ChunkUserDataObject_token;
static int Chunk_token = ServerChunkModel_token | ChunkTerrain_token |
	ChunkWaypointSet_token | ServerChunkTree_token | ServerChunkWater_token |
	ChunkModelVLO_token | ChunkModelVLORef_token | ChunkStationNode_token|
	ChunkUserDataObject_token;

// -----------------------------------------------------------------------------
// Section: PhysicalChunkSpace
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PhysicalChunkSpace::LoadResourceCallback::LoadResourceCallback(
		SpaceID spaceID ):
	AddMappingAsyncCallback(),
	spaceID_( spaceID )
{
}

/**
 *	This method notifies the loading result, called from a background thread
 */
void PhysicalChunkSpace::LoadResourceCallback::onAddMappingAsyncCompleted( 
		SpaceEntryID spaceEntryID,
		AddMappingAsyncCallback::AddMappingResult result )
{
	if (result == AddMappingAsyncCallback::RESULT_FAILURE)
	{
		Space * pSpace = CellApp::instance().findSpace( spaceID_ );
		if (pSpace != NULL)
		{
			pSpace->onLoadResourceFailed( spaceEntryID );
		}
	}
}

/**
 *	Constructor.
 */
PhysicalChunkSpace::PhysicalChunkSpace( SpaceID id, GeometryMapper & mapper ) :
	pChunkSpace_( NULL ),
	geometryMappings_( mapper )
{
	pChunkSpace_ = new ChunkSpace( id );
	pChunkSpace_->pMappingFactory( &geometryMappings_ );	
}
	
/**
 *	This method requests to load the specified resource, 
 *	possibly using a translation matrix,
 *	and assigns it the given SpaceEntryID.
 *	(The loading itself may be performed asynchronously.)
 */
void PhysicalChunkSpace::loadResource( const SpaceEntryID & mappingID, 
									   const BW::string & path,
									   float * matrix )
{
	AddMappingAsyncCallbackPtr pAddMappingAsyncCB = 
		new LoadResourceCallback( pChunkSpace_->id() );
	pChunkSpace_->addMappingAsync( mappingID, matrix, path, pAddMappingAsyncCB );
}

/**
 *	This method requests to unload the resource specified by the given
 *	SpaceEntryID.
 *	(The unloading itself may be performed asynchronously.)
 */
void PhysicalChunkSpace::unloadResource( const SpaceEntryID & mappingID )
{
	pChunkSpace_->delMapping( mappingID );
}


/**
 *	This method loads and/or unloads resources, to cover the axis-aligned
 *	rectangle (the updated area that we serve).
 *	Return value: whether any new resources have been loaded.
 */
bool PhysicalChunkSpace::update( const BW::Rect & rect, bool unloadOnly )
{
	const Vector3 minB(rect.xMin(), 0.f, rect.yMin());
	const Vector3 maxB(rect.xMax(), 0.f, rect.yMax());
	return geometryMappings_.tickLoading( minB, maxB, unloadOnly,
		pChunkSpace_->id() );
}

/**
 *	This method does the best it can to determine an axis-aligned
 *	rectangle that has been loaded. If there is no loaded resource, 
 *	then a very big rectangle is returned.
 */
void PhysicalChunkSpace::getLoadedRect( BW::Rect & rect ) const
{
	geometryMappings_.calcLoadedRect( rect );
}


/*
 *  Overrides from IPhysicalSpace
 */
bool PhysicalChunkSpace::getLoadableRects( IPhysicalSpace::BoundsList & rects ) const /* override */
{
	if (pChunkSpace_->isMappingPending())
	{
		return false;
	}

	return geometryMappings_.getLoadableRects( rects );
}


/*
 *  Overrides from IPhysicalSpace
 */
BoundingBox PhysicalChunkSpace::bounds() const
{
	return pChunkSpace_->gridBounds();
}

/*
 *  Overrides from IPhysicalSpace
 */
BoundingBox PhysicalChunkSpace::subBounds() const
{
	return pChunkSpace_->subGridBounds();
}

/**
 *	This function goes through any resources that have been requested to be 
 *	loaded, and cancels their loading process.
 */
void PhysicalChunkSpace::cancelCurrentlyLoading()
{
	geometryMappings_.prepareNewlyLoadedChunksForDelete();
}

/**
 *	This method returns whether or not the space is fully unloaded.
 */
bool PhysicalChunkSpace::isFullyUnloaded() const
{
	return geometryMappings_.isFullyUnloaded();
}


/**
 *	This method prepares the space to be reused.
 */
void PhysicalChunkSpace::reuse()
{
	pChunkSpace_->clear();
}


/**
 *	This method unloads all resources and clears all state.
 */
void PhysicalChunkSpace::clear()
{
	pChunkSpace_->clear();
	pChunkSpace_->pMappingFactory( NULL );
}

BW_END_NAMESPACE


