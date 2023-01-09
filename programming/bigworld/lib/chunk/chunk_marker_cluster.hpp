#ifndef CHUNK_MARKER_CLUSTER_HPP
#define CHUNK_MARKER_CLUSTER_HPP


#include "chunk_item_tree_node.hpp"
#include "chunk_stationnode.hpp"

BW_BEGIN_NAMESPACE

class ChunkMarker;
class ChunkMarkerClusterCache;

/**
 *	This class is the world editor tool's representation
 *	of a marker cluster.
 */
class ChunkMarkerCluster : public ChunkItemTreeNode
{
public:
	ChunkMarkerCluster()
		: availableMarkers_( 0 )
		, position_( Vector3::zero() )
	{
	}

	bool load( DataSectionPtr pSection, Chunk * pChunk );
	virtual bool load( DataSectionPtr pSection )
	{
		return this->load( pSection, NULL );
	}

protected:
	Vector3 position_;
    int availableMarkers_;

private:
	static ChunkItemFactory::Result create( Chunk * pChunk, DataSectionPtr pSection );
	static ChunkItemFactory	factory_;
};


typedef SmartPointer<ChunkMarkerCluster> ChunkMarkerClusterPtr;

BW_END_NAMESPACE


#endif // CHUNK_MARKER_CLUSTER_HPP
