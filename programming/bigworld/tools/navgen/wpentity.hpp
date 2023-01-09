#ifndef WPENTITY_HPP
#define WPENTITY_HPP

#include "chunk/chunk_item.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_cache.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is an Entity for the purposes of waypoint generation
 */
class WPEntity : public ChunkItem
{
	DECLARE_CHUNK_ITEM( WPEntity )

public:
	WPEntity();
	~WPEntity();

	bool load( DataSectionPtr pSection );

	virtual void toss( Chunk * pChunk );
	virtual void draw( Moo::DrawContext& drawContext );

	const Vector3 & position() const	{ return transform_.applyToOrigin(); }

private:
	BW::string typeName_;
	DataSectionPtr pProps_;
	void finishLoad();

	WPEntity( const WPEntity& );
	WPEntity& operator=( const WPEntity& );

	Matrix		transform_;
	class SuperModel			* pSuperModel_;
};

typedef SmartPointer<WPEntity> WPEntityPtr;

typedef BW::vector<WPEntityPtr> WPEntities;


/**
 *	This class is a cache of all the entities in a given chunk
 */
class WPEntityCache : public ChunkCache
{
public:
	WPEntityCache( Chunk & chunk );
	~WPEntityCache();

	WPEntities::iterator begin()	{ return entities_.begin(); }
	WPEntities::iterator end()		{ return entities_.end(); }

	void add( WPEntityPtr e );
	void del( WPEntityPtr e );

	static Instance<WPEntityCache> instance;

private:
	WPEntities	entities_;
};

BW_END_NAMESPACE

#endif // WPENTITY_HPP
