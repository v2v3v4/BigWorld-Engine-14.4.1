#include "pch.hpp"

#include "chunk_entity.hpp"

#include "entity.hpp"
#include "entity_type.hpp"
#include "entity_udo_factory.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkEntity
// -----------------------------------------------------------------------------


/// Static factory initialiser
ChunkItemFactory ChunkEntity::factory_( "entity", 0, ChunkEntity::create );

int ChunkEntity_token = 0;


/**
 *	Constructor.
 */
ChunkEntity::ChunkEntity() :
	pEntity_( NULL ),
	wasCreated_( false ),
	pType_( NULL ),
	pPropertiesDS_( NULL )
{
	BW_GUARD;	
}


/**
 *	Destructor.
 */
ChunkEntity::~ChunkEntity()
{
	BW_GUARD;
	if (pChunk_ != NULL) this->toss( NULL );
}


/**
 *	This static method creates a chunk entity from the input section
 *	and adds it to the given chunk. It first checks whether or not the
 *	client should automatically instantiate the entity.
 */
ChunkItemFactory::Result ChunkEntity::create( Chunk * pChunk,
	DataSectionPtr pSection )
{
	BW_GUARD;
	// if this is a server-instantiated-only entity, then ignore it
	// here 'coz the server will tell us about it when we get there.
	if ( !pSection->readBool( "clientOnly",false ) )
		return ChunkItemFactory::SucceededWithoutItem();

	// now continue with the standard creation code
	ChunkEntity * pEntity = new ChunkEntity();

	if (!pEntity->load( pSection, pChunk ))
	{
		bw_safe_delete(pEntity);
		return ChunkItemFactory::Result( NULL,
			"Failed to load entity of type " + pSection->readString( "type" ) );
	}
	else
	{
		pChunk->addStaticItem( pEntity );
		return ChunkItemFactory::Result( pEntity );
	}
}



/**
 *	Loads a chunk entity description from this section
 */
bool ChunkEntity::load( DataSectionPtr pSection, Chunk * pChunk )
{
	BW_GUARD;
	// our create method has already checked 'clientOnly'

	EntityID id = pSection->readInt( "id", NULL_ENTITY_ID );
	if (id != NULL_ENTITY_ID)
	{
		ERROR_MSG( "ChunkEntity::load: id property of entities specified in "
				"space geometry data is no longer supported\n" );
	}

	BW::string type = pSection->readString( "type" );
	pType_ = EntityType::find( type );
	if (pType_ == NULL)
	{
		ERROR_MSG( "No such entity type '%s'\n", type.c_str() );
		return false;
	}

	DataSectionPtr pTransform = pSection->openSection( "transform" );
	if (pTransform)
	{
		worldTransform_ = pSection->readMatrix34( "transform", Matrix::identity );
		worldTransform_.postMultiply( pChunk->transform() );
	}
	else
	{
		ERROR_MSG( "Entity (of type '%s') has no position\n", type.c_str() );
		return false;
	}

	pPropertiesDS_ = pSection->openSection( "properties" );

	if (!pPropertiesDS_)
	{
		ERROR_MSG( "Entity has no 'properties' section\n" );
		return false;
	}

	return true;
}


/**
 *	Puts a chunk entity description into the given chunk
 *
 *	We should consider having the entity entered only when our
 *	chunk is focussed. It may be less important once chunks unload
 *	themselves, but I think it should probably be done anyway.
 */
