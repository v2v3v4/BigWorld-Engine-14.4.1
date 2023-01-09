#ifndef NavGenUDO_HPP
#define NavGenUDO_HPP

#include "chunk/chunk_item.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_cache.hpp"


BW_BEGIN_NAMESPACE

struct GirthSeed
{
	Vector3 position;
	float	girth;
	float	generateRange;
};


/**
 *	This class is a UDO for the purposes of waypoint generation
 */
class NavGenUDO : public ChunkItem
{
	DECLARE_CHUNK_ITEM( NavGenUDO )

public:
	NavGenUDO();
	~NavGenUDO();

	bool load( DataSectionPtr pSection );

	virtual void toss( Chunk * pChunk );

	const Vector3 & position() const	{ return transform_.applyToOrigin(); }

	typedef BW::map<NavGenUDO*,GirthSeed> GirthSeeds;
	static  GirthSeeds s_girthSeeds;
private:
	DataSectionPtr pProps_;
	void finishLoad();

	NavGenUDO( const NavGenUDO& );
	NavGenUDO& operator=( const NavGenUDO& );

	Matrix		transform_;
};

typedef SmartPointer<NavGenUDO> NavGenUDOPtr;

typedef BW::vector<NavGenUDOPtr> NavGenUDOs;


/**
 *	This class is a cache of all the entities in a given chunk
 */
class NavGenUDOCache : public ChunkCache
{
public:
	NavGenUDOCache( Chunk & chunk );
	~NavGenUDOCache();

	NavGenUDOs::iterator begin()	{ return udos_.begin(); }
	NavGenUDOs::iterator end()		{ return udos_.end(); }

	void add( NavGenUDOPtr e );
	void del( NavGenUDOPtr e );

	static Instance<NavGenUDOCache> instance;

private:
	NavGenUDOs	udos_;
};

BW_END_NAMESPACE

#endif // NavGenUDO_HPP
