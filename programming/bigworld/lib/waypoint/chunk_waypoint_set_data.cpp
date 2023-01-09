#include "pch.hpp"

#include "chunk_waypoint_set.hpp"
#include "chunk_waypoint_set_data.hpp"
#include "waypoint.hpp"
#include "waypoint_stats.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/concurrency.hpp"

#include "chunk/chunk_space.hpp"

#include "resmgr/bwresource.hpp"

#include <cfloat>
#include <limits.h>
#include "cstdmf/bw_hash.hpp"


BW_BEGIN_NAMESPACE

namespace // anonymous
{

// Link it in to the chunk item section map
ChunkItemFactory navmeshFactoryLink( "worldNavmesh", 0, 
	&ChunkWaypointSetData::navmeshFactory );

typedef BW::vector< ChunkWaypointSetData * > NavmeshPopulationRecord;
typedef BW::map< BW::string, NavmeshPopulationRecord > NavmeshPopulation;

NavmeshPopulation s_navmeshPopulation;

SimpleMutex s_navmeshPopulationLock;

template <class C>
inline C consume( const char * & ptr )
{
	// gcc shoots itself in the foot with aliasing of this
	// return *((C*&)ptr)++;

	C * oldPtr = (C*)ptr;
	ptr += sizeof( C );
	return *oldPtr;
}

/**
 *	Helper class for building a mapping between unique Vector2s in a vector and
 *	vertexindices stored in ChunkWaypoint edge data.
 */
class VertexIndices
{
public:
	typedef BW::vector< Vector2 > Vertices;

	VertexIndices( Vertices & vertices, int32 numEdges ):
			vertices_( vertices ),
			map_()
	{
#ifdef _WIN32
		map_.reserve( numEdges );
#else // _WIN32
		// Assuming load factor 1.0
		map_.rehash( numEdges );
#endif // _WIN32
	}

	EdgeIndex getIndexForVertex( const Vector2 & vertex )
	{
		EdgeIndex index = 0;

		Map::iterator iVertex = map_.find( vertex );
		if (iVertex == map_.end())
		{
			index = static_cast<EdgeIndex>( vertices_.size() );
			vertices_.push_back( vertex );
			iVertex = map_.insert( Map::value_type( vertex, index ) ).first;

			static const size_t MAX_SIZE = (1 << (sizeof( EdgeIndex ) * 8)) - 1;

			if (vertices_.size() > MAX_SIZE)
			{
				// We've overflowed the storage for vertex indices.
				CRITICAL_MSG( "Chunk waypoint vertex indices have overflowed: "
						"%zu > %zu\n",
					vertices_.size(), MAX_SIZE );
			}
		}
		else
		{
			index = iVertex->second;
		}

		return index;
	}
private:
	Vertices & vertices_;

	struct VectorHash
	{
		size_t operator()( const Vector2 & hash ) const
		{
			return hash_string( (const char *)&hash, sizeof(Vector2) );
		}
	};

	typedef BW::unordered_map< Vector2, EdgeIndex, VectorHash > Map;
	Map map_;
};


} // end namespace (anonymous)

extern void NavmeshPopulation_remove( const BW::string & source );


/**
 *	This is the ChunkWaypointSetData constructor.
 */
ChunkWaypointSetData::ChunkWaypointSetData( WaypointIndex baseIndex ) :
	girth_( 0.f ),
	waypoints_(),
	source_(),
	edges_( NULL ),
	numEdges_( 0 ),
	baseIndex_( baseIndex )
{
}


/**
 *	This is the ChunkWaypointSetData destructor.
 */
ChunkWaypointSetData::~ChunkWaypointSetData()
{
	bw_safe_delete_array(edges_);

	WaypointStats::removeEdgesAndVertices( numEdges_, 
		static_cast<uint>(vertices_.size()) );
}


/**
 *	Safely delete set data
 */
void ChunkWaypointSetData::destroy() const
{
	SimpleMutexHolder smh( s_navmeshPopulationLock );
	if (this->refCount() == 0)
	{
		if (!source_.empty())
		{
			s_navmeshPopulation.erase( source_ );
		}
		delete this;
	}
}

/**
 *	Find the waypoint that contains the given point.
 *
 *	@param point		The point that is used to find the waypoint.
 *	@param ignoreHeight	A flag which indicates vertical range should be
 *						considered in finding the waypoint.  If not, the best
 *						waypoint that is closest to give point is selected (of
 *						course the waypoint should contain the projection of
 *						the given point regardless.)
 *	@return				The index of the waypoint, or -1 if not found.
 */
