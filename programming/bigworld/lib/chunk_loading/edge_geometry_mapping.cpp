#include "edge_geometry_mapping.hpp"

#include "edge_geometry_mappings.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_loader.hpp"
#include "chunk/chunk_overlapper.hpp"
#include "chunk/chunk_space.hpp"

#include "math/boundbox.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: EdgeGeometryMapping
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EdgeGeometryMapping::EdgeGeometryMapping( ChunkSpacePtr pSpace,
		const Matrix & m, const BW::string & path, DataSectionPtr pSettings,
		EdgeGeometryMappings & mappings, const Vector3 * pInitialPoint ) :
	GeometryMapping( pSpace, m, path, pSettings ),
	currLoadedAll_( false ),
	mappings_( mappings )
{
	if (this->pSpace() == NULL)
	{
		// GeometryMapping's constructor failed.
		return;
	}

	if (!pSettings)
	{
		pSettings = this->openSettings( path );
	}

	// GeometryMapping fails if this is false
	MF_ASSERT( pSettings );

	Vector3 lpoint( 0,0,0 );
	if (pInitialPoint)
	{
		lpoint = this->invMapper().applyPoint( *pInitialPoint );
	}

	BW::RectInt bounds( this->minLGridX(), this->minLGridY(),
			this->maxLGridX() + 1, this->maxLGridY() + 1 );

	// Clamp in float first to avoid integer overflow
	int xGrid = (int)Math::clamp( float( bounds.xMin() ),
									floorf( lpoint.x/pSpace->gridSize() ),
									float( bounds.xMax() ) );
	int zGrid = (int)Math::clamp( float( bounds.yMin() ),
									floorf( lpoint.z/pSpace->gridSize() ),
									float( bounds.yMax() ) );

	edge_[ LEFT   ].init( 0, xGrid, zGrid, std::max( bounds.xMin(), xGrid - 2 ) );
	edge_[ BOTTOM ].init( 1, zGrid, xGrid, std::max( bounds.yMin(), zGrid - 2 ) );
	edge_[ RIGHT  ].init( 2, xGrid, zGrid, std::min( bounds.xMax(), xGrid + 2 ) );
	edge_[ TOP    ].init( 3, zGrid, xGrid, std::min( bounds.yMax(), zGrid + 2 ) );
}


/**
 *	Destructor.
 */
EdgeGeometryMapping::~EdgeGeometryMapping()
{
	mappings_.removeMapping( this );
}


/**
 *	This method returns whether this mapping is newly, fully loaded.
 */
bool EdgeGeometryMapping::hasFullyLoaded()
{
	if (currLoadedAll_)
	{
		return false;
	}

	bool fullyLoaded = edge_[0].isFullyLoaded() &&
						edge_[1].isFullyLoaded() &&
						edge_[2].isFullyLoaded() &&
						edge_[3].isFullyLoaded();

	if (fullyLoaded)
	{
		currLoadedAll_ = true;
	}

	return fullyLoaded;
}


/**
 *	This method loads or unloads this Chunks in this mapping.
 *
 *	@return True if any columns (including outside chunk and all overlappers)
 *		have been bound in this tick.
 */
