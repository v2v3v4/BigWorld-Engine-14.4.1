#ifndef CHUNK_QUAD_TREE_HPP
#define CHUNK_QUAD_TREE_HPP


BW_BEGIN_NAMESPACE

const uint32 QUAD_TREE_CHUNK_DIM = 16;
const uint32 QUAD_TREE_CHUNK_SIZE = 16 * 16;
const uint32 QUAD_TREE_NODE_COUNT = 16*16 + 8*8 + 4*4 + 2*2 + 1;

/**
 *	This class implements a quad tree for outdoor chunks
 *	it is used to quickly determine what chunks are visible
 */
class ChunkQuadTree
{
public:
	ChunkQuadTree(uint32 token, float gridSize);

	void addChunk( Chunk* pChunk );
	bool removeChunk( Chunk* pChunk );

	void calculateVisible( const Matrix& viewProjection, BW::vector<Chunk*>& chunkList );

	float distanceToPoint( const Vector3& point );

	static uint32 token( const Vector3& position, float gridSiz );

	static void setTraverseOrder( const Vector3& viewDirection );
private:
	static uint8 s_traverseOrder_[4];

	float topSize_;
	Vector2 corner_;

	/**
	 *	This struct implements the quad tree nodes
	 */
	struct QuadTreeNode
	{
		void init( Chunk** pStartChunk, uint32 chunkCount, QuadTreeNode* &nextAllocation );
		void insert( Chunk* pChunk, Vector2 location );
		void remove( Vector2 location );
		void calculateVisible( const Matrix& viewProjection, BW::vector<Chunk*>& chunkList );
		void allVisible(BW::vector<Chunk*>& chunkList);
		BoundingBox bb_;
		Chunk**	pStartChunk_;
		uint32	chunkCount_;
		QuadTreeNode* nodes_[4];
	};

	QuadTreeNode	nodes_[QUAD_TREE_NODE_COUNT];

	Chunk*	pChunks_[QUAD_TREE_CHUNK_SIZE];

};

BW_END_NAMESPACE

#endif // CHUNK_QUAD_TREE_HPP
