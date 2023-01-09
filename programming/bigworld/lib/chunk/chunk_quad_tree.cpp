#include "pch.hpp"

#include <limits>

#include "chunk_quad_tree.hpp"
#include "chunk.hpp"

#include "moo/line_helper.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 );

BW_BEGIN_NAMESPACE

/**
 *	ChunkQuadTree constructor
 *	@param token the token for this quad tree which is the 
 *		x/z grid position of this quad tree
 */
ChunkQuadTree::ChunkQuadTree(uint32 token, float gridSize) :
	topSize_( float( QUAD_TREE_CHUNK_DIM ) * gridSize )
{
	// get the grid positions from the token
	int16 gridX = int16(token & 0xffff);
	int16 gridZ = int16((token >>16) & 0xffff);

	// Set up the corner of this quad tree
	corner_.set( float( gridX ) * topSize_,
					float( gridZ ) * topSize_ );

	// Grab the second pre-allocated quad tree node as this will 
	// be used as the next node to allocate
	QuadTreeNode* nextAlloc = nodes_ + 1;

	// Init our quad tree nodes
	nodes_[0].init( pChunks_, QUAD_TREE_CHUNK_SIZE, nextAlloc );

	memset( pChunks_, 0, sizeof(pChunks_) );
}


/**
 *	This method adds a chunk to the quad tree
 *	@param pChunk the chunk to add
 */
void ChunkQuadTree::addChunk( Chunk* pChunk )
{
	BW_GUARD;
	// Calculate the normalised location of this chunk in the quad tree
	Vector3 loc3d = pChunk->centre();
	Vector2 location( loc3d.x - corner_.x, loc3d.z - corner_.y );

	location *= 1.f / topSize_;

	// Do the insert
	nodes_[0].insert( pChunk, location );
}


/**
 *	This method removes a chunk from the quad tree
 *	@param pChunk the chunk to add
 *	@return true if this quad tree still has data in it
 */
bool ChunkQuadTree::removeChunk( Chunk* pChunk )
{
	BW_GUARD;
	// Calculate the normalised location of this chunk in the quad tree
	Vector3 loc3d = pChunk->centre();
	Vector2 location( loc3d.x - corner_.x, loc3d.z - corner_.y );

	location *= 1.f / topSize_;

	// Remove the node
	nodes_[0].remove( location );

	// If the top node is not inside out we still have data
	return !nodes_[0].bb_.insideOut();
}


/**
 *	Calculate the visible chunks
 *	@param viewProjection the projection matrix used to determine if a chunk is visible
 *	@param chunkList the list of chunks to fill in with visible chunks
 */
void ChunkQuadTree::calculateVisible( const Matrix& viewProjection, BW::vector<Chunk*>& chunkList )
{
	BW_GUARD;
	nodes_[0].calculateVisible( viewProjection, chunkList );
}


/**
 *	This method calculates the distance from a point to the top quad tree node
 *	@param point the point to calculate the distance from
 *	@return the distance to the point
 */
float ChunkQuadTree::distanceToPoint( const Vector3& point )
{
	BW_GUARD;
	float res = std::numeric_limits<float>::max();

	if (!nodes_[0].bb_.insideOut())
	{
		res = nodes_[0].bb_.distance( point );
	}

	return res;
}


/**
 *	Calculate the quad tree token from a position
 *	@param position the position
 *	@return the token
 */
uint32 ChunkQuadTree::token( const Vector3& position, float gridSize )
{
	BW_GUARD;
	float xPos = floorf(position.x / float( QUAD_TREE_CHUNK_DIM ) * gridSize);
	float zPos = floorf(position.z / float( QUAD_TREE_CHUNK_DIM ) * gridSize);

	return uint32( (int32(xPos) & 0xffff) | ((int32(zPos) << 16) & 0xffff0000));
}


/**
 *	This method sets the traverse order for a quad tree based on
 *	a viewing direction, this is used to output the chunks in a rough
 *	front to back order.
 *	@param viewDirection viewing direction
 */
