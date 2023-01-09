#ifndef CHUNK_WAYPOINT_SET_HPP
#define CHUNK_WAYPOINT_SET_HPP

#include "chunk_waypoint.hpp"
#include "chunk_waypoint_set_data.hpp"
#include "chunk_waypoint_vertex_provider.hpp"
#include "dependent_array.hpp"

#include "chunk/chunk_item.hpp"
#include "chunk/chunk_boundary.hpp"
#include "chunk/chunk.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class	ChunkWaypointSet;
typedef SmartPointer< ChunkWaypointSet > ChunkWaypointSetPtr;

typedef BW::vector< ChunkWaypointSetPtr > ChunkWaypointSets;

typedef BW::map< ChunkWaypointSetPtr, ChunkBoundary::Portal * >
	ChunkWaypointConns;

struct EdgeLabel
{
	ChunkWaypointSetPtr pSet_;
	WaypointIndex waypoint_;
	Vector3 entryPoint_;
};

typedef BW::unordered_map< EdgeIndex, EdgeLabel >
	ChunkWaypointEdgeLabels;

struct NeighbourEdge
{
	float yValues[3];
	WaypointSpaceVector3 v;
	bool isBound;
};

typedef BW::map< EdgeIndex, NeighbourEdge > NeighbourEdges;


/**
 *	This class is a set of connected waypoints in a chunk.
 *	It may have connections to other waypoint sets when its chunk is bound.
 */
class ChunkWaypointSet : public ChunkItem, public ChunkWaypointVertexProvider
{
	DECLARE_CHUNK_ITEM( ChunkWaypointSet )

public:
	ChunkWaypointSet( ChunkWaypointSetDataPtr pSetData );
	~ChunkWaypointSet();

	virtual void toss( Chunk * pChunk );

	void bind();

	/**
	 *	This finds the waypoint that contains the given point.
	 *
	 *	@param point		The point that is used to find the waypoint.
	 *	@param ignoreHeight	Flag indicates vertical range should be considered
	 *						in finding waypoint.  If not, the best waypoint
	 *						that is closest to give point is selected (of
	 *						course the waypoint should contain projection of
	 *						the given point regardless).
	 *
	 *	@return				The index of the waypoint that contains the point,
	 *						-1 if no waypoint contains the point.
	 */
	WaypointIndex find( const WaypointSpaceVector3 & point, bool ignoreHeight = false )
	{
		return pSetData_->find( point, ignoreHeight );
	}

	/**
	 *	This finds the waypoint that contains the given point.
	 *
	 *	@param point		The point that is used to find the waypoint.
	 *	@param bestDistanceSquared	The point must be closer than this to the
	 *						waypoint.  It is updated to the new best distance
	 *						if a better waypoint is found.
	 *
	 *	@return				The index of the waypoint that contains the point,
	 *						-1 if no waypoint contains the point.
	 */
	WaypointIndex find( const WaypointSpaceVector3 & point, float & bestDistanceSquared )
	{ 
		return pSetData_->find( chunk(), point, bestDistanceSquared ); 
	}

	/**
	 *	This gets the girth of the waypoints.
	 *
	 *	@returns			The girth of the waypoints.
	 */
	float girth() const
	{
		return pSetData_->girth();
	}

	/**
	 *	This gets the number of waypoints in the set.
	 *
	 *	@returns			The number of waypoints.
	 */
	ChunkWaypoints::size_type waypointCount() const
	{
		return pSetData_->waypoints().size();
	}

	/**
	 *	This gets the index'th waypoint.
	 *
	 *	@param index		The index of the waypoint to get.
	 *	@return				A reference to the index'th waypoint.
	 */
	ChunkWaypoint & waypoint( ChunkWaypoints::size_type index )
	{
		return pSetData_->waypoints()[index];
	}

	/**
	 *	This gets the index'th waypoint.
	 *
	 *	@param index		The index of the waypoint to get.
	 *	@return				A reference to the index'th waypoint.
	 */
	const ChunkWaypoint & waypoint( ChunkWaypoints::size_type index ) const
	{
		return pSetData_->waypoints()[index];
	}

	/**
	 *	This returns an iterator to the first connection.
	 *
	 *	@return				An iterator to the first connection.
	 */
	ChunkWaypointConns::iterator connectionsBegin()
	{
		return connections_.begin();
	}

	/**
	 *	This returns a const iterator to the first connection.
	 *
	 *	@return				A const iterator to the first connection.
	 */
	ChunkWaypointConns::const_iterator connectionsBegin() const
	{
		return connections_.begin();
	}

