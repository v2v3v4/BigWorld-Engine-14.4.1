#ifndef CHUNK_NAVIGATOR_HPP
#define CHUNK_NAVIGATOR_HPP

#include "chunk_waypoint_set.hpp"
#include "girth_grids.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_cache.hpp"

#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

class NavigatorFindResult;
class Vector3;

/**
 *  This class is a cache of all the waypoint sets in a chunk.
 */
class ChunkNavigator : public ChunkCache
{
public:
	ChunkNavigator( Chunk & chunk );
	~ChunkNavigator();

	virtual void bind( bool isUnbind );

	bool find( const WorldSpaceVector3& point, float girth,
				NavigatorFindResult & res, bool ignoreHeight = false ) const;
	bool findExact( const WorldSpaceVector3& point, float girth,
				NavigatorFindResult & res ) const;

	bool isEmpty() const;
	bool hasNavPolySet( float girth ) const;

	void add( ChunkWaypointSet * pSet );
	void del( ChunkWaypointSet * pSet );

	static bool shouldUseGirthGrids()
		{ return s_useGirthGrids_; }

	static void shouldUseGirthGrids( bool value )
		{ s_useGirthGrids_ = value; }

	static Instance< ChunkNavigator > instance;

#if !defined( MF_SERVER )
	static void drawDebug();
	void drawWaypointSets() const;
#endif // !defined( MF_SERVER )

private:
#if !defined( MF_SERVER )
	static bool s_debugDraw;
	static bool s_debugDrawConnected;
#endif // !defined( MF_SERVER )

	Chunk & chunk_;

	ChunkWaypointSets	wpSets_;

	GirthGrids	girthGrids_;
	Vector2		girthGridOrigin_;
	float		girthGridResolution_;

	static bool s_useGirthGrids_;

	static const uint GIRTH_GRID_SIZE = 12;

	void tryGrid( GirthGridList* gridList, const WaypointSpaceVector3& point,
		float& bestDistanceSquared, int xi, int zi, NavigatorFindResult& res ) const;
};

BW_END_NAMESPACE

#endif // CHUNK_NAVIGATOR_HPP