void ChunkQuadTree::setTraverseOrder( const Vector3& viewDirection )
{
	BW_GUARD;
	if (viewDirection.x > 0.f && viewDirection.z > 0.f)
	{
		s_traverseOrder_[0] = 0;
		s_traverseOrder_[1] = 1;
		s_traverseOrder_[2] = 2;
		s_traverseOrder_[3] = 3;
	}
	else if (viewDirection.x > 0.f && viewDirection.z <= 0.f)
	{
		s_traverseOrder_[0] = 2;
		s_traverseOrder_[1] = 3;
		s_traverseOrder_[2] = 0;
		s_traverseOrder_[3] = 1;
	}
	else if (viewDirection.x <= 0.f && viewDirection.z <= 0.f)
	{
		s_traverseOrder_[0] = 3;
		s_traverseOrder_[1] = 2;
		s_traverseOrder_[2] = 1;
		s_traverseOrder_[3] = 0;
	}
	else
	{
		s_traverseOrder_[0] = 1;
		s_traverseOrder_[1] = 0;
		s_traverseOrder_[2] = 3;
		s_traverseOrder_[3] = 2;
	}
}



uint8 ChunkQuadTree::s_traverseOrder_[4] = { 0, 1, 2, 3 };


/**
 *	Calculate the visible chunks by traversing the quad tree nodes
 *	@param viewProjection the projection matrix used to determine if a chunk is visible
 *	@param chunkList the list of chunks to fill in with visible chunks
 */
void ChunkQuadTree::QuadTreeNode::calculateVisible( const Matrix& viewProjection, BW::vector<Chunk*>& chunkList )
{
	BW_GUARD;
	if (!bb_.insideOut())
	{
		// Check our bounding box against the view frustum
		bb_.calculateOutcode( viewProjection );

		// If the bounding box is completely outside the view frustum we are done
		if (!bb_.combinedOutcode())
		{
			// If the bounding box is completely inside the view frustum
			// or this node only contains 1 chunk, this node is visible
			if (!bb_.outcode() || chunkCount_ == 1)
			{
				allVisible(chunkList);
			}
			else
			{
				// Traverse deeper into the quad tree
				for (uint32 i = 0; i < 4; i++)
				{
					nodes_[s_traverseOrder_[i]]->calculateVisible( viewProjection, chunkList );
				}
			}
		}
	}
}


/**
 *	This method collects all the chunks in this quad tree node as visible
 *	@param chunkList the list of chunks to fill in with visible chunks
 */
void ChunkQuadTree::QuadTreeNode::allVisible(BW::vector<Chunk*>& chunkList)
{
	BW_GUARD;
	if (chunkCount_ > 1)
	{
		// If there are more than 1 chunks in our list, check the bounding boxes of
		// the child nodes to see if we should skip some nodes
		if (nodes_[0]->bb_.insideOut() ||
			nodes_[1]->bb_.insideOut() ||
			nodes_[2]->bb_.insideOut() ||
			nodes_[3]->bb_.insideOut())
		{
			// Traverse deeper into the quad tree
			for (uint32 i = 0; i < 4; i++)
			{
				QuadTreeNode* node = nodes_[s_traverseOrder_[i]];
				if (!node->bb_.insideOut())
				{
					node->allVisible(chunkList);
				}
			}
		}
		else
		{
			// Add all chunks that have not already been traverse to the chunk list
			Chunk** ppChunk = pStartChunk_;
			for (uint32 i = 0; i < chunkCount_; i++, ppChunk++)
			{
				if (*ppChunk && (*ppChunk)->traverseMark() != Chunk::s_nextMark_)
				{
					(*ppChunk)->traverseMark( Chunk::s_nextMark_ );
					chunkList.push_back( *ppChunk );
				}
			}
		}
	}
	else
	{
		// Only one chunk in this node.
		// If this chunk has not yet been traversed add it to the chunk list
		if ((*pStartChunk_)->traverseMark() != Chunk::s_nextMark_)
		{
			(*pStartChunk_)->traverseMark( Chunk::s_nextMark_ );
			chunkList.push_back( *pStartChunk_ );
		}
	}
}


