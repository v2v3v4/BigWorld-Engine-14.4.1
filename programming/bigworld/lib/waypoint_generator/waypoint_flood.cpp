#include "pch.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/debug.hpp"

#include "waypoint_flood.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"

#include <time.h>

#include <stack>

DECLARE_DEBUG_COMPONENT2( "WayPoint", 0 )

//#define DEBUG_WAYPOINT_FLOOD


BW_BEGIN_NAMESPACE

class LogFile
{
	static HANDLE file_;
public:
	LogFile( const char* filename )
	{
		BW_GUARD;

#ifdef DEBUG_WAYPOINT_FLOOD
		if( file_ == INVALID_HANDLE_VALUE )
		{
			char s[1024];
			bw_snprintf( s, sizeof( s ), filename, time( NULL ) );
			file_ = CreateFileA( s, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
		}
		SetFilePointer( file_, 0, NULL, FILE_END );
		println( 0, "************************ Start Log ************************" );
#endif//DEBUG_WAYPOINT_FLOOD
	}
	void print( const char* format, ... )
	{
		BW_GUARD;

#ifdef DEBUG_WAYPOINT_FLOOD
		va_list args;
		va_start( args, format );

		char buffer[10240];
		vsprintf_s( buffer, sizeof( buffer ), format, args );

		DWORD bytesWritten;
		WriteFile( file_, buffer, strlen( buffer ), &bytesWritten, NULL );
#endif//DEBUG_WAYPOINT_FLOOD
	}
	void println( int indent, const char* format, ... )
	{
		BW_GUARD;

#ifdef DEBUG_WAYPOINT_FLOOD
		va_list args;
		va_start( args, format );

		char buffer[10240];
		static const int INDENT = 4;
		std::fill( buffer, buffer + indent * INDENT, ' ' );
		vsprintf_s( buffer + indent * INDENT, sizeof( buffer ) - indent * INDENT, format, args );
		strcat( buffer, "\r\n" );

		DWORD bytesWritten;
		WriteFile( file_, buffer, strlen( buffer ), &bytesWritten, NULL );
#endif//DEBUG_WAYPOINT_FLOOD
	}
	~LogFile()
	{
		BW_GUARD;

		println( 0, "************************ End Log ************************" );
	}
};

HANDLE LogFile::file_ = INVALID_HANDLE_VALUE;

// Floating point fudge factor
#define FLOOD_FLOAT_FUDGE	0.01f
#define HEIGHT_THRESHOLD 1.0f
#define INITIAL_HEIGHT -10000.0f

// All points are offset by this amount, to stop findDropPoint
// from falling through the cracks between polygons. When we
// switch over to the new collision code, this will no longer
// be necessary.
#define HACK_OFFSET 0.1f


/**
 *	This is the constructor.
 */
WaypointFlood::WaypointFlood() :
	resolution_( 0.0f ),
	xsize_( 0 ),
	zsize_( 0 ),
	size_(),
	pPhysics_( NULL ),
	pChunk_(),
	smallestHeight_( MAX_HEIGHTS )
{
	BW_GUARD;

	for ( uint g = 0; g < MAX_HEIGHTS; ++g )
	{
		adjGrids_[g] = NULL;
		hgtGrids_[g] = NULL;
	}
}

/**
 *	This is the destructor.
 */
WaypointFlood::~WaypointFlood()
{
	BW_GUARD;

	for ( uint g = 0; g < MAX_HEIGHTS; ++g )
	{
		bw_safe_delete_array(adjGrids_[g]);
		bw_safe_delete_array(hgtGrids_[g]);
	}
}

/**
 * set the pChunk to flood.
 */
void WaypointFlood::setChunk( Chunk * pChunk )
{
	pChunk_ = pChunk;
}

/**
 *	This method allocates the space needed for the adjacency bitmap.
 *	It takes a bounding box, and a sampling resolution.
 *
 *	@param min	Minimum point in bounding box
 *	@param max	Maximum point in bounding box
 *	@param resolution	Sampling resolution
 *
 *	@retun True if successful, false if allocation failed.
 */
bool WaypointFlood::setArea( const Vector3& min, const Vector3& max, float resolution )
{
	BW_GUARD;

	for ( uint g = 0; g < MAX_HEIGHTS; ++g )
	{
		bw_safe_delete_array(adjGrids_[g]);
		bw_safe_delete_array(hgtGrids_[g]);
	}

	min_ = min;
	max_ = max;
	resolution_ = resolution;

	if (!almostEqual( int( min_.x / resolution_ ) * resolution_, min_.x ))
	{
		min_.x = int( min_.x / resolution_ ) * resolution_;
	}

	if (!almostEqual( int( min_.z / resolution_ ) * resolution_, min_.z ))
	{
		min_.z = int( min_.z / resolution_ ) * resolution_;
	}

	if (!almostEqual( int( max_.x / resolution_ ) * resolution_, max_.x ))
	{
		max_.x = int( max_.x / resolution_ + 1 ) * resolution_;
	}

	if (!almostEqual( int( max_.z / resolution_ ) * resolution_, max_.z ))
	{
		max_.z = int( max_.z / resolution_ + 1 ) * resolution_;
	}

	xsize_ = int((max.x - min.x) / resolution)+1;
	zsize_ = int((max.z - min.z) / resolution)+1;
	size_ = xsize_ * zsize_;

	for ( uint g = 0; g < MAX_HEIGHTS; ++g )
	{
		adjGrids_[g] = new AdjGridElt[size_];
		hgtGrids_[g] = new float[size_];
	}

	// Slight hack, so that we don't do our movement checks and drop tests on
	// integer boundaries. This is a workaround for a bug in the collision code.
	min_.x += HACK_OFFSET;
	max_.x += HACK_OFFSET;
	min_.z += HACK_OFFSET;
	max_.z += HACK_OFFSET;

	for ( uint g = 0; g < MAX_HEIGHTS; ++g )
	{
		memset( adjGrids_[g], 0, size_*sizeof(AdjGridElt) );

		for ( int i = 0; i < size_; ++i )
			hgtGrids_[g][i] = INITIAL_HEIGHT;
	}
		
	return true;
}

/**
 *	This method sets the physics interface used to find drop
 *	points and adjust movement.
 *
 *	@param pPhysics Pointer to an object that implements IPhysics
 */
void WaypointFlood::setPhysics( IPhysics* pPhysics )
{
	pPhysics_ = pPhysics;
}


/**
 *	This method ensures that the floodfill has not left the
 *	bounds of the initial chunk (make sure src and dst are
 *  in the current chunk). It is allowed to move one
 *	grid square outside the chunk, so that adjacent chunks
 *	will overlap slightly.
 */
bool WaypointFlood::checkBounds( Vector3 src, Vector3 dst )
{
	BW_GUARD;

	dst.y = src.y;

	for (int i = 0; i < 16; ++i)
	{
		Vector3 isrc( src );
		Vector3 idst( dst );

		if (i & 1)
		{
			isrc.x += resolution_;
		}
		else
		{
			isrc.x -= resolution_;
		}

		if (i & 2)
		{
			isrc.z += resolution_;
		}
		else
		{
			isrc.z -= resolution_;
		}

		if (i & 4)
		{
			idst.x += resolution_;
		}
		else
		{
			idst.x -= resolution_;
		}

		if (i & 8)
		{
			idst.z += resolution_;
		}
		else
		{
			idst.z -= resolution_;
		}

		if (pChunk_->owns( isrc ) &&
			pChunk_->owns( idst ))
		{
			return true;
		}

		// raise y a bit to avoid collision error in some conditions
		isrc.y += pPhysics_->getScrambleHeight();
		idst.y += pPhysics_->getScrambleHeight();

		if (pChunk_->owns( isrc ) &&
			pChunk_->owns( idst ))
		{
			return true;
		}
	}
	return false;
}

/**
 *	This method performs a flood fill, starting at a given
 *	seed point. It can be called multiple times with different
 *	seed points, if not all regions are reachable from one
 *	another.
 *
 *	@param seedPoint	A point within the area.
 *	@param pProgress	Optional progress callback object
 *	@return				The total number of points processed, -1 if should stop
 */
int WaypointFlood::fill( const Vector3& seedPoint, IProgress * pProgress, bool debug )
{
	BW_GUARD;

	// Setup
	Vector3 v1(441.1f, 0.2f, 80.6f);
	Vector3 v2(441.6f, 0.2f, 81.1f);
	bool b1 = pChunk_->owns( v1 );
	bool b2 = pChunk_->owns( v2 );
	this->checkBounds( v1, v2 );

	LogFile log( "wpfill%d.log" );

	log.println( 0, "get seed point %g %g %g", seedPoint.x, seedPoint.y, seedPoint.z );

	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	
	// Angles are:
	//
	// 701 345
	// 6 2 2 6
	// 543 107

	int g, angle, count = 0;
	int src_x, src_z, dst_x, dst_z, src_index, dst_index, src_grid, dst_grid;
	std::stack<std::pair<Vector3,int>, BW::vector<std::pair<Vector3,int> > > floodStack;
	Vector3 dst, src2, dst2;
	float dropPoint;

	Vector3 src;
	src.x = (int)( ( seedPoint.x - min_.x ) / resolution_) * resolution_ + min_.x;
	src.z = (int)( ( seedPoint.z - min_.z ) / resolution_) * resolution_ + min_.z;
	src.y = seedPoint.y + 0.1f;

	//HACK_MSG("Seed point: (%f %f) (%f %f)\n", 
	//	seedPoint.x, seedPoint.z,
	//	src.x, src.z);

	log.println( 1, "round to %g %g %g", src.x, src.y, src.z );

	// drop the source point
	if ( !pPhysics_->findDropPoint(src, dropPoint) )
	{
		if ( pProgress != NULL )
		{
			WARNING_MSG("WaypointFlood: "
				"failed to find drop point from (%f,%f,%f)\n",
				src.x, src.y, src.z);
		}
		log.println( 1, "drop point failure" );
		return 0;
	}

	//HACK_MSG( "Dropped from (%f,%f,%f) to %f\n",
	//	src.x, src.y, src.z, dropPoint );

	src.y = dropPoint;

	if (BgTaskManager::shouldAbortTask())
	{
		return -1;
	}

	if ( !this->checkBounds( src, src ) )
	{
		src.x += seedPoint.x >= 0.0 ? resolution_ : -resolution_;
		src.y = seedPoint.y + 0.1f;
		if ( ! ( pPhysics_->findDropPoint(src, dropPoint) &&
			( src.y = dropPoint, true ) &&
			this->checkBounds( src, src ) ) )
		{
			src.x -= seedPoint.x >= 0.0 ? resolution_ : -resolution_;
			src.z += seedPoint.z >= 0.0 ? resolution_ : -resolution_;
			src.y = seedPoint.y + 0.1f;
			if ( ! ( pPhysics_->findDropPoint(src, dropPoint) &&
				( src.y = dropPoint, true ) &&
				this->checkBounds( src, src ) ) )
			{
				src.x += seedPoint.x >= 0.0 ? resolution_ : -resolution_;;
				src.y = seedPoint.y + 0.1f;
				if ( ! ( pPhysics_->findDropPoint(src, dropPoint) &&
					( src.y = dropPoint, true ) &&
					this->checkBounds( src, src ) ) )
				{
					log.println( 1, "check bounds failure" );
					return 0;
				}
			}
		}
	}

	if (BgTaskManager::shouldAbortTask())
	{
		return -1;
	}

	// Make sure our initial point is in some grid
	src_x = (int)((src.x - min_.x) / resolution_ + 0.5f);
	src_z = (int)((src.z - min_.z) / resolution_ + 0.5f);
	if (src_x < 0 || src_x >= xsize_ || src_z < 0 || src_z >= zsize_)
	{
		ERROR_MSG( "WaypointFlood: Seed source point is outside grid!\n" );
		log.println( 1, "seed source point is outside grid" );
		return 0;
	}

	src_index = src_x + src_z * xsize_;

	log.println( 1, "drop to %g --- src_x: %d  src_z: %d  src_index: %d", dropPoint, src_x, src_z, src_index );

	int bestG = 0;
	float minGDist = 100000.f;
	for ( g = MAX_HEIGHTS - 1; g > 0; --g )
	{
		if (fabs(hgtGrids_[g][src_index] - src.y) < HEIGHT_THRESHOLD &&
			pPhysics_->isUnblocked(src,hgtGrids_[g][src_index]))
		{
			if( fabs(hgtGrids_[g][src_index] - src.y) < minGDist )
			{
				minGDist = fabs(hgtGrids_[g][src_index] - src.y);
				bestG = g;
			}
		}
	}
	g = bestG;

	if (BgTaskManager::shouldAbortTask())
	{
		return -1;
	}

	// Add it to an available grid if it is not
	if ( g == 0 )
	{
		for ( g = MAX_HEIGHTS - 1; g > 0; --g )
		{
			if ( hgtGrids_[g][src_index] == INITIAL_HEIGHT )
			{
				break;
			}
		}

		if ( g == 0 )
		{
			log.println( 1, "More than %d different heights on (%g, %g, %g)!",
				MAX_HEIGHTS - 1, src.x, src.y, src.z );
			ERROR_MSG( "WaypointFlood: More than %d different heights on (%g, %g, %g)!\n",
				MAX_HEIGHTS - 1, src.x, src.y, src.z );
			return 0;
		}
		hgtGrids_[g][src_index] = src.y;

		if (g < smallestHeight_)
		{
			smallestHeight_ = g;
		}

		log.println( 1, "got new height: %g    grid: %d", hgtGrids_[g][src_index], g );
	}
	else
		log.println( 1, "height: %g    grid: %d", hgtGrids_[g][src_index], g );
	src_grid = g;
	if ( adjGrids_[src_grid][src_index].all == 0 )
		floodStack.push( std::make_pair( src, src_grid ) );
	else
	{
		log.println( 1, "already filled, all: %u", adjGrids_[src_grid][src_index].all );
		return 0;
	}

	int saveG = src_grid;
	int saveIndex = src_index;
	bool gotOne = false;

	// Now seed and expand the stack
	while ( !floodStack.empty() )
	{
		if (BgTaskManager::shouldAbortTask())
		{
			return -1;
		}

		src = floodStack.top().first;
		src_grid = floodStack.top().second;
		floodStack.pop();

		++count;
		if ( (count % 200) == 0 )
		{
			if (pProgress != NULL)
			{
				INFO_MSG("Processed %d points (current stack size: %d)\n", 
					count, floodStack.size());

				if ( pProgress->filled( count ) )
					return -1; // request stop processing
			}
		}

		src_x = (int)((src.x - min_.x) / resolution_ + 0.5f);
		src_z = (int)((src.z - min_.z) / resolution_ + 0.5f);
		src_index = src_x + src_z * xsize_;

		log.println( 1, "************************ new point ************************" );
		log.println( 1, "pop point: %g %g %g  src_x: %d  src_z: %d  src_index: %d src_grid: %d  height: %g",
			src.x, src.y, src.z, src_x, src_z, src_index, src_grid, hgtGrids_[src_grid][src_index] );

		//HACK_MSG( "Flooding from (%d,%d) i.e. (%f,%f)\n",
		//	src_x, src_z, src.x, src.z );

		for ( angle = 0; angle < 8; ++angle )
		{
			if( adjGrids_[src_grid][src_index].angle( angle ) != 0 )
				continue;

			dst_x = src_x + dx[angle];
			dst_z = src_z + dy[angle];
			dst_index = dst_x + dst_z * xsize_;

			dst.x = min_.x + dst_x * resolution_;
			dst.z = min_.z + dst_z * resolution_;
			dst.y = src.y;

			// not on grid.
			if ( dst_x < 0 || dst_x >= xsize_ || dst_z < 0 || dst_z >= zsize_ )
			{
				continue;
			}

            // not in space
			if ( dst.x < pChunk_->space()->minCoordX() ||
				dst.x > pChunk_->space()->maxCoordX() ||
				dst.z < pChunk_->space()->minCoordZ() ||
				dst.z > pChunk_->space()->maxCoordZ() )
			{
				continue;
			}

			// If we have already checked this adjacency from
			// the other side, no need to check it again.
			/*
			if ( adjGrids_[src_grid][src_index].angle( angle ) )
			{
				continue;
			}
			*/

			// Check that movement is possible from src to dst
			pPhysics_->adjustMove(src, dst, dst2);
			if ( fabs(dst.x - dst2.x) > FLOOD_FLOAT_FUDGE ||
		  	     fabs(dst.z - dst2.z) > FLOOD_FLOAT_FUDGE ||
                 fabs(dst.y - dst2.y) > HEIGHT_THRESHOLD * 10 )
			{
				log.println( 1, "adjustMove for angle %d: dst_x: %d dst_z: %d dst: %g %g %g  dst2: %g %g %g failed",
					angle, dst_x, dst_z, dst.x, dst.y, dst.z, dst2.x, dst2.y, dst2.z );
				continue;
			}
			log.println( 1, "adjustMove for angle %d: dst_x: %d dst_z: %d dst: %g %g %g  dst2: %g %g %g succeeded",
				angle, dst_x, dst_z, dst.x, dst.y, dst.z, dst2.x, dst2.y, dst2.z );

			// Now we know the correct y value for the destination,
			dst.y = dst2.y;
			// so we can determine which grid it should occupy.

			int bestG = 0;
			float minGDist = 100000.f;
			for ( g = MAX_HEIGHTS - 1; g > 0 ; --g )
			{
				if (fabs(hgtGrids_[g][dst_index] - dst.y) < HEIGHT_THRESHOLD &&
					pPhysics_->isUnblocked(dst,hgtGrids_[g][dst_index]))
				{
					if( fabs(hgtGrids_[g][src_index] - src.y) < minGDist )
					{
						minGDist = fabs(hgtGrids_[g][src_index] - src.y);
						bestG = g;
					}
				}
			}
			g = bestG;

			if ( g == 0 )
			{
				for ( g = MAX_HEIGHTS - 1; g > 0; --g )
				{
					if (hgtGrids_[g][dst_index] == INITIAL_HEIGHT)
					{
						break;
					}
				}
				if ( g == 0 )
				{
					log.println( 1, "More than %d different heights on (%g, %g, %g)!",
						MAX_HEIGHTS - 1, dst.x, dst.y, dst.z );
					ERROR_MSG( "WaypointFlood: More than %d different heights on (%g, %g, %g)!\n",
						MAX_HEIGHTS - 1, dst.x, dst.y, dst.z );
					continue;
				}
				log.println( 1, "got new grid : %d", g );
				// we set the height up later
				//hgtGrids_[g][dst_index] = dst.y;
				if ( adjGrids_[g][dst_index].all != 0 )
				{
					CRITICAL_MSG( "In grid %d index %d (%d,%d), "
						"height is %f but adj is %u!\n",
						g, dst_index, dst_x, dst_z,
						hgtGrids_[g][dst_index],
						adjGrids_[g][dst_index].all );
				}
			}
			else
				log.println( 1, "got existing grid : %d  height: %g", g, hgtGrids_[g][dst_index] );
			
			dst_grid = g;

			// Now try moving back again, from dst to src
			pPhysics_->adjustMove(dst2, src, src2);

			if( (fabs(src.x - src2.x) > FLOOD_FLOAT_FUDGE ||
			      fabs(src.z - src2.z) > FLOOD_FLOAT_FUDGE) ||
			      fabs(src.y - src2.y) > 0.1f )
			{
				log.println( 1, "adjustMove Back: src: %g %g %g  src2: %g %g %g failed", src.x, src.y, src.z, src2.x, src2.y, src2.z );
				continue;
			}
			log.println( 1, "adjustMove Back: src: %g %g %g  src2: %g %g %g succeeded", src.x, src.y, src.z, src2.x, src2.y, src2.z );

			log.println( 1, "adjGrids_[dst_grid][dst_index]: %u  src_grid: %d", adjGrids_[dst_grid][dst_index].angle( angle ^ 4 ), src_grid );

			if( adjGrids_[dst_grid][dst_index].angle( angle ^ 4 ) != 0 &&
				adjGrids_[dst_grid][dst_index].angle( angle ^ 4 ) != src_grid )
			{
				for ( g = MAX_HEIGHTS - 1; g > 0 ; --g )
				{
					if (hgtGrids_[g][dst_index] == INITIAL_HEIGHT)
					{
						break;
					}
				}
				if ( g == 0 )
				{
					log.println( 1, "More than %d different heights on (%g, %g, %g)!",
						MAX_HEIGHTS - 1, dst.x, dst.y, dst.z );
					ERROR_MSG( "WaypointFlood: More than %d different heights on (%g, %g, %g)!\n",
						MAX_HEIGHTS - 1, dst.x, dst.y, dst.z );
					continue;
				}
				// we set the height up later
				//hgtGrids_[g][dst_index] = dst.y;
				if ( adjGrids_[g][dst_index].all != 0 )
				{
					CRITICAL_MSG( "In grid %d index %d (%d,%d), "
						"height is %f but adj is %u!\n",
						g, dst_index, dst_x, dst_z,
						hgtGrids_[g][dst_index],
						adjGrids_[g][dst_index].all );
				}
				dst_grid = g;

				// Now try moving back again, from dst to src
				pPhysics_->adjustMove(dst2, src, src2);
				if( (fabs(src.x - src2.x) > FLOOD_FLOAT_FUDGE ||
					fabs(src.z - src2.z) > FLOOD_FLOAT_FUDGE) ||
					fabs(src.y - src2.y) > 0.1f )
				{
					log.println( 1, "Shall NEVER fail here!" );
					continue;
				}
			}
			// If the destination has no adjacencies yet, add it to
			// the stack, so we'll process it later.
			if ( adjGrids_[dst_grid][dst_index].all == 0 )
			{
				// Unless it's out of bounds, in which case do not add it at all
				if ( !this->checkBounds( src, dst ) )
				{
					boundarySet_.insert( std::make_pair( src_index, src_grid ) );
					log.println( 1, "Check Bounds Failure on %s - %s!",
						src.desc().c_str(), dst.desc().c_str() );
					continue;
				}
				log.println( 1, "dst_grid: %d  dst_index: %d  height: %g", dst_grid, dst_index, dst.y );
				hgtGrids_[dst_grid][dst_index] = dst.y;

				if (dst_grid < smallestHeight_)
				{
					smallestHeight_ = dst_grid;
				}

				gotOne = true;
				floodStack.push(std::make_pair(dst,dst_grid));
			}

			// Set the adjacency information
			if( adjGrids_[src_grid][src_index].angle( angle ) == 0 &&
				adjGrids_[dst_grid][dst_index].angle( angle ^ 4 ) == 0 )
			{
				log.println( 1, "make link: src_grid: %d  src_index: %d angle: %d -- dst_grid: %d  dst_index: %d angle: %d",
					src_grid, src_index, angle, dst_grid, dst_index, ( angle ^ 4 ) );
				adjGrids_[src_grid][src_index].angle( angle, dst_grid );
				adjGrids_[dst_grid][dst_index].angle( angle ^ 4, src_grid );
				gotOne = true;
			}
			else
			{
				#ifdef DEBUG_WAYPOINT_FLOOD
				MF_ASSERT( adjGrids_[src_grid][src_index].angle( angle ) == dst_grid );
				MF_ASSERT( adjGrids_[dst_grid][dst_index].angle( angle ^ 4 ) == src_grid );
				#endif//DEBUG_WAYPOINT_FLOOD
			}
			// heightGrid_[dst_index] = dst.y;
		}
	}

	if (!gotOne)
	{
		hgtGrids_[ saveG ][ saveIndex ] = INITIAL_HEIGHT;
	}

	return count;
}


/**
 *	Flash flood the whole area with the given height
 */
void WaypointFlood::flashFlood( float height )
{
	BW_GUARD;

	int index, aindex, bindex;

	int hgtGrid = MAX_HEIGHTS - 1;

	// fill it all in
	for ( int x = 0; x < xsize_; ++x )
	{
		for ( int z = 0; z < zsize_; ++z )
		{
			index = x + z * xsize_;

			adjGrids_[hgtGrid][ index ].all = 0xffffffff;
			hgtGrids_[hgtGrid][ index ] = height;
		}
	}

	// fix up bottom and top
	for ( int x = 0; x < xsize_; ++x )
	{
		aindex = x;
		bindex = x + (zsize_-1) * xsize_;

		adjGrids_[hgtGrid][aindex].each.dr = 0;	adjGrids_[hgtGrid][bindex].each.ul = 0;
		adjGrids_[hgtGrid][aindex].each.d = 0;	adjGrids_[hgtGrid][bindex].each.u = 0;
		adjGrids_[hgtGrid][aindex].each.dl = 0;	adjGrids_[hgtGrid][bindex].each.ur = 0;
		//grid_[aindex] &= ~((1<<3) | (1<<4) | (1<<5));
		//grid_[bindex] &= ~((1<<7) | (1<<0) | (1<<1));
	}

	// fix up left and right
	for ( int z = 0; z < zsize_; ++z )
	{
		aindex = z * xsize_;
		bindex = xsize_-1 + z * xsize_;

		adjGrids_[hgtGrid][aindex].each.dl = 0;	adjGrids_[hgtGrid][bindex].each.ur = 0;
		adjGrids_[hgtGrid][aindex].each.l = 0;	adjGrids_[hgtGrid][bindex].each.r = 0;
		adjGrids_[hgtGrid][aindex].each.ul = 0;	adjGrids_[hgtGrid][bindex].each.dr = 0;
		//grid_[aindex] &= ~((1<<5) | (1<<6) | (1<<7));
		//grid_[bindex] &= ~((1<<1) | (1<<2) | (1<<3));
	}

	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {xsize_, xsize_, 0, -xsize_, -xsize_, -xsize_, 0, xsize_};

	// now remove the corner points completely
	for ( int a = 0; a < 8; a+=2 )
	{
		index = ((a&4) ? xsize_ - 1 : 0) +
			((uint(a-2) < 4) ? zsize_ - 1 : 0 ) * xsize_;
		adjGrids_[hgtGrid][index].all = 0;
		adjGrids_[hgtGrid][index + dx[(a+0)&7] + dy[(a+0)&7] ].angle( (a+4)&7, 0 );
		adjGrids_[hgtGrid][index + dx[(a+1)&7] + dy[(a+1)&7] ].angle( (a+5)&7, 0 );
		adjGrids_[hgtGrid][index + dx[(a+2)&7] + dy[(a+2)&7] ].angle( (a+6)&7, 0 );
	}
}

/**
 *	This method applies post filters to the adjacency grid after all
 *	fills have been done.
 */
void WaypointFlood::postfilteradd()
{
	BW_GUARD;

	AdjGridElt * origGrids[MAX_HEIGHTS];
	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
		origGrids[g] = new AdjGridElt[size_];
		memcpy( origGrids[g], adjGrids_[g], size_*sizeof(AdjGridElt) );
	}

	