WaypointIndex ChunkWaypointSetData::find( const WaypointSpaceVector3 & point, 
		bool ignoreHeight )
{
	WaypointIndex bestWaypoint = -1;
	float bestHeightDiff = FLT_MAX;

	ChunkWaypoints::iterator wit;
	WaypointIndex i;
	for (wit = waypoints_.begin(), i = 0; wit != waypoints_.end(); ++wit, ++i)
	{
		if (ignoreHeight)
		{
			if (wit->containsProjection( *this, point ))
			{
				if (point.y > wit->minHeight_ - 0.1f &&
						point.y < wit->maxHeight_ + 0.1f)
				{
					return i;
				}
				else // find best fit
				{
					float wpAvgDiff = fabs( point.y -
						(wit->maxHeight_ + wit->minHeight_) / 2 );
					if (bestHeightDiff > wpAvgDiff)
					{
						bestHeightDiff = wpAvgDiff;
						bestWaypoint = i;
					}
				}
			}
		}
		else
		{
			if (wit->contains( *this, point ))
			{
				return i;
			}
		}
	}
	return bestWaypoint;
}


/**
 *	This method finds the waypoint closest to the given point.
 *
 *	@param chunk		The chunk to search in.
 *	@param point		The point that is used to find the closest waypoint.
 *	@param bestDistanceSquared The point must be closer than this to the
 *						waypoint.  It is updated to the new best distance if a
 *						better waypoint is found.
 *	@return				The index of the waypoint, or -1 if not found.
 */
WaypointIndex ChunkWaypointSetData::find( const Chunk * chunk, 
		const WaypointSpaceVector3 & point,
		float & bestDistanceSquared )
{
	WaypointIndex bestWaypoint = -1;
	ChunkWaypoints::iterator wit;
	WaypointIndex i;
	for (wit = waypoints_.begin(), i = 0; wit != waypoints_.end(); ++wit, ++i)
	{
		float distanceSquared = wit->distanceSquared( *this, chunk,
			MappedVector3( point, chunk ) );
		if (bestDistanceSquared > distanceSquared)
		{
			bestDistanceSquared = distanceSquared;
			bestWaypoint = i;
		}
	}
	return bestWaypoint;
}


/**
 *	This gets the index of the given edge.
 *
 *	@param edge			The edge to get.
 *	@return				The index of the edge.
 */
EdgeIndex ChunkWaypointSetData::getAbsoluteEdgeIndex(
		const ChunkWaypoint::Edge & edge ) const
{
	size_t index = &edge - edges_;
	MF_ASSERT( index <= USHRT_MAX );
	return static_cast<EdgeIndex>( index );
}


/**
 *	Read in waypoint set data from a binary source.
 */
const char * ChunkWaypointSetData::readWaypointSet( const char * pData,
	int32 numWaypoints, int32 numEdges )
{
	const size_t NAVPOLY_ELEMENT_SIZE =
		sizeof( float ) + sizeof( float ) + sizeof( int32 ); // 12

	// Remember what vertices have been mapped to which index, so we can 
	// reuse vertices. 
	VertexIndices vertexIndices( vertices_, numEdges ); 

	const char * pEdgeData = pData + numWaypoints * NAVPOLY_ELEMENT_SIZE; 
	waypoints_.resize( numWaypoints ); 
	numEdges_ = numEdges; 
	edges_ = new ChunkWaypoint::Edge[ numEdges ]; 
	WaypointStats::addEdges( numEdges ); 
	ChunkWaypoint::Edge * nedge = edges_; 

	for (int32 p = 0; p < numWaypoints; ++p) 
	{ 
		ChunkWaypoint & wp = waypoints_[p]; 

		wp.minHeight_ = consume<float>( pData ); 
		wp.maxHeight_ = consume<float>( pData ); 
		const int32 vertexCount = consume< int32 >( pData ); 

		//dprintf( "poly %d:", p ); 
		wp.edges_ = ChunkWaypoint::Edges( nedge, nedge+vertexCount ); 
		nedge += vertexCount; 

		for (EdgeIndex e = 0; e < vertexCount; ++e) 
		{ 
			ChunkWaypoint::Edge & edge = wp.edges_[e]; 

			float x = consume< float >( pEdgeData ); 
			float y = consume< float >( pEdgeData ); 

			Vector2 vertex( x, y ); 
			edge.vertexIndex_ = vertexIndices.getIndexForVertex( vertex ); 

			edge.neighbour_ = static_cast< EdgeIndex >(
				consume< int32 >( pEdgeData ) ); 
				// 'adj' already encoded as we like it 
			--numEdges; 

			//dprintf( " %08X", edge.neighbour_ ); 
		} 
		//dprintf( "\n" ); 
		wp.calcCentre( *this ); 

		wp.visited_ = 0; 
	} 

	MF_ASSERT( numEdges == 0 ); 

	WaypointStats::addVertices( static_cast<uint>(vertices_.size()) ); 

	return pEdgeData; 
}


