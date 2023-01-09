#ifndef BASE_CHUNK_TREE_HPP
#define BASE_CHUNK_TREE_HPP

#include "math/boundbox.hpp"
#include "math/matrix.hpp"
#include "chunk_item.hpp"
#include <memory>


BW_BEGIN_NAMESPACE

class BSPTree;

/**
 *	This class is a body of water as a chunk item
 */
class BaseChunkTree : public ChunkItem
{
public:
	BaseChunkTree();
	~BaseChunkTree();

	virtual void toss( Chunk * pChunk );

	const Matrix & transform() const { return this->transform_; }
	virtual void setTransform( const Matrix & transform );

	virtual bool localBB( BoundingBox & bbRet ) const { bbRet = boundingBox_; return true; }

protected:
	const BSPTree * bspTree() const;
	void setBSPTree( const BSPTree * bspTree );

	const BoundingBox & boundingBox() const { return this->boundingBox_; }
	void setBoundingBox( const BoundingBox & bbox );

private:
	Matrix transform_;
	const BSPTree * bspTree_;
	BoundingBox     boundingBox_;

private:

	// Disallow copy
	BaseChunkTree( const BaseChunkTree & );
	const BaseChunkTree & operator = ( const BaseChunkTree & );
};

BW_END_NAMESPACE

#endif // BASE_CHUNK_TREE_HPP