void ChunkEntity::toss( Chunk * pChunk )
{
	BW_GUARD;
	// short-circuit this method if we're adding to the same chunk.
	// a chunk can do this sometimes when its bindings change.
	if (pChunk == pChunk_) return;

	// now follow the standard toss schema
	// note that we only do this if our entity has been created.
	// this makes sure that we don't try to call the entity manager
	// or anything in python from the loading thread.

	if (pChunk_ != NULL)
	{
		if (pEntity_ != NULL && pChunk == NULL)
		{
			if (!pEntity_->isDestroyed())
			{
				// Destroy the entity if we're being tossed out
				// TODO: Archive the entity's data for later recreation
				EntityUDOFactory::destroyBWEntity( pEntity_.get() );
			}

			MF_ASSERT( pEntity_->isDestroyed() );

			pEntity_ = NULL;
		}

		ChunkEntityCache::instance( *pChunk_ ).del( this );
	}

	this->ChunkItem::toss( pChunk );

	if (pChunk_ != NULL)
	{
		ChunkEntityCache::instance( *pChunk_ ).add( this );

		if (wasCreated_ && pEntity_ == NULL)
		{
			// Recreate entity if we're tossed back in
			this->makeEntity();
		}
	}
}


/**
 *	This method is called when we are bound (or maybe unbound),
 *	anyway it is called early from the main thread.
 */
void ChunkEntity::bind()
{
	BW_GUARD;

	// For unbinding, just take our entity out of existence
	if (pChunk_ == NULL)
	{
		if (pEntity_ != NULL)
		{
			if (!pEntity_->isDestroyed())
			{
				// Destroy the entity if we're being tossed out
				// TODO: Archive the entity's data for later recreation
				EntityUDOFactory::destroyBWEntity( pEntity_.get() );
			}

			MF_ASSERT( pEntity_->isDestroyed() );

			pEntity_ = NULL;
		}
		return;
	}

	// If we already have an entity, we're done.
	if (pEntity_ != NULL)
	{
		return;
	}

	// Create an entity
	this->makeEntity();
}


/**
 *	This method makes the actual entity object in the entity manager
 */
void ChunkEntity::makeEntity()
{
	BW_GUARD;
	MF_ASSERT( pType_ != NULL );
	MF_ASSERT( pPropertiesDS_ != NULL );
	MF_ASSERT( pChunk_ != NULL );

	pEntity_ = EntityUDOFactory::createEntityWithType( pChunk_->space()->id(),
		pType_, worldTransform_, pPropertiesDS_ );

	if (pEntity_ == NULL)
	{
		// TODO: Currently happens when we shut down, as BWConnection is
		// gone and we get hit when putting the ChunkManager into
		// sync mode while deleting the mapping.
		// We need to slightly modify ChunkSpace::delMapping to mark the
		// mapping as condemned _before_ we switch to sync mode, so that we
		// can query pChunk_->mapping()->condemned() here.
		// Although then we wouldn't even get here, as a chunk which loads
		// in a condemned mapping deletes itself, I think.
		WARNING_MSG( "ChunkEntity::makeEntity: "
				"Failed to create \"%s\" ChunkEntity in space %u\n",
			pType_->name().c_str(), pChunk_->space()->id() );
	}

	wasCreated_ = true;
}



// -----------------------------------------------------------------------------
// Section: ChunkEntityCache
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
ChunkEntityCache::ChunkEntityCache( Chunk & chunk )
{
}

/**
 *	Destructor
 */
ChunkEntityCache::~ChunkEntityCache()
{
}


/**
 *	Bind (and unbind) method, lets us know when our chunk boundness changes
 */
void ChunkEntityCache::bind( bool isUnbind )
{
	BW_GUARD;
	for (ChunkEntities::iterator it = entities_.begin();
		it != entities_.end();
		it++)
	{
		(*it)->bind();
	}
}


/**
 *	Add an entity to our list
 */
void ChunkEntityCache::add( ChunkEntity * pEntity )
{
	BW_GUARD;
	entities_.push_back( pEntity );
}

/**
 *	Remove an entity from our list
 */
void ChunkEntityCache::del( ChunkEntity * pEntity )
{	
	BW_GUARD_PROFILER( ChunkEntityCache_del );
	ChunkEntities::iterator found =
		std::find( entities_.begin(), entities_.end(), pEntity );
	if (found != entities_.end()) entities_.erase( found );
}


/// Static instance object initialiser
ChunkCache::Instance<ChunkEntityCache> ChunkEntityCache::instance;

BW_END_NAMESPACE

// chunk_entity.cpp
