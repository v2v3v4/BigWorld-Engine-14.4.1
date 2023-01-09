#include "pch.hpp"
#include "chunk_model_obstacle.hpp"

#include "cstdmf/bw_set.hpp"
#include "physics2/hulltree.hpp"
#include "chunk_space.hpp"
#include "chunk_obstacle.hpp"
#include "physics2/bsp.hpp"

#include "cstdmf/guard.hpp"


#ifdef MF_SERVER
#include "server_super_model.hpp"
#else  // MF_SERVER (is client)
#include "model/super_model.hpp"
#endif // MF_SERVER

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

// ----------------------------------------------------------------------------
// Section: ChunkModelObstacle
// ----------------------------------------------------------------------------

/**
 *	Constructor
 */
ChunkModelObstacle::ChunkModelObstacle( Chunk & chunk ) :
	pChunk_( &chunk )
{
}


/**
 *	Destructor
 */
ChunkModelObstacle::~ChunkModelObstacle()
{
	BW_GUARD;
#ifdef EDITOR_ENABLED
	SimpleMutexHolder smh( obstaclesMutex_ );
#endif

	for (Obstacles::iterator it = obstacles_.begin();
		it != obstacles_.end(); ++it)
	{
		this->delFromSpace( *(*it) );
	}
}


typedef BW::set< ChunkSpace::Column * > ColumnSet;


/**
 *	Focus method. Adds obstacles to appropriate columns
 */
int ChunkModelObstacle::focus()
{
	BW_GUARD;
#ifdef EDITOR_ENABLED
	SimpleMutexHolder smh( obstaclesMutex_ );
#endif

	int foco = 0;

	for (Obstacles::iterator it = obstacles_.begin();
		it != obstacles_.end(); ++it)
	{
		foco += this->addToSpace( *(*it) );
	}

	return foco;
}


/**
 *	This method adds the input model's triangles to this obstacle cache
 */
void ChunkModelObstacle::addModel( SmartPointer<Model> model,
	const Matrix & world, ChunkItemPtr pItem, bool editorProxy /* = false */ )
{
    BW_GUARD;
	if (model == NULL || pItem == NULL)
        return;

	// Mutually exclusive of model reloading, start
	model->beginRead();

	const BSPTree * pBSPTree = model->decompose();

#ifdef EDITOR_ENABLED
	if ( !pBSPTree || pBSPTree->numTriangles() == 0 )
	{
		// If a model the editor is using doesn't have any bsp data, or if
		// all it's triangles are TRIANGLE_NOT_IN_BSP, add a box
		// to the collision scene so the model can still be selected via its
		// bounding box.
		static ModelPtr standInModel = Model::get(
			"helpers/props/transparent_standin.model" );

		BoundingBox modelBB = model->boundingBox();
		BoundingBox standInBB = standInModel->boundingBox();

		Matrix newWorld = world;

		float mxw = modelBB.maxBounds().x - modelBB.minBounds().x;
		float myw = modelBB.maxBounds().y - modelBB.minBounds().y;
		float mzw = modelBB.maxBounds().z - modelBB.minBounds().z;

		float sxw = standInBB.maxBounds().x - standInBB.minBounds().x;
		float syw = standInBB.maxBounds().y - standInBB.minBounds().y;
		float szw = standInBB.maxBounds().z - standInBB.minBounds().z;

		Matrix scale;
		scale.setScale( mxw / sxw,
			myw / syw,
			mzw / szw );

		newWorld.preMultiply( scale );

		Vector3 modelCenter = world.applyPoint( modelBB.centre() );
		Vector3 standInCenter = newWorld.applyPoint( standInBB.centre() );

		Matrix translation;
		translation.setTranslate( modelCenter - standInCenter );
		newWorld.postMultiply( translation );

		pBSPTree = standInModel->decompose();

		IF_NOT_MF_ASSERT_DEV( pBSPTree && pBSPTree->numTriangles() > 0 )
		{
			// Mutually exclusive of model reloading, end
			model->endRead();
			return;
		}

		this->addObstacle( new ChunkBSPObstacle(
			standInModel.get(), newWorld, &standInModel->boundingBox(), pItem ) );
	}
	else if ( !pBSPTree->canCollide() )
	{
		// The original BSP shouldn't be taken into account in shadow
		// calcs if all of its triangles are flaged TRIANGLE_NOT_IN_BSP,
		// so here we remap the triangles' flags to TRIANGLE_TRANSPARENT
		// so it gets selected but it doesn't cast shadows (flag used in
		// EditorChunkTerrain). Flag TRIANGLE_NOCOLLIDE is used so the BSP
		// doesn't get drawn, and TRIANGLE_DOUBLESIDED to improve selection.
		// TODO: we should really rethink the way BSP collision flags
		// work in the editor.
		BSPFlagsMap bspMap;
		bspMap.push_back( TRIANGLE_TRANSPARENT | TRIANGLE_NOCOLLIDE | TRIANGLE_DOUBLESIDED );
		const_cast<BSPTree*>(pBSPTree)->remapFlags( bspMap );

		this->addObstacle( new ChunkBSPObstacle(
			model.get(), world, &model->boundingBox(), pItem ) );
	}
	else
#endif
	if (pBSPTree != NULL)
	{
		// Make sure that any editor proxy models do not cast shadows but are selectable
		if (editorProxy)
		{
			BSPFlagsMap bspMap;
			bspMap.push_back( TRIANGLE_TRANSPARENT | TRIANGLE_NOCOLLIDE | TRIANGLE_DOUBLESIDED );
			const_cast<BSPTree*>(pBSPTree)->remapFlags( bspMap );
		}

		ChunkBSPObstacle* pObstacle = new ChunkBSPObstacle(
			model.get(), world, &model->boundingBox(), pItem );
		this->addObstacle( pObstacle );
	}

	// Mutually exclusive of model reloading, end
	model->endRead();
}


