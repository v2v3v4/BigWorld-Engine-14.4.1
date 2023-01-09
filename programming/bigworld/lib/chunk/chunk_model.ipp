// chunk_model.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

#include "chunk.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This method sets the transform of the model (local to its chunk).
 *	Updates the world matrix as well.
 *	@param newTransform new local transform of model.
 */
INLINE void ChunkModel::transform( const Matrix& newTransform )
{
	transform_ = newTransform;
	this->updateWorldTransform( this->chunk() );
}


/**
 *	Get the local transform of the model.
 */
INLINE const Matrix& ChunkModel::transform() const
{
	return transform_;
}


/**
 *	Reset the world transform.
 */
INLINE void ChunkModel::updateWorldTransform( const Chunk * pChunk )
{
	if (pChunk != NULL)
	{
		worldTransform_ = pChunk->transform();
		worldTransform_.preMultiply( transform_ );
	}
	else
	{
		worldTransform_ = transform_;
	}
}


/**
 *	Get the world transform of the model.
 */
INLINE const Matrix& ChunkModel::worldTransform() const
{
	return worldTransform_;
}

BW_END_NAMESPACE

// chunk_model.ipp
