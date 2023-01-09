#ifndef CHUNK_USER_DATA_OBJECT_HPP
#define CHUNK_USER_DATA_OBJECT_HPP

#include "chunk_cache.hpp"
#include "chunk_item.hpp"
#include "chunk.hpp"
#include "cstdmf/guard.hpp"
#include "math/matrix.hpp"
#include "cstdmf/bw_string.hpp"
#include "user_data_object.hpp"


BW_BEGIN_NAMESPACE

class UserDataObjectType; 
class Space;


/**
    The use of which shall be to define placable metadata objects that can be
	embedded into each chunk using world editor. 
*/
class ChunkUserDataObject : public ChunkItem
{
	DECLARE_CHUNK_ITEM( ChunkUserDataObject )

public:
	ChunkUserDataObject();
	virtual ~ChunkUserDataObject();
	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* pErrorString = NULL);
	virtual void toss( Chunk * pChunk );
	void bind();

protected:
	SmartPointer<UserDataObjectType>	pType_;
	SmartPointer< UserDataObject > pUserDataObject_;


private:
	UserDataObjectInitData initData_;
};

typedef SmartPointer<ChunkUserDataObject> ChunkUserDataObjectPtr;
/**
 *	This class is a cache of ChunkUserDataObjects, so we can create their python
 *	objects when they are bound and not before.
 */
class ChunkUserDataObjectCache : public ChunkCache
{
public:
	ChunkUserDataObjectCache( Chunk & chunk );
	~ChunkUserDataObjectCache();

	virtual void bind( bool isUnbind );

	void add( ChunkUserDataObject * pUserDataObject );
	void del( ChunkUserDataObject * pUserDataObject );

	static Instance<ChunkUserDataObjectCache> instance;

private:
	typedef BW::vector< ChunkUserDataObject * > ChunkUserDataObjects;
	ChunkUserDataObjects	userDataObjects_;
};

BW_END_NAMESPACE

#endif //CHUNK_USER_DATA_OBJECT_HPP
