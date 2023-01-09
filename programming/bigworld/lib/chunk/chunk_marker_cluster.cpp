#include "pch.hpp"
#include "chunk_marker_cluster.hpp"
#include "chunk_marker.hpp"
#include "chunk.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkMarkerCluster
// -----------------------------------------------------------------------------

/**
 *	This static method creates a marker from the input section and adds
 *	it to the given chunk.
 */
ChunkItemFactory::Result ChunkMarkerCluster::create( Chunk* pChunk, DataSectionPtr pSection )
{
	ChunkMarkerCluster* node = new ChunkMarkerCluster();
	if (!node->load( pSection, pChunk ))
	{
		BW::string err = "Failed to load marker cluster";
		if ( node->id() != UniqueID::zero() )
		{
			err += ' ' + node->id().toString();
		}
		bw_safe_delete(node);
		return ChunkItemFactory::Result( NULL, err );
	}
	else
	{
		pChunk->addStaticItem( node );
		return ChunkItemFactory::Result( node );
	}
}


/// Static factory initialiser
ChunkItemFactory ChunkMarkerCluster::factory_( "marker_cluster", 0, ChunkMarkerCluster::create );


/**
 *	Load the properties from the data section.
 *	This could be from the default section (in res/objects/misc) or the chunk
 */
bool ChunkMarkerCluster::load( DataSectionPtr pSection, Chunk * pChunk )
{
	if (!ChunkItemTreeNode::load( pSection ))
	{
		return false;
	}

	position_ = pSection->readVector3( "position" );
	availableMarkers_ = pSection->readInt( "available_markers", 0 );

	if (availableMarkers_ > numberChildren())
	{
		availableMarkers_ = 0;

		if (pChunk)
		{
			Vector3 worldPos = pChunk->transform().applyPoint( position_ );
			CRITICAL_MSG( "ChunkMarkerCluster::load\n"
				"availableMarkers > numberChildren\n"
				"for cluster at %.2f,%.2f,%.2f\n"
				"in chunk %s\n"
				"Setting availableChildren to 0\n",
				worldPos.x, worldPos.y, worldPos.z,
				pChunk->identifier().c_str() );
		}
		else
		{
			CRITICAL_MSG( "ChunkMarkerCluster::load\n"
				"availableMarkers > numberChildren\n" );
		}
	}

	return true;
}

BW_END_NAMESPACE

// chunk_marker_cluster.cpp