	// 701
	// 6 2				(mathematical system not screen system)
	// 543

	//		o-o-o
	//		|X|X|
	//		o-o-o
	//		|X|X|
	//		o-o-o

	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {xsize_, xsize_, 0, -xsize_, -xsize_, -xsize_, 0, xsize_};

	// Smooth out islands of points that are unpassable from certain
	// directions but passable from others. This is only because it
	// makes for better waypoint graphs.

	bool gotone;
	do
	{
		gotone = false;
		for ( int g = smallestHeight_; g < MAX_HEIGHTS; ++g )
		{
			for ( int x = 0; x < xsize_; ++x )
			{
				for ( int z = 0; z < zsize_; ++z )
				{
					int index = x + z * xsize_;
					

					// Check the case:
					//		o		  o
					//		 \	or	 /		and rotated 4 ways
					//		o-o		o-o
					// Turn into:
					//		o		  o
					//		|\	or	 /|		and rotated 4 ways
					//		o-o		o-o
					for ( int i = 0; i < 8; i+=2 )
					{
						if (adjGrids_[g][index].angle( i )) continue;

						if (i == 0 && z == zsize_-1) continue;
						if (i == 2 && x == xsize_-1) continue;
						if (i == 4 && z == 0) continue;
						if (i == 6 && x == 0) continue;

						int ondex = index + dx[i] + dy[i];/*(
							(i&2)?( (i&4)?-1:1 ):( (i&4)?-xsize_:xsize_ ) );*/
						int og = 0;
						int tindex, tg;

						tindex = index + dx[(i+2)&7] + dy[(i+2)&7];
						tg = origGrids[g][index].angle( (i+2)&7 );
						if (!og && tg)
							og = origGrids[tg][tindex].angle( (i+7)&7 );
						tindex = index + dx[(i+6)&7] + dy[(i+6)&7];
						tg = origGrids[g][index].angle( (i+6)&7 );
						if (!og && tg)
							og = origGrids[tg][tindex].angle( (i+1)&7 );
						/*
						if ((origGrid[index] & (1<<((i+2)&7)))
							(origGrid[ondex] & (1<<((i+3)&7))) ||
							(origGrid[index] & (1<<((i+6)&7))) &&
							(origGrid[ondex] & (1<<((i+5)&7))))
						{
							adjGrids_[index] |= (1<<i);
							adjGrids_[ondex] |= (1<<(i^4));
							*/
						if (og)
						{
							adjGrids_[g][index].angle( i, og );
							adjGrids_[og][ondex].angle( i^4, g );
							gotone = true;
						}
					}
				}
			}
		}

		for ( int g = smallestHeight_; g < MAX_HEIGHTS; ++g )
		{
			memcpy( origGrids[g], adjGrids_[g], size_*sizeof(AdjGridElt) );
		}
	} while (gotone);


