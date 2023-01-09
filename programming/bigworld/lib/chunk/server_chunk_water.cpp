#include "server_chunk_water.hpp"

#include "cstdmf/guard.hpp"

#include "physics2/worldtri.hpp"

#include "chunk.hpp"
#include "chunk_vlo_obstacle.hpp"
#include "geometry_mapping.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ServerChunkWater
// -----------------------------------------------------------------------------


int ServerChunkWater_token;

/**
 *	Constructor.
 */
ServerChunkWater::ServerChunkWater( const BW::string & uid ) :
	VeryLargeObject( uid, "water" ),
	orientation_(),
	depth_(),
	pTree_( NULL ),
	pModel_( NULL )
{
}


/**
 *	Destructor.
 */
ServerChunkWater::~ServerChunkWater()
{
	if (pTree_ != NULL)
	{
		bw_safe_delete( pTree_ );
	}
}


/**
 *	Load method
 */
bool ServerChunkWater::load( DataSectionPtr pSection, Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk == NULL)
	{
		return false;
	}

	DataSectionPtr pRequiredSection;

	pRequiredSection = pSection->openSection( "position" );
	if (pRequiredSection == NULL)
	{
		return false;
	}

	position_ = pRequiredSection->asVector3();

	pRequiredSection = pSection->openSection( "size" );
	if (pRequiredSection == NULL)
	{
		return false;
	}
	Vector3 sizeV3 = pRequiredSection->asVector3();
	size_ = Vector2( sizeV3.x, sizeV3.z );

	orientation_ = pSection->readFloat( "orientation", 0.f );

	depth_ = pSection->readFloat( "depth", 10.f );

	transform_ = Matrix::identity;
	transform_.setScale( size_.x, 1.f, size_.y );
	transform_.postRotateY( orientation_ );
	transform_.postTranslateBy( position_ );

	// Two triangle quad
	tris_.push_back( WorldTriangle(
			Vector3( -0.5f, 0.f, -0.5f ),
			Vector3( -0.5f, 0.f, 0.5f ),
			Vector3( 0.5f, 0.f, 0.5f ),
			TRIANGLE_WATER ) );
	tris_.push_back( WorldTriangle(
			Vector3( 0.5f, 0.f, 0.5f ),
			Vector3( 0.5f, 0.f, -0.5f ),
			Vector3( -0.5f, 0.f, -0.5f ),
			TRIANGLE_WATER ) );

	BoundingBox bb( Vector3( -0.5f, 0, -0.5f ), Vector3( 0.5f, 0, 0.5f ) );

	pTree_ = BSPTreeTool::buildBSP( tris_ );

	pModel_ = new ServerModel( pTree_, bb );

	return true;
}


/*virtual */void ServerChunkWater::addCollision( ChunkItemPtr item )
{
	ChunkVLOObstacle::instance( *item->chunk() ).addModel( pModel_, transform_, item );
}


/**
 *	This static method creates a body of water from the input section and adds
 *	it to the given chunk.
 */
bool ServerChunkWater::create(
	Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid )
{
	BW_GUARD;

	ServerChunkWater * pItem = new ServerChunkWater( uid );	

	if (!pItem->load( pSection, pChunk ))
	{
		bw_safe_delete(pItem);
		return false;
	}

	return true;
}


VLOFactory ServerChunkWater::factory_( "water", 0, ServerChunkWater::create );


BW_END_NAMESPACE

// server_chunk_water.cpp
