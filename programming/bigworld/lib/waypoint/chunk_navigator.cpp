#include "pch.hpp"

#include "chunk_navigator.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk_waypoint.hpp"
#include "chunk_waypoint_set.hpp"
#include "girth_grid_list.hpp"
#include "navigator_find_result.hpp"

#include "math/vector3.hpp"

#include <cfloat>


BW_BEGIN_NAMESPACE

// static initialisers
ChunkNavigator::Instance< ChunkNavigator > ChunkNavigator::instance;

bool ChunkNavigator::s_useGirthGrids_ = true;

#if !defined( MF_SERVER )
/*static*/ bool ChunkNavigator::s_debugDraw = false;
/*static*/ bool ChunkNavigator::s_debugDrawConnected = false;
#endif // !defined( MF_SERVER )

/**
 *  This is the ChunkNavigator constructor.
 *
 *	@param chunk		The chunk to cache the ChunkWaypointSets.
 */
ChunkNavigator::ChunkNavigator( Chunk & chunk ) :
	chunk_( chunk ),
	wpSets_(),
	girthGrids_(),
	girthGridOrigin_(),
	girthGridResolution_( 0.f )
{
	if (s_useGirthGrids_)
	{
		// set up the girth grid parameters
		BoundingBox bb = chunk_.localBB();
		Matrix m = chunk_.transform();

		m.postMultiply( chunk_.mapping()->invMapper() );
		bb.transformBy( m );

		float maxDim = std::max( bb.maxBounds().x - bb.minBounds().x,
			bb.maxBounds().z - bb.minBounds().z );

		// extra grid square off each edge
		float oneSqProp = 1.f / (GIRTH_GRID_SIZE - 2);

		girthGridOrigin_ = Vector2( bb.minBounds().x - maxDim*oneSqProp,
			bb.minBounds().z - maxDim*oneSqProp );

		girthGridResolution_ = 1.f / (maxDim * oneSqProp);
	}

#if !defined( MF_SERVER )
	static bool firstTime = true;
	if (firstTime)
	{
		firstTime = false;
		MF_WATCH( "Client Settings/Physics/debug draw waypoints",
			s_debugDraw,
			Watcher::WT_READ_WRITE,
			"Toggle whether or not to draw navigation waypoint visualisation. "
			"Connections to another chunks are darker." );
		MF_WATCH( "Client Settings/Physics/debug draw waypoints through portals",
			s_debugDrawConnected,
			Watcher::WT_READ_WRITE,
			"If we should draw waypoint sets for connected portals as well" );
	}
#endif // !defined( MF_SERVER )
}


/**
 *  This is the ChunkNavigator destructor.
 */
ChunkNavigator::~ChunkNavigator()
{
  if (s_useGirthGrids_)
  {
	for (GirthGrids::iterator git = girthGrids_.begin();
		git != girthGrids_.end();
		++git)
	{
		bw_safe_delete_array(git->grid);
	}
	girthGrids_.clear();
  }
}


/**
 *	This is the bind method that tells all our waypoint sets about it.
 *
 *	@param isUnbind		Not used.
 */
void ChunkNavigator::bind( bool /*isUnbind*/ )
{
	PROFILER_SCOPED( ChunkNavigator_bind );
	// TODO: Should this be being called in the isUnbind == true case?
	ChunkWaypointSets::iterator it;
	for (it = wpSets_.begin(); it != wpSets_.end(); ++it)
	{
		(*it)->bind();
	}
}


/**
 *  Try a grid to see if it is a better choice
 */
void ChunkNavigator::tryGrid( GirthGridList* gridList, const WaypointSpaceVector3& point,
	float& bestDistanceSquared, int xi, int zi, NavigatorFindResult& res ) const
{
	int x = (xi);
	int z = (zi);

	if (uint(x) < GIRTH_GRID_SIZE && uint(z) < GIRTH_GRID_SIZE)
	{
		gridList[x + z * GIRTH_GRID_SIZE].findNonVisited( &chunk_, point,
			bestDistanceSquared, res );
	}
}

/**
 *  Find the waypoint and its set closest to the given point of matching
 *	girth.
 *
 *	@param wpoint			The point to test.
 *	@param girth			The girth to search with.
 *	@param res				The result of the find.
 *	@param ignoreHeight		If true then the search is done only in the x-z
 *							directions.
 *	@return					True if the search was successful, false otherwise.
 */