	for ( int g = smallestHeight_; g < MAX_HEIGHTS; ++g )
	{
		for ( int x = 1; x < xsize_; ++x )
		{
			for ( int z = 1; z < zsize_; ++z )
			{
				int index = x + z * xsize_;

				// Check the case:
				//		o-o
				//		| |
				//		o-o
				// Turn into:
				//		o-o
				//		|X|
				//		o-o
				/*
				if ((origGrid[index] & (1<<6)) &&
					(origGrid[index] & (1<<4)) &&
					(origGrid[index-xsize_-1] & (1<<2)) &&
					(origGrid[index-xsize_-1] & (1<<0)))
				{
					grid_[index] |= (1<<5);
					grid_[index-1] |= (1<<3);
					grid_[index-xsize_] |= (1<<7);
					grid_[index-xsize_-1] |= (1<<1);
				}
				*/

				int lg, dg, rg, ug;
				if ( (lg=origGrids[g][index].each.l) &&
					 (dg=origGrids[lg][index-1].each.d) &&
				 	 (rg=origGrids[dg][index-xsize_-1].each.r) &&
					 (ug=origGrids[rg][index-xsize_].each.u) &&
					 ug == g )
				{
					adjGrids_[g][index].each.dl = dg;
					adjGrids_[lg][index-1].each.dr = rg;
					adjGrids_[dg][index-xsize_-1].each.ur = g;
					adjGrids_[rg][index-xsize_].each.ul = lg;
				}
			}
		}
	}

	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
		memcpy( origGrids[g], adjGrids_[g], size_*sizeof(AdjGridElt) );
	}

	this->checkGridConsistency();

	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
		bw_safe_delete_array(origGrids[g]);
	}
}

