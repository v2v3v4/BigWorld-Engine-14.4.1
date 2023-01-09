#include "pch.hpp"
#include "primitive_manager.hpp"
#include "primitive.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

namespace Moo
{

PrimitiveManager::PrimitiveManager()
{
}

PrimitiveManager::~PrimitiveManager()
{
	BW_GUARD;
	while (!primitives_.empty())
	{
		primitives_.erase( primitives_.begin() );
	}
}

PrimitiveManager* PrimitiveManager::instance( )
{
	SINGLETON_MANAGER_WRAPPER( PrimitiveManager )
	return pInstance_;
}

void PrimitiveManager::deleteManagedObjects( )
{
	BW_GUARD;
	PrimitiveMap::iterator it = primitives_.begin();
	PrimitiveMap::iterator end = primitives_.end();

	while( it != end )
	{
		it->second->release();
		++it;
	}
}

void PrimitiveManager::createManagedObjects( )
{
	BW_GUARD;
	PrimitiveMap::iterator it = primitives_.begin();
	PrimitiveMap::iterator end = primitives_.end();

	while( it != end )
	{
		it->second->load();
		++it;
	}
}


/**
 *	Get the given primitive resource
 */
PrimitivePtr PrimitiveManager::get( const BW::string& resourceID )
{
	BW_GUARD;
	PrimitivePtr res = this->find( resourceID );
	if (res) return res;


	res = new Primitive( resourceID );
	res->load();

	this->addInternal( &*res );

	return res;
}

void PrimitiveManager::tryDestroy( const Primitive * pPrimitive,
	bool isInPrimativeMap )
{
	BW_GUARD;
	if (PrimitiveManager * pm = instance())
	{
		SimpleMutexHolder smh( pm->primitivesLock_ );
		if (pPrimitive->refCount() == 0)
		{
			if (isInPrimativeMap)
			{
				pm->delInternal( pPrimitive );
			}
			delete pPrimitive;
		}
	}
	else
	{
		MF_ASSERT(pPrimitive->refCount() == 0);
		delete pPrimitive;
	}
}

/**
 *	Add this primitive to the map of those already loaded
 */
void PrimitiveManager::addInternal( Primitive * pPrimitive )
{
	BW_GUARD;
	SimpleMutexHolder smh( primitivesLock_ );

	primitives_.insert( std::make_pair(pPrimitive->resourceID(), pPrimitive ) );
}


/**
 *	Remove this primitive from the map of those already loaded
 */
void PrimitiveManager::delInternal( const Primitive * pPrimitive )
{
	BW_GUARD;
	PrimitiveMap::iterator it = primitives_.find( pPrimitive->resourceID() );
	if (it != primitives_.end())
	{
		primitives_.erase( it );
		return;
	}
	else
	{
		for (it = primitives_.begin(); it != primitives_.end(); it++)
		{
			if (it->second == pPrimitive)
			{
				//DEBUG_MSG( "PrimitiveManager::del: %s\n",
				//	pPrimitive->resourceID().c_str() );
				primitives_.erase( it );
				return;
			}
		}
	}

	ERROR_MSG( "PrimitiveManager::del: "
		"Could not find primitive at 0x%p to delete it\n", pPrimitive );
}


/**
 *	Find all the primitives in the map of those already loaded
 *  who belong to the primitiveFile
 *  @param primitiveFile	the .primitive file name
 *	@param primitives		the returned list of PrimitivePtr
 *	@param howMany			return howMany are found
 */
void PrimitiveManager::find( const BW::string & primitiveFile, 
						PrimitivePtr primitives[], size_t& howMany )
{
	BW_GUARD;
	SimpleMutexHolder smh( primitivesLock_ );

	howMany = 0;
	for (PrimitiveMap::iterator it = primitives_.begin();
		it != primitives_.end(); ++it )
	{
		if (it->first.find( primitiveFile ) == 0 )
		{
			primitives[ howMany++ ] = it->second;
		}
	}
}

/**
 *	Find this primitive in the map of those already loaded
 */
PrimitivePtr PrimitiveManager::find( const BW::string & resourceID )
{
	BW_GUARD;
	SimpleMutexHolder smh( primitivesLock_ );

	PrimitiveMap::iterator it = primitives_.find( resourceID );

	if (it != primitives_.end())
	{
		return it->second;
	}

	return NULL;
}

PrimitiveManager* PrimitiveManager::pInstance_ = NULL;

void PrimitiveManager::init()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( !pInstance_ )
	{
		return;
	}
	pInstance_ = new PrimitiveManager;
	REGISTER_SINGLETON( PrimitiveManager )
}

void PrimitiveManager::fini()
{
	BW_GUARD;

	bw_safe_delete(pInstance_);
}

} // namespace Moo

BW_END_NAMESPACE

// primitive_manager.cpp