/**
 *	This method adds an obstacle representing the input item to our obstacle
 *	cache. We then take care of adding it to the space, etc.
 */
void ChunkModelObstacle::addObstacle( ChunkObstacle * pObstacle )
{
	BW_GUARD;
#ifdef EDITOR_ENABLED
	SimpleMutexHolder smh( obstaclesMutex_ );
#endif

	obstacles_.push_back( pObstacle );

	// if our chunk is already focussed, then add it immediately,
	// otherwise we'll be added when it comes under the grid
	if (pObstacle->pItem()->chunk()->focussed())
	{
		this->addToSpace( *obstacles_.back() );
	}
}


/**
 *	This method removes the input item's obstacles from our obstacle cache.
 */
void ChunkModelObstacle::delObstacles( ChunkItemPtr pItem )
{
	BW_GUARD;
#ifdef EDITOR_ENABLED
	SimpleMutexHolder smh( obstaclesMutex_ );
#endif

	// find all the obstacles made from this item
	Obstacles::iterator it = obstacles_.begin();

	while (it != obstacles_.end())
	{
		ChunkObstacle & cso = *(*it);
		if (cso.pItem() == pItem)
		{
			// let the space know it's gone
			this->delFromSpace( cso );

			// remove it from our vector
			it = obstacles_.erase( it );
		}
		else
		{
			++it;
		}
	}
}


/**
 *	This private method adds one obstacle to its chunk space columns.
 *
 *	@return number of columns added to
 */
int ChunkModelObstacle::addToSpace( ChunkObstacle & cso )
{
	BW_GUARD;
	int numColumns = 0;

	// we can't just look at the points of the bb transformed, so we use
	// the whole bounding box transformed. this could put us in some
	// columns we're don't need to be in, but it's better than other
	// simple alternatives (really should look at edges of oriented bb,
	// but I'm too lazy and the column accessor isn't set up that way)
	BoundingBox bbTr = cso.bb_;
	bbTr.transformBy( cso.transform_ );

	const Vector3 & minTrBounds = bbTr.minBounds();
	const Vector3 & maxTrBounds = bbTr.maxBounds();

	for (int i = 0; i < 8; i++)
	{
		Vector3 pt(	(i&1) ? maxTrBounds.x : minTrBounds.x,
					(i&2) ? maxTrBounds.y : minTrBounds.y,
					(i&4) ? maxTrBounds.z : minTrBounds.z );

		ChunkSpace::Column * pColumn = this->pChunk_->space()->column( pt );

		if (pColumn)
		{
			pColumn->addObstacle( cso );
			numColumns++;
		}
	}

	if (numColumns == 0)
	{
		ERROR_MSG( "ChunkModelObstacle::addToSpace: "
				"Object is not inside the space -\n"
					"\tmin = (%.1f, %.1f, %.1f). max = (%.1f, %.1f, %.1f)\n",
			minTrBounds.x, minTrBounds.y, minTrBounds.z,
			maxTrBounds.x, maxTrBounds.y, maxTrBounds.z );
	}

	//dprintf( "ChunkModelObstacle::addToSpace: "
	//	"Adding hull of obstacle at 0x%08X (ncols %d)\n",
	//	&cso, numColumns );

	return numColumns;
}


/**
 *	This method deletes the obstacle from its chunk space columns
 */
void ChunkModelObstacle::delFromSpace( ChunkObstacle & cso )
{
	BW_GUARD;
	// find all the columns it intersects with
	ColumnSet columns;

	BoundingBox bbTr = cso.bb_;
	bbTr.transformBy( cso.transform_ );

	const Vector3 & mbt = bbTr.minBounds();
	const Vector3 & Mbt = bbTr.maxBounds();

	for (int i = 0; i < 8; i++)
	{
		Vector3 pt(	(i&1) ? Mbt.x : mbt.x,
					(i&2) ? Mbt.y : mbt.y,
					(i&4) ? Mbt.z : mbt.z );
		columns.insert( this->pChunk_->space()->column( pt, false ) );
	}

	// and flag them all as stale
	for (ColumnSet::iterator it = columns.begin(); it != columns.end(); it++)
	{
		ChunkSpace::Column * pCol = &**it;
		if (pCol == NULL) continue;

		pCol->stale();
	}
}


/// Static instance accessor initialiser
ChunkCache::Instance<ChunkModelObstacle> ChunkModelObstacle::instance;

BW_END_NAMESPACE

// chunk_model_obstacle.cpp