void WaypointFlood::postfilterremove()
{
	BW_GUARD;

	AdjGridElt * origGrids[MAX_HEIGHTS];
	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
		origGrids[g] = new AdjGridElt[size_];
		memcpy( origGrids[g], adjGrids_[g], size_*sizeof(AdjGridElt) );
	}

	

	// 701
	// 6 2				(mathematical system not screen system)
	// 543

	//		o-o-o
	//		|X|X|
	//		o-o-o
	//		|X|X|
	//		o-o-o

	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {xsize_, xsize_, 0, -xsize_, -xsize_, -xsize_, 0, xsize_};

	const int xdirs[4] = { 1, -xsize_, -1, xsize_ };
	const int zdirs[4] = { xsize_, 1, -xsize_, -1 };
	const int anglemap[9] = { 5, 4, 3, 6, -1, 2, 7, 0, 1 };

	for ( int g = smallestHeight_; g < MAX_HEIGHTS; ++g )
	{
		for ( int x = 1; x < xsize_ - 1; ++x )
		{
			for ( int z = 1; z < zsize_ - 1; ++z )
			{
				int index = x + z * xsize_;

				if( boundarySet_.find( std::make_pair( index, g ) ) != boundarySet_.end() )
					continue;

				// Check the case:
				//		o?o?o
				//		* ? ?
				//		o-o?o		one of * must be set
				//		|X| ?
				//		o-o*o
				// Turn into:
				//		o?o?o				6 7 8
				//		?   ?
				//		o o o		ondex:	3 4 5
				//		|\  ?
				//		o-o?o				0 1 2

				// and rotated 4 ways

				for ( int a = 0; a < 8; a += 2 )
				{
					int ondex[9];
					int og[9];
					for ( int i = 0; i < 9; ++i )
					{
						ondex[i] = index +
							xdirs[a>>1]*((i%3)-1) + zdirs[a>>1]*((i/3)-1);
						og[i] = (anglemap[i] != -1) ?
							adjGrids_[g][index].angle( (a + anglemap[i]) & 7 ) : g;
					}
					if (og[3] && origGrids[og[3]][ondex[3]].angle( (1+a)&7 ) == 0 &&
						og[3] && origGrids[og[3]][ondex[3]].angle( (2+a)&7 ) != 0 &&
						og[3] && origGrids[og[3]][ondex[3]].angle( (3+a)&7 ) != 0 &&
						og[3] && origGrids[og[3]][ondex[3]].angle( (4+a)&7 ) == og[0] &&
						og[1] && origGrids[og[1]][ondex[1]].angle( (1+a)&7 ) == 0 &&
						og[1] && origGrids[og[1]][ondex[1]].angle( (0+a)&7 ) != 0 &&
						og[1] && origGrids[og[1]][ondex[1]].angle( (7+a)&7 ) != 0 &&
						og[1] && origGrids[og[1]][ondex[1]].angle( (6+a)&7 ) == og[0] &&

						origGrids[g][index].angle( (6+a)&7 ) != 0 &&
						origGrids[g][index].angle( (5+a)&7 ) != 0 &&
						origGrids[g][index].angle( (4+a)&7 ) != 0 &&
						origGrids[g][index].angle( (3+a)&7 ) == 0 &&
						origGrids[g][index].angle( (1+a)&7 ) == 0 &&
						origGrids[g][index].angle( (7+a)&7 ) == 0 &&
						(
						og[3] && origGrids[og[3]][ondex[3]].angle( (0+a)&7 ) != 0 ||
						og[1] && origGrids[og[1]][ondex[1]].angle( (2+a)&7 ) != 0
						))
					{
						adjGrids_[og[1]][ondex[1]].angle( (0+a)&7, 0 );
						adjGrids_[og[1]][ondex[1]].angle( (1+a)&7, 0 );
						adjGrids_[og[0]][ondex[0]].angle( (1+a)&7, 0 );
						adjGrids_[og[3]][ondex[3]].angle( (1+a)&7, 0 );
						adjGrids_[og[3]][ondex[3]].angle( (2+a)&7, 0 );

						adjGrids_[g][index].all = 0;

						if (og[5]) adjGrids_[og[5]][ondex[5]].angle( (6+a)&7, 0 );
						if (og[7]) adjGrids_[og[7]][ondex[7]].angle( (4+a)&7, 0 );
					}
					/*
					if ((origGrid[ondex[3]] & (1<<((1+a)&7))) == 0 &&
						(origGrid[ondex[3]] & (1<<((2+a)&7))) &&
						(origGrid[ondex[3]] & (1<<((3+a)&7))) &&
						(origGrid[ondex[3]] & (1<<((4+a)&7))) &&
						(origGrid[ondex[1]] & (1<<((1+a)&7))) == 0 &&
						(origGrid[ondex[1]] & (1<<((0+a)&7))) &&
						(origGrid[ondex[1]] & (1<<((7+a)&7))) &&
						(origGrid[ondex[1]] & (1<<((6+a)&7))) &&

						(origGrid[ondex[4]] & (1<<((6+a)&7))) &&
						(origGrid[ondex[4]] & (1<<((5+a)&7))) &&
						(origGrid[ondex[4]] & (1<<((4+a)&7))) &&
						(origGrid[ondex[4]] & (1<<((3+a)&7))) == 0 &&
						(origGrid[ondex[4]] & (1<<((1+a)&7))) == 0 &&
						(origGrid[ondex[4]] & (1<<((7+a)&7))) == 0 &&
						(
						(origGrid[ondex[3]] & (1<<((0+a)&7))) ||
						(origGrid[ondex[1]] & (1<<((2+a)&7)))
						))
					{
						grid_[ondex[1]] &= ~(1<<((0+a)&7));
						grid_[ondex[1]] &= ~(1<<((1+a)&7));
						grid_[ondex[0]] &= ~(1<<((1+a)&7));
						grid_[ondex[3]] &= ~(1<<((1+a)&7));
						grid_[ondex[3]] &= ~(1<<((2+a)&7));

						grid_[ondex[4]] = 0;

						grid_[ondex[5]] &= ~(1<<((6+a)&7));
						grid_[ondex[7]] &= ~(1<<((4+a)&7));
					}
					*/
				}
			}
		}
	}

	this->checkGridConsistency();

	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
		bw_safe_delete_array(origGrids[g]);
	}
}