/**
 *	Recursively init the quad tree node
 *	@param pStartChunk pointer to the first chunk pointer in this node
 *	@param chunkCount number of chunks contained in this node
 *	@param nextAllocation the next QuadTreeNode to use as a child of this node
 */
void ChunkQuadTree::QuadTreeNode::init(Chunk** pStartChunk, 
			uint32 chunkCount, ChunkQuadTree::QuadTreeNode* &nextAllocation)
{
	BW_GUARD;
	// Set up bounding box
	bb_ = BoundingBox::s_insideOut_;

	// Initialise our contents
	pStartChunk_ = pStartChunk;
	chunkCount_ = chunkCount;

	// Our children all get a quarter of our chunk pointers
	uint32 nextCount = chunkCount / 4;
	if (nextCount > 0)
	{
		// Since this is not a leaf node initialise children
		for (uint32 i = 0; i < 4; i++)
		{
			nodes_[i] = nextAllocation++;
			nodes_[i]->init( pStartChunk, nextCount, nextAllocation );
			pStartChunk += nextCount;
		}
	}
	else
	{
		// Initialise our node pointer as we are a leaf node
		for (uint32 i = 0; i < 4; i++)
		{
			nodes_[i] = NULL;
		}
	}
}


/**
 *	This method recursively inserts a chunk into the quad tree
 *	@param pChunk the chunk to insert
 *	@param location the normalised 2d location to insert the chunk
 */
void ChunkQuadTree::QuadTreeNode::insert( Chunk* pChunk, Vector2 location )
{
	BW_GUARD;
	// If this node has 1 entry, we are a leaf node
	if (chunkCount_ == 1)
	{
		// Set our chunk and bounding box
		*pStartChunk_ = pChunk;
		bb_ = pChunk->visibilityBox();
	}
	else
	{
		// Work out where in the quad tree to push the node further down
		uint32 index = 0;
		if (location.x > 0.5f)
		{
			index = 1;
			location.x -= 0.5f;
		}
		if (location.y > 0.5f)
		{
			index += 2;
			location.y -= 0.5f;
		}
		location *= 2.f;

		// Push the node further into the quad tree
		nodes_[index]->insert( pChunk, location );

		// Recalculate the bounding box for this node
		bb_ = BoundingBox::s_insideOut_;
		for (uint32 i = 0; i < 4; i++)
		{
			if (!nodes_[i]->bb_.insideOut())
			{
				bb_.addBounds( nodes_[i]->bb_ );
			}
		}
	}
}

/**
 *	This method recursively removes a chunk from the quad tree
 *	@param location the normalised 2d location to insert the chunk
 */
void ChunkQuadTree::QuadTreeNode::remove( Vector2 location )
{
	BW_GUARD;
	// If this node has 1 entry, we are a leaf node
	if (chunkCount_ == 1)
	{
		// Invalidate our chunk and bounding box
		*pStartChunk_ = NULL;
		bb_ = BoundingBox::s_insideOut_;
	}
	else
	{
		// Work out where in the quad tree to remove the node further down
		uint32 index = 0;
		if (location.x > 0.5f)
		{
			index = 1;
			location.x -= 0.5f;
		}
		if (location.y > 0.5f)
		{
			index += 2;
			location.y -= 0.5f;
		}
		location *= 2.f;

		// Remove the node further from the quad tree
		nodes_[index]->remove( location );

		// Recalculate the bounding box for this node
		bb_ = BoundingBox::s_insideOut_;
		for (uint32 i = 0; i < 4; i++)
		{
			if (!nodes_[i]->bb_.insideOut())
			{
				bb_.addBounds( nodes_[i]->bb_ );
			}
		}
	}
}

BW_END_NAMESPACE

// chunk_quad_tree.cpp