bool ChunkNavigator::find( const WorldSpaceVector3 & wpoint, float girth,
		NavigatorFindResult & res, bool ignoreHeight ) const
{
	GirthGrids::const_iterator git;
	for (git = girthGrids_.begin(); git != girthGrids_.end(); ++git)
	{
		if (almostEqual( git->girth, girth ))
		{
			break;
		}
	}

	GeoSpaceVector3 point = MappedVector3( wpoint, &chunk_ );

	if (git != girthGrids_.end())
	{
		// we have an appropriate girth grid
		int xg = int((point.x - girthGridOrigin_.x) * girthGridResolution_);
		int zg = int((point.z - girthGridOrigin_.y) * girthGridResolution_);
		// no need to round above conversions as always +ve
		if (uint(xg) >= GIRTH_GRID_SIZE || uint(zg) >= GIRTH_GRID_SIZE)
		{
			return false;	// hmmm
		}
		GirthGridList * gridList = git->grid;

		// try to find closest one then
		float bestDistanceSquared = FLT_MAX;

		if (gridList[xg + zg * GIRTH_GRID_SIZE].findMatchOrLower( point, res ))
		{
			ChunkWaypoint & wp = res.pSet()->waypoint( res.waypoint() );
			WaypointSpaceVector3 wv = MappedVector3( point, &chunk_ );

			if (wp.minHeight_ - 0.1f <= wv.y && wv.y <= wp.maxHeight_ + 0.1f)
			{
				return true;
			}
			else
			{
				bestDistanceSquared = wp.maxHeight_ - point.y;
				bestDistanceSquared *= bestDistanceSquared;
			}
		}

		bool result = false;

		// first try the original grid square
		tryGrid( gridList, point, bestDistanceSquared, xg, zg, res );

		// now try ever-increasing rings around it
		for (int r = 1; r < (int)GIRTH_GRID_SIZE; r++)
		{
			bool hadCandidate = (res.pSet() != NULL);

			// r is the radius of our ever-increasing square
			int xgCorner = xg - r;
			int zgCorner = zg - r;

			for (int n = 0; n < r+r; n++)
			{
				tryGrid( gridList, point, bestDistanceSquared,
					xgCorner + n  , zg - r, res );
				tryGrid( gridList, point, bestDistanceSquared,
					xgCorner + n + 1, zg + r, res );
				tryGrid( gridList, point, bestDistanceSquared,
					xg - r, zgCorner + n + 1, res );
				tryGrid( gridList, point, bestDistanceSquared,
					xg + r, zgCorner + n, res );
			}

			// if we found one in the _previous_ ring then we can stop now
			if (hadCandidate)
			{
				result = true;
				break;
			}
			// NOTE: Actually, this is not entirely true, due to the
			// way that large triangular waypoints are added to the
			// grids.  This should do until we go binary w/bsps however.
		}

		BW::vector<ChunkWaypoint*>::iterator iter;

		for (iter = ChunkWaypoint::s_visitedWaypoints_.begin();
			iter != ChunkWaypoint::s_visitedWaypoints_.end(); ++iter)
		{
			(*iter)->visited_ = 0;
		}

		ChunkWaypoint::s_visitedWaypoints_.clear();

		return result;
#undef TRY_GRID
	}

	ChunkWaypointSets::const_iterator it;
	for (it = wpSets_.begin(); it != wpSets_.end(); ++it)
	{
		if (!almostEqual( (*it)->girth(), girth ))
		{
			continue;
		}

		WaypointIndex found = (*it)->find( point, ignoreHeight );
		if (found >= 0)
		{
			res.pSet( *it );
			res.waypoint( found );
			res.exactMatch( true );
			return true;
		}
	}

	// no exact match, so use the closest one then
	float bestDistanceSquared = FLT_MAX;
	for (it = wpSets_.begin(); it != wpSets_.end(); ++it)
	{
		if (!almostEqual( (*it)->girth(), girth ))
		{
			continue;
		}

		WaypointIndex found = (*it)->find( point, bestDistanceSquared );

		if (found >= 0)
		{
			res.pSet( *it );
			res.waypoint( found );
			res.exactMatch( false );
		}
	}

	return (res.pSet() != NULL);
}