/**
 *	This private method assesses the internal consistency of the grid.
 */
bool WaypointFlood::checkGridConsistency()
{
	BW_GUARD;

	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {xsize_, xsize_, 0, -xsize_, -xsize_, -xsize_, 0, xsize_};

	// make sure the grid is internally consistent
	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
			for ( int x = 1; x < xsize_ - 1; ++x )
		{
			for ( int z = 1; z < zsize_ - 1; ++z )
			{
				uint sindex = x + z * xsize_;
				for ( int angle = 0; angle < 8; ++angle )
				{
					uint dindex = sindex + dx[angle] + dy[angle];

	//				if ((!!(grid_[sindex] & (1<<angle))) !=
	//					(!!(grid_[dindex] & (1<<(angle^4)))))
					int dg = adjGrids_[g][sindex].angle( angle );
					if ( dg && adjGrids_[dg][dindex].angle( angle ^ 4 ) != g )
					{
						ERROR_MSG( "WaypointFlood: Failed consistency check "
							"at %d, %d in %d angle %d: fwd %d rev %d\n", x, z, g, angle,
							dg, adjGrids_[dg][dindex].angle( angle ^ 4 ) );
						return false;
					}
				}
			}
		}
	}
	return true;
}


/**
 *	This method shrinks the passable area in the grid by one unit
 */