bool EdgeGeometryMapping::tick( const Vector3 & minB, const Vector3 & maxB,
		bool unloadOnly )
{
	PROFILER_SCOPED( EdgeGeometryMapping_tick );

	if (this->condemned())
	{
		// Always shrink when condemned.
		for (size_t i = 0; i < ARRAY_SIZE(edge_); i++)
		{
			edge_[i].tickCondemned( this );
		}

		// Unloading, so nothing gets bound
		return false;
	}


	bool anyColumnsLoaded = false;

	BoundingBox bb( minB, maxB );
	bb.transformBy( this->invMapper() );

	// turn it into grid squares and clamp it to our space bounds
	int lim[4];
	lim[ LEFT ] = Math::clamp( this->minLGridX(),
			int( floorf( bb.minBounds().x / pSpace()->gridSize() ) ),
			this->maxLGridX()+1 );
	lim[ BOTTOM ] = Math::clamp( this->minLGridY(),
			int( floorf( bb.minBounds().z / pSpace()->gridSize() ) ),
			this->maxLGridY()+1 );
	lim[ RIGHT ] = Math::clamp( this->minLGridX(),
			int( ceilf( bb.maxBounds().x / pSpace()->gridSize()) ),
			this->maxLGridX()+1 );
	lim[ TOP ] = Math::clamp( this->minLGridY(),
			int( ceilf( bb.maxBounds().z / pSpace()->gridSize()) ),
		   	this->maxLGridY()+1 );

	// expand the rect as quick as you like, but only contract it gradually
	int unchanged = 0;
	int antiHyst = 10;	// 10 chunks worth of antihysteresis

	if (unloadOnly)
	{
		antiHyst = 0;
		// figure out which axis has the largest front (repeatable calc)
		int little = (lim[2]-lim[0] > lim[3]-lim[1]);
		// collapse that axis
		lim[little|2] = lim[little];
		// and reduce the front to at most 2 grid squares
		lim[(little^1)|2] = std::min( lim[little^1]+2, lim[(little^1)|2] );
	}

	for (int i = 0; i < 4; i++)
	{
		if (!edge_[i].clipDestTo( lim[i], antiHyst ))
		{
			++unchanged;
		}
	}

	if (unchanged != 4)
	{
		DEBUG_MSG( "EdgeGeometryMapping(%s)::tick(): "
			"New full bounds (%d,%d)-(%d,%d)\n", this->name().c_str(),
			edge_[0].destPos(), edge_[1].destPos(),
			edge_[2].destPos(), edge_[3].destPos() );
	}


	// go through every line - left, bottom, right, top
	for (int i = 0; i < 4; i++)
	{
		currLoadedAll_ &=
			edge_[i].tick( this, edge_[(i^1)&1], edge_[(i^1)|2],
					edge_[i^2], anyColumnsLoaded );
	}

	return anyColumnsLoaded;
}


/**
 *	This method binds the given chunk.
 */
void EdgeGeometryMapping::bind( Chunk * pChunk )
{
	pChunk->bind( /* shouldFormPortalConnections */ true );
	this->postBoundChunk( pChunk );
}


/**
 *	This temporary method does stuff to the chunk after it has been bound.
 *	This method shouldn't really be necessary.
 */
void EdgeGeometryMapping::postBoundChunk( Chunk * pChunk )
{
	//TRACE_MSG( "Bound chunk '%s'\n", pChunk->identifier().c_str() );

	ChunkSpace * pChunkSpace = pChunk->space();
	pChunkSpace->removeFromBlurred( pChunk );

	MF_ASSERT( !pChunk->focussed() );

	// do the actual focussing work
	pChunk->focus();
	pChunkSpace->seeChunk( pChunk );

	// This is no longer required as the outside chunk is only bound once all
	// overlappers have been loaded.
#if 0
	// check if entities in corresponding outside chunks
	// would prefer to live here
	// (this will eventually be accomplished with dynamic chunk items instead)
	if (!pChunk->isOutsideChunk())
	{
		const BoundingBox & bb = pChunk->boundingBox();

		for (float x = floorf( bb.minBounds().x / pSpace()->gridSize() );
			x <= floorf( bb.maxBounds().x / pSpace()->gridSize() );
			x += 1.f)
		{
			for (float z = floorf( bb.minBounds().z / pSpace()->gridSize() );
				z <= floorf( bb.maxBounds().z / pSpace()->gridSize() );
				z += 1.f)
			{
				// for x over x columns; for y over y columns
				ChunkSpace::Column * pCol = pChunkSpace->column(
					Vector3( x * pSpace()->gridSize(), 0.f, z * pSpace()->gridSize() ),
					false );
				if (pCol != NULL && pCol->pOutsideChunk() != NULL)
				{
					CellChunk & oc = CellChunk::instance( *pCol->pOutsideChunk() );

					CellChunk::EntityIterator iter = oc.begin();
					CellChunk::EntityIterator end = oc.end();

					while (iter != oc.end())
					{
						Entity * pEntity = *iter;
						// Increment first as checkChunkCrossing can change
						// Entity::nextInChunk.
						iter++;

						pEntity->checkChunkCrossing();
					}
				}
			}
		}
	}
#endif
}


