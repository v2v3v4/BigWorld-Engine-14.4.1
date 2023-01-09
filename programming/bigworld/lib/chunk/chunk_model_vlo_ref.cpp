#include "pch.hpp"
#include "chunk_model_vlo_ref.hpp"
#include "chunk/chunk_vlo_obstacle.hpp"

BW_BEGIN_NAMESPACE

int ChunkModelVLORef_token;

ChunkModelVLORef::ChunkModelVLORef(
	WantFlags wantFlags /*= WANTS_DRAW */)
	: ChunkVLO( wantFlags )
	, collisionAdded_( false )
{
}


/**
 *	This static method creates a VLO reference from the input section and adds
 *	it to the given chunk.
 *
 *	@param pChunk			Chunk to add a VLO reference to.
 *	@param pSection			Source data section.
 *	@return	Result of the load/reference addition.
 */
ChunkItemFactory::Result ChunkModelVLORef::create(
	Chunk * pChunk, DataSectionPtr pSection )
{
	BW_GUARD;
	ChunkModelVLORef * pVLO = new ChunkModelVLORef( (WantFlags)( WANTS_DRAW ) );
	BW::string type = pSection->readString( ChunkVLO::getTypeAttrName(), "" );
	BW::string uid = pSection->readString( ChunkVLO::getUIDAttrName(), "" );
	return ChunkVLO::create( pVLO, pChunk, pSection );
}


/*static */BW::string & ChunkModelVLORef::getSectionName()
{
	static BW::string PROXY_VLO_SECT_NAME = "proxyvlo";
	return PROXY_VLO_SECT_NAME;
}


/*virtual */void ChunkModelVLORef::removeCollisionScene()
{
	ChunkVLOObstacle::instance( *pChunk_ ).delObstacles( this );
	collisionAdded_ = false; 
}


/*virtual */void ChunkModelVLORef::addCollisionScene()
{
	if (collisionAdded_)
	{
		return;
	}
	collisionAdded_ = true; 
	object()->addCollision( this );
}


ChunkItemFactory ChunkModelVLORef::factory_(
	ChunkModelVLORef::getSectionName(), 0, ChunkModelVLORef::create );

BW_END_NAMESPACE


// chunk_model_vlo_ref.cpp