void WaypointFlood::shrink()
{
	BW_GUARD;

	AdjGridElt * origGrids[MAX_HEIGHTS];
	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
		origGrids[g] = new AdjGridElt[size_];
		memcpy( origGrids[g], adjGrids_[g], size_*sizeof(AdjGridElt) );
	}

	const int dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	const int dy[8] = {xsize_, xsize_, 0, -xsize_, -xsize_, -xsize_, 0, xsize_};

#if 0
	// simply remove all points that are not completely connected
	for ( int x = 2; x < xsize_ - 2; ++x )
	{
		for ( int z = 2; z < zsize_ - 2; ++z )
		{
			int index = x + z * xsize_;

			uint8 gval = origGrid[index];
			if (gval == 0xFF) continue;

			for ( int a = 0; a < 8; ++a )
			{
				if ( !((gval >> a) & 1) ) continue;
				grid_[index + dx[a] + dy[a]] &= ~(1 << (a^4));
			}
			grid_[index] = 0;
		}
	}

	// make sure the grid is internally consistent
	for ( int x = 1; x < xsize_ - 1; ++x )
	{
		for ( int z = 1; z < zsize_ - 1; ++z )
		{
			uint sindex = x + z * xsize_;
			for ( int angle = 0; angle < 8; ++angle )
			{
				uint dindex = sindex + dx[angle] + dy[angle];

				if ( (!!(grid_[sindex] & (1<<angle))) !=
				  	 (!!(grid_[dindex] & (1<<(angle^4)))) )
				{
					ERROR_MSG( "WaypointFlood: Failed shrink consistency check "
						"at %d, %d angle %d: %X %X\n", x, z, angle,
						uint(grid_[sindex]), uint(grid_[dindex]) );
				}
			}
		}
	}

