#ifndef CELL_CHUNK_HPP
#define CELL_CHUNK_HPP

#include "chunk/chunk.hpp"
#include "chunk/chunk_cache.hpp"


BW_BEGIN_NAMESPACE

class Entity;

/**
 *	This class is used to add extra data to chunks that is used by the CellApp.
 */
class CellChunk: public ChunkCache
{
public:
	CellChunk( Chunk & chunk );
	virtual ~CellChunk();

	virtual void bind( bool isUnbind );

	bool hasEntities() const { return pFirstEntity_ != NULL; }
	void addEntity( Entity* pEntity );
	void removeEntity( Entity* pEntity );

	void propagateNoise(const Entity* who,
						float propagationRange,
						Vector3 position,
						float remainingRange,
						int event,
						int info,
						uint32 mark = 0);


	/**
	 *	Helper iterator class for iterating over all entities
	 */
	class EntityIterator
	{
	public:
		void operator++(int);
		bool  operator==(EntityIterator other ) { return pEntity_ == other.pEntity_; }
		bool  operator!=(EntityIterator other ) { return pEntity_ != other.pEntity_; }
		Entity* & operator*() { return pEntity_; }

	private:
		EntityIterator(Entity* pEntity) : pEntity_(pEntity) {}
		Entity* pEntity_;

		friend class CellChunk;
	};

	EntityIterator begin()	{ return EntityIterator( pFirstEntity_); }
	EntityIterator end()	{ return EntityIterator( NULL); }

	static Instance<CellChunk> instance;

private:
	void clearAllEntities();

	Chunk & chunk_;
	Entity* pFirstEntity_;
	Entity* pLastEntity_;
};

BW_END_NAMESPACE

#endif //CELL_CHUNK_HPP
