#include "pch.hpp"

#include "cstdmf/concurrency.hpp"
#include "resmgr/bwresource.hpp"
#include "visual_manager.hpp"

#include "primitive_manager.hpp"
#include "vertices_manager.hpp"

#include "moo/reload.hpp"
#include "moo/visual_common.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "visual_manager.ipp"
#endif

//-----------------------------------------------------------------------------
// Section: Resource Loading Mutex Holder
//-----------------------------------------------------------------------------
namespace
{
// declaration of ResourceLoadMutexHolder. 
class ResourceLoadMutexHolder
{ 
public: 
	ResourceLoadMutexHolder( const BW::StringRef& ); 
	~ResourceLoadMutexHolder(); 

	static void fini();

private: 
	BW::string resourceID_; 

	typedef BW::map< BW::string, SimpleMutex* > ResourceLoadMutexMap; 
	static SimpleMutex loadMapLock_; 
	static ResourceLoadMutexMap loadLocks_; 
}; 

/*static*/SimpleMutex ResourceLoadMutexHolder::loadMapLock_; 
/*static*/ResourceLoadMutexHolder::ResourceLoadMutexMap
ResourceLoadMutexHolder::loadLocks_;

/**
* creates a Mutex for each visual object being loaded
* so we can stop access to the same .visual resource from multiple threads
* BUGFIX: 32012
* 
* @Param: Resource ID of object being loaded.
*/
ResourceLoadMutexHolder::ResourceLoadMutexHolder( const BW::StringRef& resourceID ): 
resourceID_( resourceID.data(), resourceID.size() ) 
{ 
	loadMapLock_.grab(); 

	ResourceLoadMutexMap::iterator it = loadLocks_.find( resourceID_ ); 
	SimpleMutex* findMutex = NULL; 

	if( it != loadLocks_.end() ) 
	{ 
		findMutex = it->second; 
	} 
	else 
	{ 
		findMutex = new SimpleMutex(); 
		loadLocks_[ resourceID_ ] = findMutex; 
	} 

	assert( findMutex != NULL ); 

	loadMapLock_.give(); 
	findMutex->grab(); 
} 

ResourceLoadMutexHolder::~ResourceLoadMutexHolder() 
{ 
	loadMapLock_.grab(); 

	ResourceLoadMutexMap::iterator it = loadLocks_.find( resourceID_ ); 

	assert( it != loadLocks_.end() ); 

	loadMapLock_.give(); 
	it->second->give(); 
}
/*static*/ void ResourceLoadMutexHolder::fini()
{
	ResourceLoadMutexMap::iterator it = loadLocks_.begin();
	while ( it != loadLocks_.end() )
	{
		delete (it->second);
		it = loadLocks_.erase( it );
	}

}
}// *End:Anonymous Namespace*


//-----------------------------------------------------------------------------
// Section: Visual Manager
//-----------------------------------------------------------------------------
namespace Moo
{

VisualManager::VisualManager() :
	fullHouse_( false )
{
	REGISTER_SINGLETON( VisualManager )
	BWResource::instance().addModificationListener( this );
}

VisualManager::~VisualManager()
{
	BW_GUARD;

	// Check if BWResource singleton exists as this singleton could
	// be destroyed afterwards
	if (BWResource::pInstance() != NULL)
	{
		BWResource::instance().removeModificationListener( this );
	}

	ResourceLoadMutexHolder::fini();

	if (!visuals_.empty())
	{
		WARNING_MSG( "VisualManager::~VisualManager - not all visuals freed.\n" ); 
		VisualMap::iterator it = visuals_.begin();
		while (it != visuals_.end())
		{
			INFO_MSG( "-- Visual not deleted: %s\n", it->first.to_string().c_str() );
			it++;
		}
	}
}


VisualManager* VisualManager::instance()
{
	SINGLETON_MANAGER_WRAPPER( VisualManager )
	return pInstance_;
}


/**
 *	Get the given visual resource.
 */
VisualPtr VisualManager::get( const BW::StringRef& resourceID, bool loadIfMissing )
{
	BW_GUARD;

	// this creates a mutex lock on the .visual resource
	ResourceLoadMutexHolder resLoadLock( resourceID );

	// first check in the map
	VisualPtr p = this->find( resourceID );
	if (p) return p;

	if (!loadIfMissing)
	{
		return NULL;
	}
	// load it if not yet loaded
	if (VisualLoader<Visual>::visualFileExists( resourceID ))
	{
		if (fullHouse_)
		{
			CRITICAL_MSG( "VisualManager::getVisualResource: "
				"Loading the visual '%s' into a full house!\n",
				resourceID.to_string().c_str() );
		}

		p = new Visual( resourceID );

		this->add( &*p, resourceID );
	}
	return p;
}


/**
 *	Set whether or not we have a full house
 *	(and thus cannot load any new visuals)
 */
void VisualManager::fullHouse( bool noMoreEntries )
{
	fullHouse_ = noMoreEntries;
}


/**
 *	Add this visual to the map of those already loaded
 */
void VisualManager::add( Visual * pVisual, const BW::StringRef & resourceID )
{
	BW_GUARD;
	SimpleMutexHolder smh( visualsLock_ );

	VisualMap::iterator it = visuals_.insert(
		std::make_pair( resourceID, pVisual ) ).first;
}

void VisualManager::del(Visual* pVisual)
{
	BW_GUARD;
	if (pInstance_)
		pInstance_->delInternal( pVisual );
}

/**
 *	Remove this visual from the map of those already loaded
 */
void VisualManager::delInternal( Visual * pVisual )
{
	BW_GUARD;
	SimpleMutexHolder smh( visualsLock_ );

	for (VisualMap::iterator it = visuals_.begin(), end = visuals_.end();
		it != end; ++it)
	{
		if (it->second == pVisual)
		{
			//DEBUG_MSG( "VisualManager::del: %s\n", it->first.c_str() );
			visuals_.erase( it );
			return;
		}
	}

	ERROR_MSG( "VisualManager::del: "
		"Could not find visual at 0x%p to delete it\n", pVisual );
}


/**
 *	Find this visual in the map of those already loaded
 */
VisualPtr VisualManager::find( const BW::StringRef & resourceID )
{
	BW_GUARD;
	SimpleMutexHolder smh( visualsLock_ );

	VisualMap::iterator it = visuals_.find( resourceID );
	if (it == visuals_.end()) return NULL;

	// Visual is not a SafeReferenceCount object, because we assume
	// that it is shielded from multithreading effects by Models.
	// Since this is not entirely true (particle systems, etc.) it
	// is worthwhile taking at least some precautions here...
	// TODO: If this is the case Visual should be made SafeReferenceCount
	MF_ASSERT( it->second->refCount() );
	if (it->second->refCount() == 0) return NULL;
	return it->second;
}

VisualManager* VisualManager::pInstance_ = NULL;

void VisualManager::init()
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pInstance_ == NULL )
	{
		return;
	}

	pInstance_ = new VisualManager;
}

