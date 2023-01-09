#ifndef EDITOR_CHUNK_ENTITY_BASE_HPP
#define EDITOR_CHUNK_ENTITY_BASE_HPP


#include "chunk/chunk_item.hpp"
#include "chunk/chunk_cache.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of an entity in a chunk.
 *
 *	Note that it does not derive from the client's version of a chunk entity
 *	(because the editor does not have the machinery necessary to implement it)
 */
class EditorChunkEntityBase : public ChunkItem
{
	DECLARE_CHUNK_ITEM( EditorChunkEntityBase )
public:
	EditorChunkEntityBase() : transform_( Matrix::identity ) {}
	virtual const Matrix& edTransform()	{ return transform_; }

	bool load( DataSectionPtr pSection );
    virtual void toss( Chunk * pChunk );

protected:
	EditorChunkEntityBase( const EditorChunkEntityBase& );
	EditorChunkEntityBase& operator=( const EditorChunkEntityBase& );

	Matrix transform_;
};


typedef SmartPointer<EditorChunkEntityBase> EditorChunkEntityBasePtr;


class EditorChunkEntityCache : public ChunkCache
{
public:
	EditorChunkEntityCache( Chunk & chunk ) : chunk_( chunk ) {}

	void add( EditorChunkEntityBasePtr e ) { entities_.insert( e ); }
	void del( EditorChunkEntityBasePtr e ) { entities_.erase( e ); }

	void getEntityPositions( BW::vector<Vector3>& entityPts ) const;

	static Instance<EditorChunkEntityCache> instance;

private:
	Chunk& chunk_;
	BW::set<EditorChunkEntityBasePtr> entities_;
};


BW_END_NAMESPACE
#endif // EDITOR_CHUNK_ENTITY_BASE_HPP