bool ChunkNavigator::findExact( const WorldSpaceVector3 & wpoint, float girth,
		NavigatorFindResult & res ) const
{
	PROFILER_SCOPED( findExact );
	GirthGrids::const_iterator git;
	for (git = girthGrids_.begin(); git != girthGrids_.end(); ++git)
	{
		if (almostEqual( git->girth, girth ))
		{
			break;
		}
	}

	GeoSpaceVector3 point = MappedVector3( wpoint, &chunk_ );

	if (git != girthGrids_.end())
	{
		// we have an appropriate girth grid
		int xg = int((point.x - girthGridOrigin_.x) * girthGridResolution_);
		int zg = int((point.z - girthGridOrigin_.y) * girthGridResolution_);
		// no need to round above conversions as always +ve
		if (uint(xg) >= GIRTH_GRID_SIZE || uint(zg) >= GIRTH_GRID_SIZE)
		{
			return false;	// hmmm
		}
		GirthGridList * gridList = git->grid;

		if (gridList[xg + zg * GIRTH_GRID_SIZE].findMatchOrLower( point, res ))
		{
			const float tolerance 
				= ChunkWaypoint::HEIGHT_RANGE_TOLERANCE + 0.01f;
			ChunkWaypoint & wp = res.pSet()->waypoint( res.waypoint() );
			WaypointSpaceVector3 wv = MappedVector3( point, &chunk_ );

			// Allow for 11cm out as when used for linking waypoint portals
			// 10cm is used, so these values may be close to equal, having
			// 11cm allows for some floating point error.
			if (wp.minHeight_ - tolerance <= wv.y &&
				wv.y <= wp.maxHeight_ + tolerance)
			{
				return true;
			}
		}

		return false;
	}

	const float INVALID_HEIGHT = 1000000.f;
	float bestHeight = INVALID_HEIGHT;
	NavigatorFindResult r;
	ChunkWaypointSets::const_iterator it;

	for (it = wpSets_.begin(); it != wpSets_.end(); ++it)
	{
		if (!almostEqual( (*it)->girth(), girth ))
		{
			continue;
		}

		for (ChunkWaypoints::size_type i = 0; i < (*it)->waypointCount(); ++i)
		{
			const ChunkWaypoint & wp = (*it)->waypoint( i );

			if (wp.minHeight_ > point.y + 0.1f)
			{
				continue;
			}

			if (wp.containsProjection( **it, point ))
			{
				if (wp.maxHeight_ + 0.1f >= point.y )
				{
					res.pSet( *it );
					res.waypoint( static_cast<WaypointIndex>(i) );
					res.exactMatch( true );

					return true;
				}

				// Try to find a reasonable navmesh if there are more than one
				// navmeshes overlapped in the same x,z range.
				// We would like to pick a relatively close navmesh with height
				// range a bit lower than y coordinate of the given point.
				if (fabsf( wp.maxHeight_ - point.y ) <
					fabsf( bestHeight - point.y ))
				{
					bestHeight = wp.maxHeight_;
					r.pSet( *it );
					r.waypoint( static_cast<WaypointIndex>(i) );
				}
			}
		}
	}

	if (!isEqual( bestHeight, INVALID_HEIGHT ))
	{
		r.exactMatch( true );
		res = r;

		return true;
	}

	return (res.pSet() != NULL);
}



/**
 *	Add the given waypoint set to our cache.
 *
 *	@param pSet			The set to add to the cache.
 */
void ChunkNavigator::add( ChunkWaypointSet * pSet )
{
	wpSets_.push_back( pSet );

	// get out now if girth grids are disabled
	if (!s_useGirthGrids_)
	{
		return;
	}

	// only use grids on outside chunks
	if (!chunk_.isOutsideChunk())
	{
		return;
	}

	GirthGridList * gridList = NULL;

	// ensure a grid exists for this girth
	GirthGrids::iterator git;
	for (git = girthGrids_.begin(); git != girthGrids_.end(); ++git)
	{
		if (almostEqual( git->girth, pSet->girth() ))
		{
			gridList = git->grid;
			break;
		}
	}

	if (git == girthGrids_.end())
	{
		girthGrids_.push_back( GirthGrid() );
		GirthGrid & girthGrid = girthGrids_.back();
		girthGrid.girth = pSet->girth();
		gridList = girthGrid.grid = new GirthGridList[GIRTH_GRID_SIZE * GIRTH_GRID_SIZE];
	}


	// add every waypoint to it
	for (ChunkWaypoints::size_type i = 0; i < pSet->waypointCount(); ++i)
	{
		const ChunkWaypoint & wp = pSet->waypoint( i );

		float minX = FLT_MAX;
		float minZ = FLT_MAX;
		float maxX = -FLT_MAX;
		float maxZ = -FLT_MAX;

		for (ChunkWaypoint::Edges::const_iterator eit = wp.edges_.begin();
				eit != wp.edges_.end();
				++eit)
		{
			const Vector2 & start = pSet->vertexByIndex( eit->vertexIndex_ );
			Vector2 gf = (start - girthGridOrigin_) * girthGridResolution_;

			minX = std::min( minX, gf.x );
			minZ = std::min( minZ, gf.y );
			maxX = std::max( maxX, gf.x );
			maxZ = std::max( maxZ, gf.y );

		}

		for (int xg = int(minX); xg <= int(maxX); ++xg)
		{
			for (int zg = int(minZ); zg <= int(maxZ); ++zg)
			{
				if (uint(xg) < GIRTH_GRID_SIZE && uint(zg) < GIRTH_GRID_SIZE)
				{
					gridList[xg + zg * GIRTH_GRID_SIZE].add( 
						GirthGridElement( pSet, static_cast<int>(i) ) );
				}
			}
		}
	}
}