/**
 *	This method unbindsk and unloads the given chunk.
 */
void EdgeGeometryMapping::unbindAndUnload( Chunk * pChunk )
{
	this->preUnbindChunk( pChunk );
	pChunk->unbind( false );
	pChunk->unload();
}


/**
 *	Debugging helper called before unbinding a chunk.
 */
void EdgeGeometryMapping::preUnbindChunk( Chunk * pChunk )
{
	MF_ASSERT( pChunk->isBound() );
}


/**
 *	This method returns whether all desired chunks have been loaded.
 */
bool EdgeGeometryMapping::isFullyUnloaded() const
{
	// only one axis need be fully collapsed
	if (edge_[0].destPos() != edge_[2].destPos() &&
		edge_[1].destPos() != edge_[3].destPos())
	{
		return false;
	}

	// check that each direction is as far as it wants to go
	for (int i = 0; i < 4; i++)
	{
		// see if have arrived at desired line
		if (!edge_[i].isFullyLoaded())
		{
			return false;
		}

		// see if in the middle of loading (very important case!)
		if (edge_[i].isLoading())
		{
			return false;
		}
	}

	return true;
}


/**
 *	This method returns the largest rectangle containing only loaded chunks.
 */
void EdgeGeometryMapping::calcLoadedRect( BW::Rect & loadedRect,
	   bool isFirst, bool isLast ) const
{
	const float veryBigNumber = 1000000000000.f;
	// use a pretty big number (in case scaled or offset or some such)
	const float prettyBigNumber = veryBigNumber / 1000.f;

	if (isFirst)
	{
		loadedRect = BW::Rect( -veryBigNumber, -veryBigNumber,
			veryBigNumber, veryBigNumber );
	}

	Vector3 minPt( float(edge_[0].currPos()), 0, float(edge_[1].currPos()) );
	Vector3 maxPt( float(edge_[2].currPos()), 0, float(edge_[3].currPos()) );

	minPt *= pSpace()->gridSize();
	maxPt *= pSpace()->gridSize();

	if (edge_[0].currPos() == this->minLGridX())
			minPt.x = -veryBigNumber;
	if (edge_[1].currPos() == this->minLGridY())
			minPt.z = -veryBigNumber;

	if (edge_[2].currPos() == this->maxLGridX() + 1)
			maxPt.x = veryBigNumber;
	if (edge_[3].currPos() == this->maxLGridY() + 1)
			maxPt.z = veryBigNumber;

	// Now transform that local rect into space coordinates.
	// For now we always assume that any rotations will be multiples
	// of 90 degrees around the Y axis
	minPt = this->mapper().applyPoint( minPt );
	maxPt = this->mapper().applyPoint( maxPt );

	BW::Rect oner;
	oner.xMin( std::min( minPt.x, maxPt.x ) );
	oner.yMin( std::min( minPt.z, maxPt.z ) );
	oner.xMax( std::max( minPt.x, maxPt.x ) );
	oner.yMax( std::max( minPt.z, maxPt.z ) );

	// if it's the first then it's easy.
	if (isFirst)
	{
		loadedRect = oner;
	}
	// otherwise we have to intersect it with the current rect
	else
	{
		// we do a strange intersection - if any of the mappings are
		// loaded to the edge (and thus go to infinity), they never
		// cut off that edge of the rectangle... yep, but it'll do.
		if (loadedRect.xMin() > -prettyBigNumber)
		{
			if (oner.xMin() > -prettyBigNumber)
				loadedRect.xMin( std::max(loadedRect.xMin(),oner.xMin()) );
			else
				loadedRect.xMin( oner.xMin() );
		}
		if (loadedRect.yMin() > -prettyBigNumber)
		{
			if (oner.yMin() > -prettyBigNumber)
				loadedRect.yMin( std::max(loadedRect.yMin(),oner.yMin()) );
			else
				loadedRect.yMin( oner.yMin() );
		}
		if (loadedRect.xMax() < prettyBigNumber)
		{
			if (oner.xMax() < prettyBigNumber)
				loadedRect.xMax( std::min(loadedRect.xMax(),oner.xMax()) );
			else
				loadedRect.xMax( oner.xMax() );
		}
		if (loadedRect.yMax() < prettyBigNumber)
		{
			if (oner.yMax() < prettyBigNumber)
				loadedRect.yMax( std::min(loadedRect.yMax(),oner.yMax()) );
			else
				loadedRect.yMax( oner.yMax() );
		}
	}

	if (isLast)
	{
		// Set values to FLT_MAX if range is unbounded.
		if (loadedRect.xMin_ < -prettyBigNumber) loadedRect.xMin_ = -FLT_MAX;
		if (loadedRect.yMin_ < -prettyBigNumber) loadedRect.yMin_ = -FLT_MAX;
		if (loadedRect.xMax_ >  prettyBigNumber) loadedRect.xMax_ =  FLT_MAX;
		if (loadedRect.yMax_ >  prettyBigNumber) loadedRect.yMax_ =  FLT_MAX;
	}
}


