#ifndef CHUNK_TREE_HPP
#define CHUNK_TREE_HPP

#include "base_chunk_tree.hpp"
#include <memory>


BW_BEGIN_NAMESPACE

/**
 *	This class is a body of water as a chunk item
 */
class ServerChunkTree : public BaseChunkTree
{
	DECLARE_CHUNK_ITEM( ServerChunkTree )

public:
	ServerChunkTree();
	~ServerChunkTree();

	bool load( DataSectionPtr pSection, Chunk * pChunk );

private:
	// Disallow copy
	ServerChunkTree( const ServerChunkTree & );
	const ServerChunkTree & operator = ( const ServerChunkTree & );
};

BW_END_NAMESPACE

#endif // CHUNK_TREE_HPP
