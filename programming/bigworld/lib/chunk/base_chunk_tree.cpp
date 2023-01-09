#include "pch.hpp"
#include "base_chunk_tree.hpp"
#include "chunk_obstacle.hpp"
#include "chunk_model_obstacle.hpp"

#include "physics2/bsp.hpp"


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: BaseChunkTree
// -----------------------------------------------------------------------------

#define DEBUG_CHUNKBSPOBSTACLE_DELETE 0
#if DEBUG_CHUNKBSPOBSTACLE_DELETE
	class TreeChunkBSPObstacle : public ChunkBSPObstacle
	{
	public:
		TreeChunkBSPObstacle(
			const BSPTree & bsp, const Matrix & transform,
			const BoundingBox & bb, ChunkItemPtr pItem ) :
				ChunkBSPObstacle( bsp, transform, bb, pItem )
		{}

		virtual ~TreeChunkBSPObstacle()
		{
			WARNING_MSG("~TreeChunkBSPObstacle\n");
		}
	};
#else  // DEBUG_CHUNKBSPOBSTACLE_DELETE
	typedef ChunkBSPObstacle TreeChunkBSPObstacle;
#endif // DEBUG_CHUNKBSPOBSTACLE_DELETE


/**
 *	Constructor.
 */
BaseChunkTree::BaseChunkTree() :
	ChunkItem( (WantFlags)(WANTS_DRAW | WANTS_SWAY) ),
	transform_(Matrix::identity),
	bspTree_( 0 ),
	boundingBox_( BoundingBox::s_insideOut_ )
{}


/**
 *	Destructor.
 */
BaseChunkTree::~BaseChunkTree()
{}


/**
 *	Add this model to (or remove it from) this chunk
 */
void BaseChunkTree::toss( Chunk * pChunk )
{
	BW_GUARD;
	// remove it from old chunk
	if (pChunk_ != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
	}

	// call base class method
	this->ChunkItem::toss( pChunk );

	// add it to new chunk
	if (pChunk_ != NULL)
	{
		Matrix world( pChunk_->transform() );
		world.preMultiply( this->transform_ );

		if (this->bspTree_ != NULL)
		{
			ChunkModelObstacle::instance( *pChunk_ ).addObstacle( 
				new TreeChunkBSPObstacle( this->bspTree_, 
					world, &this->boundingBox_, this ) );
		}
	}
}


void BaseChunkTree::setTransform( const Matrix & transform )
{
	this->transform_ = transform;
}


const BSPTree * BaseChunkTree::bspTree() const
{
	return this->bspTree_;
}


void BaseChunkTree::setBSPTree( const BSPTree * bspTree )
{
	BW_GUARD;
	this->bspTree_ = bspTree;
	if (this->pChunk_ != NULL)
	{
		ChunkModelObstacle::instance(*this->pChunk_).delObstacles(this);

		if (this->bspTree_ != NULL)
		{
			Matrix world( this->pChunk_->transform() );
			world.preMultiply( this->transform_ );
			ChunkModelObstacle::instance( *pChunk_ ).addObstacle( 
				new TreeChunkBSPObstacle( this->bspTree_, 
					world, &this->boundingBox_, this ) );
		}
	}
}


void BaseChunkTree::setBoundingBox( const BoundingBox & bbox )
{
	this->boundingBox_ = bbox;
}

BW_END_NAMESPACE

// base_chunk_tree.cpp
