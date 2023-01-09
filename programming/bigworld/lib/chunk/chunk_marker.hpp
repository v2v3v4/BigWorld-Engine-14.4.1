#ifndef CHUNK_MARKER_HPP
#define CHUNK_MARKER_HPP


#include "chunk_item_tree_node.hpp"
#include "chunk_stationnode.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

class ChunkMarkerCluster;
typedef SmartPointer<ChunkMarkerCluster> ChunkMarkerClusterPtr;


/**
 *	This class is a chunk item for markers 
 *	(locations where items can be dynamically placed)
 */
class ChunkMarker : public ChunkItemTreeNode
{
public:
	ChunkMarker()
		: ChunkItemTreeNode()
	{
	}

	bool load( DataSectionPtr pSection );


protected:
	Matrix transform_;
	BW::string category_;

private:
	static ChunkItemFactory::Result create( Chunk * pChunk, DataSectionPtr pSection );
	static ChunkItemFactory	factory_;
};

typedef SmartPointer<ChunkMarker> ChunkMarkerPtr;

BW_END_NAMESPACE

#endif // CHUNK_MARKER_HPP
