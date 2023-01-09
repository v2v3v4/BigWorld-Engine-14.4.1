#ifndef CHUNK_ENTITY_HPP
#define CHUNK_ENTITY_HPP

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_cache.hpp"


BW_BEGIN_NAMESPACE

class BWEntity;
class EntityType;
class MemoryOStream;
typedef SmartPointer< BWEntity > BWEntityPtr;

/**
 *	This class is a static or client-side entity in a saved chunk.
 */
class ChunkEntity : public ChunkItem
{
	DECLARE_CHUNK_ITEM( ChunkEntity )

public:
	ChunkEntity();
	~ChunkEntity();

	bool load( DataSectionPtr pSection, Chunk * pChunk );

	virtual void toss( Chunk * pChunk );

	void bind();

private:
	ChunkEntity( const ChunkEntity& );
	ChunkEntity& operator=( const ChunkEntity& );

	void makeEntity();

	BWEntityPtr			pEntity_;

	bool				wasCreated_;
	const EntityType *	pType_;
	Matrix				worldTransform_;
	DataSectionPtr		pPropertiesDS_;
};


/**
 *	This class is a cache of ChunkEntities, so we can create their python
 *	objects when they are bound and not before.
 */
class ChunkEntityCache : public ChunkCache
{
public:
	ChunkEntityCache( Chunk & chunk );
	~ChunkEntityCache();

	virtual void bind( bool isUnbind );

	void add( ChunkEntity * pEntity );
	void del( ChunkEntity * pEntity );

	static Instance<ChunkEntityCache> instance;

private:
	typedef BW::vector< ChunkEntity * > ChunkEntities;
	ChunkEntities	entities_;
};

BW_END_NAMESPACE

#endif // CHUNK_ENTITY_HPP
