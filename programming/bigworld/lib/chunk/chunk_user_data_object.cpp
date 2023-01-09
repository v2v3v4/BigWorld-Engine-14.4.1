#include "pch.hpp"

#include "chunk_user_data_object.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/unique_id.hpp"
#include "cstdmf/guard.hpp"
#include "chunk.hpp"
#include "chunk_item.hpp"
#include "chunk_space.hpp"
#include "math/matrix.hpp"
#include "user_data_object_type.hpp"
#include "user_data_object.hpp"
#include "pyscript/script.hpp"
#include "cstdmf/bw_string.hpp"


DECLARE_DEBUG_COMPONENT2( "ChunkUserDataObject", 0 )


BW_BEGIN_NAMESPACE

int ChunkUserDataObject_token=1;

// force UserDataObjectLinkDataType token as well
extern int force_link_UDO_REF;
//static int s_tokenSet = force_link_UDO_REF;




// -----------------------------------------------------------------------------
// Section: ChunkUserDataObject
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */

ChunkUserDataObject::ChunkUserDataObject() :
	ChunkItem( WANTS_NOTHING ), // TODO: what wants flags do i need?
	pType_( NULL ),
	pUserDataObject_( NULL )
{

}

/**
 *	Destructor.
 */
ChunkUserDataObject::~ChunkUserDataObject()
{
	BW_GUARD;

	if (pUserDataObject_)
	{
		pUserDataObject_->decChunkItemRefCount();
	}
}


/**
 *  This static method creates a chunk entity from the input section
 *  and adds it to the given chunk, according to the domain of the UDO.
 */
ChunkItemFactory::Result ChunkUserDataObject::create( Chunk * pChunk, DataSectionPtr pSection )
{
	BW_GUARD;

	BW::string type = pSection->readString( "type" );

	if (UserDataObjectType::knownTypeInDifferentDomain( type.c_str() ))
	{
	    return ChunkItemFactory::SucceededWithoutItem();
	}

	ChunkUserDataObject* pItem = new ChunkUserDataObject();

	BW::string errorString;
	if ( pItem->load(pSection, pChunk, &errorString) )
	{
		pChunk->addStaticItem( pItem );
		return ChunkItemFactory::Result( pItem );
	}

	bw_safe_delete(pItem);
	return ChunkItemFactory::Result( NULL, errorString );
}
ChunkItemFactory ChunkUserDataObject::factory_( "UserDataObject", 0, ChunkUserDataObject::create );


/**
 *	Load method
 */
bool ChunkUserDataObject::load( DataSectionPtr pSection,Chunk* pChunk,BW::string *pErrorString)
{
	BW_GUARD;
	BW::string type = pSection->readString( "type" );
	pType_ = UserDataObjectType::getType( type.c_str() );
	if (pType_ == NULL)
	{
		if ( pErrorString )
		{
			*pErrorString = "ChunkUserDataObject::load: No such UserDataObject type '" +
				type +"'";
		}
		return false;
	}
	BW::string idStr = pSection->readString( "guid" );
	if ( idStr.empty() )
	{
		if ( pErrorString )
		{
			*pErrorString = "ChunkUserDataObject::load: UserDataObject has no GUID";
		}
		return false;
	}
	initData_.guid = UniqueID( idStr );
	DataSectionPtr pTransform = pSection->openSection( "transform" );
	if (pTransform)
	{
		Matrix m = pSection->readMatrix34( "transform", Matrix::identity );
		m.postMultiply( pChunk->transform() );
		initData_.position = m.applyToOrigin();
		initData_.direction = Direction3D(Vector3( m.roll(), m.pitch(), m.yaw() ));
	}
	else
	{
		if ( pErrorString )
		{
			*pErrorString = "ChunkUserDataObject::load: UserDataObject (of type '" +
				type + "') has no position";
		}
		return false;
	}
	/* Now create an object with properties using the properties
	   section */
	initData_.propertiesDS = pSection->openSection( "properties" );
	if (!initData_.propertiesDS)
	{
		if ( pErrorString )
		{
			*pErrorString = "ChunkUserDataObject::load: UserDataObject has no 'properties' section";
		}
		return false;
	}
	return true;
}

/**
  * This will be called by the main thread once the chunk has been loaded.
  */
void ChunkUserDataObject::bind()
{
	BW_GUARD;
	/* Dont create the entity more than once */
	if (pUserDataObject_ != NULL)
		return;

	pUserDataObject_ = UserDataObject::findOrLoad( initData_, pType_ );
	pUserDataObject_->incChunkItemRefCount();
}


/**
 *	Puts a chunk entity description into the given chunk
 */
void ChunkUserDataObject::toss( Chunk * pChunk )
{
	BW_GUARD;
	// Don't toss it if it wasn't properly loaded.
	if ( pType_ == NULL )
		return;

	// short-circuit this method if we're adding to the same chunk.
	// a chunk can do this sometimes when its bindings change.
	if (pChunk == pChunk_) return;

	// now follow the standard toss schema
	// note that we only do this if our entity has been created.
	// this makes sure that we don't try to call the entity manager
	// or anything in python from the loading thread.

	if (pChunk_ != NULL)
	{
		ChunkUserDataObjectCache::instance( *pChunk_ ).del( this );
	}

	this->ChunkItem::toss( pChunk );

	if (pChunk_ != NULL)
	{
		ChunkUserDataObjectCache::instance( *pChunk_ ).add( this );
	}
}


// -----------------------------------------------------------------------------
// Section: ChunkUserDataObjectCache
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
ChunkUserDataObjectCache::ChunkUserDataObjectCache( Chunk & chunk )
{
}

/**
 *	Destructor
 */
ChunkUserDataObjectCache::~ChunkUserDataObjectCache()
{
}


/**
 *	Bind (and unbind) method, lets us know when our chunk boundness changes
 */
void ChunkUserDataObjectCache::bind( bool isUnbind )
{
	BW_GUARD;
	for (ChunkUserDataObjects::iterator it = userDataObjects_.begin();
		it != userDataObjects_.end();
		it++)
	{
		(*it)->bind();
	}
}


/**
 *	Add an entity to our list
 */
void ChunkUserDataObjectCache::add( ChunkUserDataObject * pUserDataObject )
{
	BW_GUARD;
	userDataObjects_.push_back( pUserDataObject );
}

/**
 *	Remove an entity from our list
 */
void ChunkUserDataObjectCache::del( ChunkUserDataObject * pUserDataObject )
{
	BW_GUARD_PROFILER( ChunkUserDataObjectCache_del );
	//TODO: instead of using linear time operations, use a set or dont call del
	// for individual objects.
	ChunkUserDataObjects::iterator found =
		std::find( userDataObjects_.begin(), userDataObjects_.end(), pUserDataObject );
	if (found != userDataObjects_.end()) userDataObjects_.erase( found );
}

/// Static instance object initialiser
ChunkCache::Instance<ChunkUserDataObjectCache> ChunkUserDataObjectCache::instance;

BW_END_NAMESPACE

// chunk_user_data_object.cpp