void VisualManager::fini()
{
	BW_GUARD;
	MF_ASSERT_DEV( pInstance_ );
	bw_safe_delete(pInstance_);
}


#if ENABLE_RELOAD_MODEL

namespace
{
class VisualManagerReloadContext
{
public:
	VisualManagerReloadContext() :
		foundPrimitive_( 0 ),
		foundVertices_( 0 )
	{

	}

	void reload( ResourceModificationListener::ReloadTask& task )
	{
		if (foundPrimitive_ > 0 || 
			foundVertices_ > 0)
		{
			BWResource::instance().purge( primitiveFile_, true );
			BWResource::instance().purge( primitiveFile_ + ".processed", true );

			//reload primitives
			for (size_t i = 0; i < foundPrimitive_; ++i)
			{
				primitives_[i]->reload( true );
			}
			//reload vertices
			for (size_t i = 0; i < foundVertices_; ++i)
			{
				vertices_[i]->reload( true );
			}
		}

		if (pVisual_)
		{
			BWResource::instance().purge( visualFile_, true );
			BWResource::instance().purge( visualFile_ + ".processed", true );

			//reload the visual in VisualManager
			bool succeed  = pVisual_->reload( true );

			TRACE_MSG( "VisualManager: modified '%s', reload %s\n",
				task.resourceID().c_str(), succeed? "succeed.":"failed." );
		}
	}

	PrimitivePtr primitives_[ 512 ];
	VerticesPtr vertices_[ 512 ];
	size_t foundPrimitive_;
	size_t foundVertices_;
	BW::string primitiveFile_;
	BW::string visualFile_;
	VisualPtr pVisual_;
};
} // End anon namespace

#endif // ENABLE_RELOAD_MODEL

/**
 *	Notification from the file system that something we care about has been
 *	modified at run time. Let us reload visual.
 */
void VisualManager::onResourceModified( 
	const BW::StringRef& basePath, const BW::StringRef& resourceID,
	ResourceModificationListener::Action action )
{
	BW_GUARD;
#if ENABLE_RELOAD_MODEL
	if (!Reloader::enable())
		return;

	BW::string baseFilePath = basePath.to_string();
	BW::StringRef resource = resourceID;
	BW::StringRef ext = BWResource::getExtension( resource );

	if ( ext == "processed" )
	{
		resource = BWResource::removeExtension( resourceID );
		ext = BWResource::getExtension( resource );

		if(!BWResource::fileAbsolutelyExists(basePath + resource))
		{
			//visual file isn't in same path as processed, try and find it
			IFileSystem::FileDataLocation filedataloc;
			if(!BWResource::instance().fileSystem()->locateFileData(resource, &filedataloc))
			{
				//Couldn't find this file anywhere, do nothing
				return;
			}
			//Split path out
			size_t pathlength = filedataloc.hostResourceID.size()-resource.size();
			baseFilePath = filedataloc.hostResourceID.substr(0, pathlength);
		}
	}

	if ( ext == "visual" )
	{
		VisualManagerReloadContext context;

		// Reload all visual, primitive and vertices.
		BW::StringRef nonExt = BWResource::removeExtension( resource );
		context.primitiveFile_ = nonExt + ".primitives";
		context.visualFile_ = nonExt + ".visual";

		PrimitiveManager::instance()->find(
			context.primitiveFile_, 
			context.primitives_, 
			context.foundPrimitive_ );
		VerticesManager::instance()->find( 
			context.primitiveFile_, 
			context.vertices_, 
			context.foundVertices_ );

		context.pVisual_ = VisualManager::instance()->get( 
			context.visualFile_, false );

		if (context.foundPrimitive_ > 0 || 
			context.foundVertices_ > 0 ||
			context.pVisual_)
		{
			queueReloadTask( context, BW::StringRef(baseFilePath), resource );
		}

		return;
	}
#endif//ENABLE_RELOAD_MODEL

}


} // namespace Moo

BW_END_NAMESPACE

// visual_manager.cpp