/**
 *  This method adds the loadable bounds of this mapping to the rects
 *  list provided as a parameter.
 *  Returns false if bounds are not loaded yet, otherwise returns true.
 */
bool EdgeGeometryMapping::getLoadableRects( BW::list< BW::Rect > & rects ) const
{
	rects.push_back( BW::Rect(
		this->minLGridX(),
		this->minLGridY(),
		this->maxLGridX(),
		this->maxLGridY())
	);

	return true;
}


/**
 *	This method is called during shutdown to take care of chunks currently
 *	loading.
 */
void EdgeGeometryMapping::prepareNewlyLoadedChunksForDelete()
{
	int numCols = sizeof(edge_)/ sizeof(edge_[0]);

	for (int i = 0; i < numCols; ++i)
	{
		edge_[i].prepareNewlyLoadedChunksForDelete( this );
	}
}


/**
 *	This method returns the outside chunk at the given grid.
 */
Chunk * EdgeGeometryMapping::findOrAddOutsideChunk( int xGrid, int yGrid )
{
	BW::string id = this->outsideChunkIdentifier( xGrid, yGrid );

	return this->pSpace()->findOrAddChunk( new Chunk( id, this ) );

}


/**
 *	This method returns the outside chunk at the given grid.
 */
Chunk * EdgeGeometryMapping::findOutsideChunk( int xGrid, int yGrid )
{
	BW::string id = this->outsideChunkIdentifier( xGrid, yGrid );

	return this->pSpace()->findChunk( id, this );

}


/**
 *	This method returns a float value representing load progress, 
 *  in [0-1] range.
 */
float EdgeGeometryMapping::getLoadProgress() const
{
	float totalW = this->maxLGridX() - this->minLGridX() + 1;
	float totalH = this->maxLGridY() - this->minLGridY() + 1;
	float totalArea = totalW * totalH;

	// +1 already included in outer edge positions.
	float loadedW = edge_[2].currPos() - edge_[0].currPos();
	float loadedH = edge_[3].currPos() - edge_[1].currPos();	

	return loadedW * loadedH / totalArea;
}


BW_END_NAMESPACE

// edge_geometry_mapping.cpp
