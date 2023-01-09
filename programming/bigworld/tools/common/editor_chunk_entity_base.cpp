#include "pch.hpp"

#include "editor_chunk_entity_base.hpp"

#include "chunk/geometry_mapping.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Our load method. We can't call (or reference) the base class's method
 *	because it would not compile (chunk item has no load method)
 */
bool EditorChunkEntityBase::load( DataSectionPtr pSection )
{
	BW_GUARD;

	transform_ = pSection->readMatrix34( "transform", Matrix::identity );

	return true;
}


/**
 *  This is called when the entity is removed from, or added to a Chunk.  Here
 *  we delete the link, it gets recreated if necessary later on, and call the
 *  base class.
 */
/*virtual*/ void EditorChunkEntityBase::toss( Chunk * pChunk )
{
	BW_GUARD;

    if (pChunk)
	{
		EditorChunkEntityCache::instance( *pChunk ).add( this );
		ChunkItem::toss( pChunk );
	}
	else
	{
		EditorChunkEntityCache::instance( *chunk() ).del( this );
		ChunkItem::toss( pChunk );
	}
}


/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection)
IMPLEMENT_CHUNK_ITEM( EditorChunkEntityBase, entity, 1 )



void EditorChunkEntityCache::getEntityPositions(
	BW::vector<Vector3>& entityPts ) const
{
	BW::set<EditorChunkEntityBasePtr>::const_iterator iter = entities_.begin();
	Matrix transform( chunk_.transform() );

	while (iter != entities_.end())
	{
		Vector3 position = transform.applyPoint(
			(*iter)->edTransform().applyToOrigin() + Vector3( 0, 1.f, 0 ) );

		entityPts.push_back( position );
		++iter;
	}
}


ChunkCache::Instance<EditorChunkEntityCache> EditorChunkEntityCache::instance;

BW_END_NAMESPACE
// editor_chunk_entity_base.cpp
