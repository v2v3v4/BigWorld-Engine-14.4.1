#ifndef CHUNK_WAYPOINT_VERTEX_PROVIDER
#define CHUNK_WAYPOINT_VERTEX_PROVIDER

#include "chunk_waypoint.hpp"

#include "math/vector2.hpp"


BW_BEGIN_NAMESPACE

class ChunkWaypointVertexProvider
{
public:
	virtual const Vector2 & vertexByIndex( EdgeIndex index ) const = 0;
};

BW_END_NAMESPACE

#endif // CHUNK_WAYPOINT_VERTEX_PROVIDER

