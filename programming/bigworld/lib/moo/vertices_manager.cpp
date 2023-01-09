#include "pch.hpp"
#include "vertices_manager.hpp"

#include <algorithm>

#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"
#include "render_context.hpp"
#include "graphics_settings.hpp"


DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

namespace Moo
{

VerticesManager::VerticesManager()
{
	BW_GUARD;
}

VerticesManager::~VerticesManager()
{
	BW_GUARD;
	while (!vertices_.empty())
	{
		vertices_.erase( vertices_.begin() );
	}
}


VerticesManager* VerticesManager::instance( )
{
	SINGLETON_MANAGER_WRAPPER( VerticesManager )
	return pInstance_;
}

void VerticesManager::deleteManagedObjects( )
{
	BW_GUARD;
	SimpleMutexHolder smh( verticesLock_ );

	VerticesMap::iterator it = vertices_.begin();
	VerticesMap::iterator end = vertices_.end();

	while( it != end )
	{
		it->second->release();
		++it;
	}
}

void VerticesManager::createManagedObjects( )
{
	BW_GUARD;
	SimpleMutexHolder smh( verticesLock_ );

	VerticesMap::iterator it = vertices_.begin();
	VerticesMap::iterator end = vertices_.end();

	while( it != end )
	{
		it->second->load();
		++it;
	}
}


/**
 *	Get the given vertices resource
 *
 *	If parameter 'numNodes' greater than zero, vertex node indices are verified
 *	to be less than 'numNodes', vertex indices that fail verification will
 *	be set to zero, and an error message will be displayed.
 *	If parameter 'numNodes' is zero, no verification is done.
 *	This only applies to skinned vertices.
 */
VerticesPtr VerticesManager::get( const BW::string& resourceID, int numNodes /* = 0 */ )
{
	BW_GUARD;
	PROFILER_SCOPED( VerticesManager_get );

	VerticesPtr res = this->find( resourceID );
	if (res) 
	{
		return res;
	}

	res = new Vertices( resourceID, numNodes );
	res->load();

	this->addInternal( &*res );

	return res;
}

void VerticesManager::populateResource( const BW::string& resourceID, 
	const VerticesPtr& pVerts)
{
	VerticesPtr res = this->find( resourceID );
	if (res)
	{
		// We already loaded that resource, ignore the new version
		ASSET_MSG( "Population of vertices resource failed as resource"
			" has already been loaded. %s \n",
			resourceID.c_str() );
		return;
	}

	this->addInternal( &*pVerts );
}

void VerticesManager::tryDestroy( const Vertices * pVertices,
	bool isInVerticesMap )
{
	BW_GUARD;
	if (VerticesManager * vm = instance())
	{
		SimpleMutexHolder smh( vm->verticesLock_ );
		if (pVertices->refCount() == 0)
		{
			if (isInVerticesMap)
			{
				vm->delInternal( pVertices );
			}
			delete pVertices;
		}
	}
	else
	{
		MF_ASSERT( pVertices->refCount() == 0 );
		delete pVertices;
	}
}


/**
 *	Add this vertices to the map of those already loaded
 */
void VerticesManager::addInternal( Vertices * pVertices )
{
	BW_GUARD;
	SimpleMutexHolder smh( verticesLock_ );

	vertices_.insert( std::make_pair(pVertices->resourceID(), pVertices) );
}


/**
 *	Remove this vertices from the map of those already loaded
 */
void VerticesManager::delInternal( const Vertices * pVertices )
{
	BW_GUARD;

	VerticesMap::iterator it = vertices_.find( pVertices->resourceID() );
	if (it != vertices_.end())
	{
		vertices_.erase( it );
		return;
	}
	else
	{
		for (it = vertices_.begin(); it != vertices_.end(); it++)
		{
			if (it->second == pVertices)
			{
				//DEBUG_MSG( "VerticesManager::del: %s\n",
				//	pVertices->resourceID().c_str() );
				vertices_.erase( it );
				return;
			}
		}
	}

	ERROR_MSG( "VerticesManager::del: "
		"Could not find vertices at 0x%p to delete it\n", pVertices );
}


/**
 *	Find all the vertices in the map of those already loaded
 *  who belong to the primitiveFile
 *  @param primitiveFile	the .primitive file name
 *	@param vertices			the returned list of VerticesPtr
 *	@param howMany			return howMany are found
 */
void VerticesManager::find(  const BW::string & primitiveFile, 
						   VerticesPtr vertices[], size_t& howMany  )
{
	BW_GUARD;
	SimpleMutexHolder smh( verticesLock_ );

	howMany = 0;
	for (VerticesMap::iterator it = vertices_.begin();
		it != vertices_.end(); ++it )
	{
		if (it->first.find( primitiveFile ) == 0 )
		{
			vertices[ howMany++ ] = it->second;
		}
	}
}


/**
 *	Find this vertices in the map of those already loaded
 */
VerticesPtr VerticesManager::find( const BW::string & resourceID )
{
	BW_GUARD;
	SimpleMutexHolder smh( verticesLock_ );

	VerticesMap::iterator it = vertices_.find( resourceID );

	if (it != vertices_.end())
	{
		return it->second;
	}
	return NULL;
}

VerticesManager* VerticesManager::pInstance_ = NULL;

void VerticesManager::init()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pInstance_ == NULL )
	{
		return;
	}
	pInstance_ = new VerticesManager;
	REGISTER_SINGLETON( VerticesManager )
}

void VerticesManager::fini()
{
	BW_GUARD;
	MF_ASSERT_DEV( pInstance_ );

	bw_safe_delete(pInstance_);
}


} // namespace Moo

BW_END_NAMESPACE

// vertices_manager.cpp