#endif
	for ( int g = 0; g < MAX_HEIGHTS; ++g )
	{
		bw_safe_delete_array(origGrids[g]);
	}
}


/**
 *	This method writes the adjacency data to a file,
 *	in TGA format. Each point in the grid is represented
 *	by one pixel. The 8 bits in each pixel represent
 *	the 8 adjacencies.
 *
 *	@param filename	The file to write to.
 *	@return True if successful.
 */
bool WaypointFlood::writeTGA(const char* filename) const
{
	BW_GUARD;

	int i;
	FILE* pFile = bw_fopen(filename, "wb");

	if ( !pFile )
	{
		ERROR_MSG("WaypointFlood: Failed to open %s\n", filename);
		return false;
	}

	unsigned char hdr[] =
	{
		sizeof(min_) + sizeof(resolution_),	// extra data size
		1, 1, 0, 0, 0, 1, 24, 0, 0, 0, 0,
		(unsigned char)((xsize_*MAX_HEIGHTS*4) & 0xff), (unsigned char)((xsize_*MAX_HEIGHTS*4) >> 8),
		(unsigned char)((zsize_*MAX_HEIGHTS*4) & 0xff), (unsigned char)((zsize_*MAX_HEIGHTS*4) >> 8),
		8, 0
	};

	fwrite( &hdr, sizeof(hdr), 1, pFile );

	// Store the min position and resolution, so we can
	// recalculate vertex positions later.

	fwrite( &min_, sizeof(min_), 1, pFile );
	fwrite( &resolution_, sizeof(resolution_), 1, pFile );

	for ( i = 0; i < 256; ++i )
	{
		fputc(i, pFile);
		fputc(i, pFile);
		fputc(i, pFile);
	}

	for ( int g = 0; g < MAX_HEIGHTS; ++g )
		fwrite( adjGrids_[g], size_ * sizeof(AdjGridElt), 1, pFile );
	for ( int g = 0; g < MAX_HEIGHTS; ++g )
		fwrite( hgtGrids_[g], size_ * sizeof(float), 1, pFile );
	fclose( pFile );
	return true;
}

BW_END_NAMESPACE
