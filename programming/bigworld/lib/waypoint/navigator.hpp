#ifndef NAVIGATOR_HPP
#define NAVIGATOR_HPP

#include "chunk_waypoint_set.hpp"

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

#include "math/vector3.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class ChunkSpace;
class ChunkWPSetState;
class ChunkWaypointSet;
class ChunkWaypointState;
class NavLoc;
class NavigatorCache;

typedef SmartPointer< ChunkWaypointSet > ChunkWaypointSetPtr;

typedef BW::vector< Vector3 > Vector3Path;

/**
 *	This class guides vessels through the treacherous domain of chunk
 *	space navigation. Each instance caches recent data so similar searches
 *	can reuse previous effort.
 */
class Navigator
{
public:
	Navigator();
	~Navigator();

	enum CacheState
	{
		READ_CACHE,
		READ_WRITE_CACHE
	};

	NavLoc createPathNavLoc( ChunkSpace * pSpace, const Vector3 & pos,
		float girth ) const;

	bool findPath( const NavLoc & src, const NavLoc & dst, float maxDistance,
		bool blockNonPermissive, NavLoc & nextWaypoint,
		bool & passedActivatedPortal,
		CacheState cacheState = READ_WRITE_CACHE ) const;

	bool findFullPath( ChunkSpace* space, const Vector3& srcPos, const Vector3& dstPos,
		float maxDistance, bool blockNonPermissive, float girth,
		Vector3Path & waypointPath ) const;

	bool findRandomNeighbourPointWithRange( ChunkSpace* space, Vector3 position,
		float minRadius, float maxRadius, float girth, Vector3& result );

	void getCachedWaypointPath( Vector3Path & vec3Path );

	size_t getWaySetPathSize() const;

	void clearCachedWayPath();
	void clearCachedWaySetPath();

	void dumpCache();

private:
	Navigator( const Navigator & other );
	Navigator & operator = ( const Navigator & other );

	bool searchNextWaypoint( NavigatorCache & cache,
		const NavLoc & src,
		const NavLoc & dst,
		float maxDistance,
		bool blockNonPermissive,
		bool * pPassedActivatedPortal = NULL ) const;

	bool searchNextLocalWaypoint( NavigatorCache & cache,
		const ChunkWaypointState & srcState, 
		const ChunkWaypointState & dstState, float maxDistanceInSet ) const;

	bool searchNextWaypointSet( NavigatorCache & cache,
		const ChunkWPSetState & srcState,
		const ChunkWPSetState & dstState,
		float maxDistance ) const;

	NavigatorCache * pCache_;
};

BW_END_NAMESPACE

#endif // NAVIGATOR_HPP


