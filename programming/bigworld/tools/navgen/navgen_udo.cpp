#include "pch.hpp"
#include "navgen_udo.hpp"

#include "chunk/chunk_model.hpp"
#include "chunk/chunk_model_obstacle.hpp"
#include "chunk/geometry_mapping.hpp"

#include "model/super_model.hpp"
#include "moo/render_context.hpp"
#include "pyscript/script.hpp"
#include "pyscript/py_data_section.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"
#include "entitydef/constants.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: NavGenUDO
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
NavGenUDO::NavGenUDO() :
	transform_( Matrix::identity )
{
}


/**
 *	Destructor.
 */
NavGenUDO::~NavGenUDO()
{
	BW_GUARD;
}


/**
 *	Load method
 */
bool NavGenUDO::load( DataSectionPtr pSection )
{
	BW_GUARD;

	Matrix temp = pSection->readMatrix34( "transform" );
	transform_ = temp;
	pProps_ = pSection->openSection( "properties" );
	return true;
}


/**
 *	Toss method
 */
void NavGenUDO::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk == pChunk_) return;

	if (pChunk_ != NULL)
	{
		NavGenUDOCache::instance( *pChunk_ ).del( this );
	}

	this->ChunkItem::toss( pChunk );

	if (pChunk_ != NULL)
	{
		NavGenUDOCache::instance( *pChunk_ ).add( this );
		
		GirthSeed girthSeed;
		girthSeed.position = pChunk->transform().applyPoint( this->position() );
		girthSeed.girth = pProps_->readFloat( "girth", -1.f );
		girthSeed.generateRange = pProps_->readFloat( "generateRange", 0.f );
		if (girthSeed.girth > 0.f )
		{
			s_girthSeeds[this] = girthSeed;
		}
	}
	else
	{
		GirthSeeds::iterator it = s_girthSeeds.find( this );
		if (it != s_girthSeeds.end())
		{
			s_girthSeeds.erase( it );
		}
	}
}

ChunkItemFactory NavGenUDO::factory_( "UserDataObject", 1, NavGenUDO::create );
																			
ChunkItemFactory::Result NavGenUDO::create( Chunk * pChunk,
			DataSectionPtr pSection )
{
	if (pSection->readString("type") != "WayPointSeed")
	{
		return ChunkItemFactory::SucceededWithoutItem();
	}

	NavGenUDO * pItem = new NavGenUDO();

	if (pItem->load(pSection))
	{
		if (!pChunk->addStaticItem( pItem ))
		{
			ERROR_MSG( "NavGenUDO::create: "
					"error in section %s of %s in mapping %s\n",
				pSection->sectionName().c_str(),
				pChunk->identifier().c_str(),
				pChunk->mapping()->path().c_str() );
		}
		return ChunkItemFactory::Result( pItem );
	}

	delete pItem;
	return ChunkItemFactory::Result( NULL, BW::string() );
}

//IMPLEMENT_CHUNK_ITEM( NavGenUDO, UserDataObject, 1 )


// -----------------------------------------------------------------------------
// Section: NavGenUDOCache
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
NavGenUDOCache::NavGenUDOCache( Chunk & )
{
}

/**
 *	Destructor
 */
NavGenUDOCache::~NavGenUDOCache()
{
}

/**
 *	Add this entity
 */
void NavGenUDOCache::add( NavGenUDOPtr e )
{
	BW_GUARD;

	udos_.push_back( e );
}

/**
 *	Remove this entity
 */
void NavGenUDOCache::del( NavGenUDOPtr e )
{
	BW_GUARD;

	NavGenUDOs::iterator found = std::find(
		udos_.begin(), udos_.end(), e );
	if (found != udos_.end())
		udos_.erase( found );
}


/// Static instance accessor initialiser
ChunkCache::Instance<NavGenUDOCache> NavGenUDOCache::instance;

BW::map<NavGenUDO*,GirthSeed> NavGenUDO::s_girthSeeds;

BW_END_NAMESPACE


// NavGenUDO.cpp
