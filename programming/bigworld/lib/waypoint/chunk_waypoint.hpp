#ifndef CHUNK_WAYPOINT_HPP
#define CHUNK_WAYPOINT_HPP

#include "dependent_array.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include "math/vector2.hpp"

#include "mapped_vector3.hpp"


BW_BEGIN_NAMESPACE

class Chunk;
class ChunkWaypointVertexProvider;
class Vector3;


typedef GeoSpaceVector3 WaypointSpaceVector3;
typedef uint16 EdgeIndex;
typedef int32 WaypointIndex;


/**
 *	This class is a waypoint as it exists in a chunk, when fully loaded.
 */
class ChunkWaypoint
{
public:
	static const float HEIGHT_RANGE_TOLERANCE;

	ChunkWaypoint();
	ChunkWaypoint( const ChunkWaypoint & other );
	~ChunkWaypoint();


	bool contains( const ChunkWaypointVertexProvider & provider, 
		const WaypointSpaceVector3 & point ) const;

	bool containsProjection( const ChunkWaypointVertexProvider & provider, 
		const WaypointSpaceVector3 & point ) const;

	float distanceSquared( const ChunkWaypointVertexProvider & provider,
		const Chunk * chunk,
		const WorldSpaceVector3 & point ) const;

	void clip( const ChunkWaypointVertexProvider & provider,
		const Chunk * chunk, WorldSpaceVector3 & point ) const;

	void makeMaxHeight( const Chunk * chunk, WorldSpaceVector3 & point ) const;

	void print( const ChunkWaypointVertexProvider & provider ) const;

	void calcCentre( const ChunkWaypointVertexProvider & provider );

// Member data

	/**
	 *	This is the minimum height of the waypoint.
	 */
	float minHeight_;

	/**
	 *	This is the maximum height of the waypoint.
	 */
	float maxHeight_;

	/**
	 *	This is a point inside the waypoint. it might
	 *  not be the exact centre but could be used as
	 *	an internal point in some cases.
	 */
	Vector2 centre_;

	/**
	 *	This class is an edge in a waypoint/navpoly.
	 */
	struct Edge
	{
		static const EdgeIndex adjacentRangeStart;
		static const EdgeIndex adjacentRangeEnd;
		static const WaypointIndex otherChunk;

		/**
		 *	The index of the start coordinate of the edge.
		 */
		EdgeIndex	vertexIndex_;

		/**
		 *	This contains adjacency information.  If this value ranges
		 *	between 0 and 32768, then that is the ID of the waypoint adjacent
		 *	to this edge.  If this value is between 32768 and 65535, then it
		 *	is adjacent to	the chunk boundary.
		 *	Otherwise, it may contain some vista flags indicating cover, for
		 *	example, if its value's top bit is 1.
		 */
		EdgeIndex neighbour_;


		/**
		 *	This returns the neighbouring waypoint's index.
		 *
		 *	@return			The index of the neighbouring waypoint if there
		 *					is one, or -1 if there	is none.
		 */
		WaypointIndex neighbouringWaypoint() const
		{
			return neighbour_ < adjacentRangeStart ?
				static_cast< WaypointIndex >( neighbour_ ) :
				otherChunk;
		}


		/**
		 *	This returns whether this edge is adjacent to a chunk boundary.
		 *
		 *	@return			True if the edge is adjacent to a chunk boundary,
		 *					false otherwise.
		 */
		bool adjacentToChunk() const
		{
			return (neighbour_ >= adjacentRangeStart)
				/*&& (neighbour_ <= adjacentRangeEnd)*/;
		}
	};


	/**
	 *	This is the list of edges of the ChunkWaypoint.
	 */
	typedef DependentArray< Edge > Edges;
	Edges	edges_;

	/**
	 *	This is the number of edges of the ChunkWaypoint.
	 *
	 *	DO NOT move this away!  DependentArray depends on edgeCount_ to exist
	 *	on its construction.
	 */
	Edges::size_type	edgeCount_;

	mutable EdgeIndex	visited_;

	static BW::vector<ChunkWaypoint*> s_visitedWaypoints_;

};

typedef BW::vector< ChunkWaypoint > ChunkWaypoints;

BW_END_NAMESPACE

#endif // CHUNK_WAYPOINT_HPP