/**
 *	This returns true if our chunk doesn't have any waypoint sets.
 *
 *	@return			True if the chunk has no waypoint sets, false otherwise.
 */
bool ChunkNavigator::isEmpty() const
{
	return wpSets_.empty();
}


/**
 *	This returns true if our chunk has any sets of the given girth.
 *
 *	@return			True if the chunk was any waypoint sets of the given
 *					girth, false otherwise.
 */
bool ChunkNavigator::hasNavPolySet( float girth ) const
{
	bool hasGirth = false;

	ChunkWaypointSets::const_iterator it;
	for (it = wpSets_.begin(); it != wpSets_.end(); ++it)
	{
		if (!almostEqual( (*it)->girth(), girth ))
		{
			continue;
		}
		hasGirth = true;
		break;
	}
	return hasGirth;
}


/**
 *	Delete the given waypoint set from our cache.
 *
 *	@param pSet		The set to delete from the cache.
 */
void ChunkNavigator::del( ChunkWaypointSet * pSet )
{
	ChunkWaypointSets::iterator found = std::find(
		wpSets_.begin(), wpSets_.end(), pSet );
	MF_ASSERT( found != wpSets_.end() )
	wpSets_.erase( found );

	// get out now if girth grids are disabled
	if (!s_useGirthGrids_)
	{
		return;
	}

	// only use grids on outside chunks
	if (!chunk_.isOutsideChunk())
	{
		return;
	}

	// find grid
	GirthGrids::iterator git;
	for (git = girthGrids_.begin(); git != girthGrids_.end(); ++git)
	{
		if (almostEqual( git->girth, pSet->girth() ))
		{
			break;
		}
	}

	if (git != girthGrids_.end())
	{
		// remove all traces of this set from it
		GirthGridList * gridLists = git->grid;
		for (uint i = 0; i < GIRTH_GRID_SIZE * GIRTH_GRID_SIZE; ++i)
		{
			GirthGridList & gridList = gridLists[i];

			for (uint j = 0; j < gridList.size(); ++j)
			{
				if (gridList.index( j ).pSet() == pSet)
				{
					gridList.eraseIndex( j );
					--j;
				}
			}
		}
	}
}


#if !defined( MF_SERVER )
/**
 *	Draw waypoint sets in the current camera chunk and optionally through
 *	portals to neighbouring chunks.
 */
void ChunkNavigator::drawDebug()
{
	BW_GUARD;

	if (!s_debugDraw)
	{
		return;
	}
	Moo::rc().push();

	Chunk * pCameraChunk = ChunkManager::instance().cameraChunk();

	if (pCameraChunk != NULL)
	{
		// Always draw camera chunk
		ChunkNavigator::instance( *pCameraChunk ).drawWaypointSets();

		// Draw all chunks connected to this chunk
		if (s_debugDrawConnected)
		{
			for (Chunk::piterator iPortal = pCameraChunk->pbegin();
				iPortal != pCameraChunk->pend();
				iPortal++)
			{
				if (!iPortal->hasChunk())
				{
					continue;
				}

				Chunk * pChunk = (*iPortal).pChunk;
				MF_ASSERT( pChunk != NULL );

				if (!pChunk->loaded() || !pChunk->isBound())
				{
					continue;
				}

				// Already drawn
				if (pChunk == pCameraChunk)
				{
					continue;
				}

				ChunkNavigator::instance( *pChunk ).drawWaypointSets();
			}
		}
	}

	Moo::rc().pop();
}


/**
 *	Draw all of the waypoint sets in the whole chunk.
 */
void ChunkNavigator::drawWaypointSets() const
{
	BW_GUARD;
	if (!s_debugDraw)
	{
		return;
	}

	for (ChunkWaypointSets::const_iterator setItr = wpSets_.begin();
		setItr != wpSets_.end();
		++setItr)
	{
		const ChunkWaypointSetPtr wpSet = (*setItr);
		wpSet->drawDebug();
	}
}
#endif // !defined( MF_SERVER )

BW_END_NAMESPACE

// chunk_waypoint_set.cpp