	/**
	 *	This returns an iterator that points one past the last connection.
	 *
	 *	@return				An iterator to one past the last connection.
	 */
	ChunkWaypointConns::iterator connectionsEnd()
	{
		return connections_.end();
	}

	/**
	 *	This returns a const iterator that points one past the last connection.
	 *
	 *	@return				A const iterator to one past the last connection.
	 */
	ChunkWaypointConns::const_iterator connectionsEnd() const
	{
		return connections_.end();
	}

	/**
	 *	This gets the portal for the given waypoint set.
	 *
	 *	@param pWaypointSet	The ChunkWaypointSet to get the portal for.
	 *	@return				The portal for the ChunkWaypointSet.
	 */
	ChunkBoundary::Portal * connectionPortal( ChunkWaypointSetPtr pWaypointSet )
	{
		return connections_[pWaypointSet];
	}

	/**
	 *	This method gets the corresponding ChunkWaypointSet for an edge.
	 *
	 *	@param edge			The ChunkWaypoint::Edge to get the ChunkWaypointSet
	 *						for.
	 *	@return				The ChunkWaypointSet along the edge.
	 */
	ChunkWaypointSetPtr connectionWaypoint( const ChunkWaypoint::Edge & edge ) const
	{
		ChunkWaypointEdgeLabels::const_iterator iter = 
			edgeLabels_.find( pSetData_->getAbsoluteEdgeIndex( edge ) );

		if (iter != edgeLabels_.end())
		{
			return iter->second.pSet_;
		}

		return NULL;
	}


	/**
	 *	This method gets the corresponding EdgeLabel for an edge.
	 *
	 *	@param edge			The ChunkWaypoint::Edge to get the EdgeLabel
	 *						for.
	 *	@return				The EdgeLabel along the edge.
	 */
	const EdgeLabel & edgeLabel( const ChunkWaypoint::Edge & edge ) const
	{
		static EdgeLabel s_none = { NULL, -1, Vector3() };
		ChunkWaypointEdgeLabels::const_iterator iter = 
			edgeLabels_.find( pSetData_->getAbsoluteEdgeIndex( edge ) );

		if (iter != edgeLabels_.end())
		{
			return iter->second;
		}

		return s_none;
	}

	/**
	 *	This returns a const iterator to the first edge label.
	 *
	 *	@return				A const iterator to the first edge label.
	 */
	ChunkWaypointEdgeLabels::const_iterator edgeLabelsBegin() const
	{
		return edgeLabels_.begin();
	}

	/**
	 *	This returns a const iterator that points one past the last edge label.
	 *
	 *	@return				A const iterator to one past the last edge label.
	 */
	ChunkWaypointEdgeLabels::const_iterator edgeLabelsEnd() const
	{
		return edgeLabels_.end();
	}

	void addBacklink( ChunkWaypointSetPtr pWaypointSet );

	void removeBacklink( ChunkWaypointSetPtr pWaypointSet );

	void print() const;

	// From ChunkWaypointVertexProvider
	virtual const Vector2 & vertexByIndex( EdgeIndex index ) const
		{ return pSetData_->vertexByIndex( index ); }

#if !defined( MF_SERVER )
	void drawDebug() const;
#endif // !defined( MF_SERVER )

private:
	bool readyToBind() const;
	void deleteConnection( ChunkWaypointSetPtr pSet );

	void removeOurConnections();
	void removeOthersConnections();

	void connect(
		ChunkWaypointSetPtr pWaypointSet,
		WaypointIndex otherWaypoint,
		ChunkBoundary::Portal * pPortal,
		const EdgeIndex & rEdgeIndex,
		const Vector3 & entryPoint );

#if !defined( MF_SERVER )
	void drawNavPolys() const;
	void drawNavPolyEdge( const ChunkWaypoint::Edge& edge,
		const ChunkWaypoint::Edge& lastEdge,
		float height,
		const Moo::Colour& regularColour,
		const Moo::Colour& darkerColour ) const;
	void drawConnections() const;
	void drawEdgeLabels() const;

	static bool s_drawArrows_;
	static bool s_drawConnections_;
#endif // !defined( MF_SERVER )

	NeighbourEdges neighbourEdges_;

protected:
	ChunkWaypointSetDataPtr		pSetData_;
	ChunkWaypointConns			connections_;
	ChunkWaypointEdgeLabels		edgeLabels_;
	ChunkWaypointSets			backlinks_;
};

BW_END_NAMESPACE


#endif // CHUNK_WAYPOINT_SET_HPP