/**
 *	Factory method for ChunkWaypointSetData's.
 */
ChunkItemFactory::Result ChunkWaypointSetData::navmeshFactory( Chunk * pChunk, 
		DataSectionPtr pSection )
{
	if (!pChunk->space()->isNavigationEnabled() ||
		(pChunk->space()->navmeshGenerator() ==
			BaseChunkSpace::NAVMESH_GENERATOR_NONE))
	{
		return ChunkItemFactory::SucceededWithoutItem();
	}

	BW::string resName = pSection->readString( "resource" ); 
	BW::string fullName = pChunk->mapping()->path() + resName; 

	// store the sets into a vector and add them into chunk 
	// after release s_navmeshPopulationLock to avoid deadlock 
	BW::vector<ChunkWaypointSetPtr> sets; 

	{ // s_navmeshPopulationLock 
		SimpleMutexHolder smh( s_navmeshPopulationLock ); 
		// see if we've already loaded this navmesh 
		NavmeshPopulation::iterator found = 
			s_navmeshPopulation.find( fullName ); 

		if (found != s_navmeshPopulation.end()) 
		{ 
			sets.reserve( found->second.size() ); 

			NavmeshPopulationRecord::const_iterator iter; 
			for (iter = found->second.begin(); 
					iter != found->second.end(); 
					++iter) 
			{ 
				ChunkWaypointSet * pSet = new ChunkWaypointSet( *iter ); 
				sets.push_back( pSet ); 
			} 
		} 
	} // !s_navmeshPopulationLock 

	if (!sets.empty()) 
	{ 
		for (BW::vector<ChunkWaypointSetPtr>::iterator iter = sets.begin(); 
				iter != sets.end(); 
				++iter) 
		{ 
			pChunk->addStaticItem( *iter ); 
		} 

		return ChunkItemFactory::SucceededWithoutItem(); 
	} 

	// ok, no, time to load it then 
	BinaryPtr pNavmesh = BWResource::instance().rootSection()->readBinary( 
		pChunk->mapping()->path() + resName ); 

	if (!pNavmesh) 
	{ 
		BW::string errorString = "Could not read navmesh '" + fullName + "'"; 
		return ChunkItemFactory::Result( NULL, errorString ); 
	} 

	if (pNavmesh->len() == 0) 
	{ 
		// empty navmesh (not put in popln) 
		return ChunkItemFactory::SucceededWithoutItem(); 
	} 

	NavmeshPopulationRecord newRecord; 

	const char * dataBeg = pNavmesh->cdata(); 
	const char * dataEnd = dataBeg + pNavmesh->len(); 
	const char * dataPtr = dataBeg; 
	WaypointIndex index = 1; // yes, it starts from 1

	while (dataPtr < dataEnd) 
	{ 
		int32 aVersion = consume< int32 >( dataPtr ); 
		float aGirth = consume< float >( dataPtr ); 
		int32 numWaypoints = consume< int32 >( dataPtr ); 
		int32 numEdges = consume< int32 >( dataPtr ); 

		MF_ASSERT( aVersion == 0 ); 

		ChunkWaypointSetDataPtr pSetData = new ChunkWaypointSetData( index ); 
		pSetData->girth( aGirth ); 

		dataPtr = pSetData->readWaypointSet( dataPtr, numWaypoints, numEdges ); 
		pSetData->source_ = fullName;
		newRecord.push_back( pSetData.get() );

		ChunkWaypointSet * pSet = new ChunkWaypointSet( pSetData ); 
		pChunk->addStaticItem( pSet ); 

		index += numWaypoints;
	} 

	{ // s_navmeshPopulationLock 
		SimpleMutexHolder smh( s_navmeshPopulationLock ); 
		NavmeshPopulation::iterator found = 
			s_navmeshPopulation.insert( std::make_pair(fullName, 
				NavmeshPopulationRecord() ) ).first; 
		found->second.swap( newRecord ); 
	} // !s_navmeshPopulationLock 

	return ChunkItemFactory::SucceededWithoutItem(); 
} 

BW_END_NAMESPACE

// chunk_waypoint_set_data.cpp

