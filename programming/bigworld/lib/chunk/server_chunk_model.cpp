#include "server_chunk_model.hpp"

#include "chunk.hpp"
#include "chunk_model_obstacle.hpp"
#include "geometry_mapping.hpp"
#include "server_super_model.hpp"


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

int ServerChunkModel_token;
extern uint32 g_chunkSize;

// ----------------------------------------------------------------------------
// Section: ServerChunkModel
// ----------------------------------------------------------------------------
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, &errorString)
IMPLEMENT_CHUNK_ITEM( ServerChunkModel, model, 0 )
IMPLEMENT_CHUNK_ITEM_ALIAS( ServerChunkModel, shell, 0 )

ServerChunkModel::ServerChunkModel() :
	ChunkItem( (WantFlags)(WANTS_DRAW | WANTS_TICK) ),
	pSuperModel_( NULL ),
	transform_( Matrix::identity )
{
}


ServerChunkModel::~ServerChunkModel()
{
	delete pSuperModel_;
}

// in chunk.cpp
extern void readMooMatrix( DataSectionPtr pSection, const BW::string & tag,
	Matrix &result );


/**
 *	This method returns the section name for a chunk model.
 */
/*static*/ const BW::string & ServerChunkModel::getSectionName()
{
	static const BW::string s_chunkModelSectionName( "model" );
	return s_chunkModelSectionName;
}


/**
 *	Load yourself from this section
 */
bool ServerChunkModel::load( DataSectionPtr pSection,
		BW::string* pErrorString )
{
	label_ = pSection->asString();

	BW::vector<BW::string> models;
	pSection->readStrings( "resource", models );

	pSuperModel_ = new SuperModel( models );

	if (pSuperModel_->nModels() > 0)
	{
		// load our transform
		readMooMatrix( pSection, "transform", transform_ );
	}
	else
	{
		// This can happen if the super model only has models without BSPs
		// (e.g. skinned objects). In the server we only care about the model's
		// BSP so if it has none then we don't care about it.
		// ERROR_MSG( "No models loaded into SuperModel\n" );

		delete pSuperModel_;
		pSuperModel_ = NULL;
	}

	return true;
}


/**
 *	This method returns the label of this ServerChunkModel.
 */
const char * ServerChunkModel::label() const
{
	return label_.c_str();
}

/**
 *	Add this model to (or remove it from) this chunk
 */
void ServerChunkModel::toss( Chunk * pChunk )
{
	// remove it from old chunk
	if (pChunk_ != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
	}

	// call base class method
	this->ChunkItem::toss( pChunk );

	// add it to new chunk
	if ((pChunk_ != NULL) && (pSuperModel_ != NULL))
	{
		Matrix world( pChunk_->transform() );
		world.preMultiply( transform_ );

		for (int i = 0; i < pSuperModel_->nModels(); i++)
		{
			ChunkModelObstacle::instance( *pChunk_ ).addModel(
				pSuperModel_->topModel( i ), world, this );
		}
	}
}

BoundingBox ServerChunkModel::localBB() const
{
	BoundingBox bb;
	if (pSuperModel_)
	{
		pSuperModel_->boundingBox( bb );
	}

	return bb;
}

#if 0

/**
 *	overridden lend method
 */
void ChunkModel::lend( Chunk * pLender )
{
	if (pSuperModel_ != NULL && pChunk_ != NULL)
	{
		Matrix world( pChunk_->transform() );
		world.preMultiply( this->transform_ );

		BoundingBox bb;
		pSuperModel_->boundingBox( bb );
		bb.transformBy( world );

		this->lendByBoundingBox( pLender, bb );
	}
}
#endif // 0

BW_END_NAMESPACE

// server_chunk_model.cpp
