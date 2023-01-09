#include "pch.hpp"

#include "model_map.hpp"

#include "model.hpp"

#include "moo/reload.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

ModelMap::ModelMap()
{
	BW_GUARD;
	BWResource::instance().addModificationListener( this );
}


ModelMap::~ModelMap()
{
	BW_GUARD;
	// Check if BWResource singleton exists as this singleton could
	// be destroyed afterwards
	if (BWResource::pInstance() != NULL)
	{
		BWResource::instance().removeModificationListener( this );
	}
}


/**
 *	Add a model to the map
 */
void ModelMap::add( Model * pModel, const BW::StringRef & resourceID )
{
	BW_GUARD;
	RecursiveMutexHolder lock( mutex_ );

	//map_[ resourceID ] = pModel;
	Map::iterator found = map_.find( resourceID );
	if (found == map_.end())
	{
		map_.insert( std::make_pair( resourceID, pModel ) );
	}
	else
	{
		// The current model is about to be removed from the map.
		// So flag it as such.
		found->second->isInModelMap( false );
		found->second = pModel;
	}
}


/**
 *	Safely delete a model from the map and free memory
 */
void ModelMap::tryDestroy( const Model * pModel, bool isInModelMap )
{
	BW_GUARD;
	RecursiveMutexHolder lock( mutex_ );
	if (pModel->refCount() == 0)
	{
		if (isInModelMap)
		{
			Map::iterator it = map_.find( pModel->resourceID() );
			if (it != map_.end())
			{
				// One would assume this should match the object
				MF_ASSERT( pModel == it->second );
				map_.erase( it );
			}
		}
		delete pModel;
	}
}


/**
 *	Find a model in the map
 */
ModelPtr ModelMap::find( const BW::StringRef & resourceID )
{
	BW_GUARD;
	RecursiveMutexHolder lock( mutex_ );

	Map::iterator found = map_.find( resourceID );
	if (found == map_.end()) return NULL;

	return found->second;
}


/**
 *	Find all the models in the map that are the immediate children of
 *	a model with the given resource ID
 */
void ModelMap::findChildren( const BW::StringRef & parentResID,
	BW::vector< ModelPtr > & children )
{
	BW_GUARD;
	RecursiveMutexHolder lock( mutex_ );

	children.clear();

	for (Map::iterator it = map_.begin(); it != map_.end(); it++)
	{
		Model * chPar = it->second->parent();
		if (chPar != NULL && chPar->resourceID() == parentResID)
		{
			children.push_back( it->second );
		}
	}
}


#if ENABLE_RELOAD_MODEL

namespace
{
class ReloadContext
{
public:
	ReloadContext( const ModelPtr& model ) :
		pModel_( model )
	{}

	void reload( ResourceModificationListener::ReloadTask& task )
	{
		BWResource::instance().purge( task.resourceID(), true );

		bool succeed = pModel_->reload( true );

		TRACE_MSG( "ModelMap: modified '%s', reload %s\n",
			task.resourceID().c_str(), succeed? "succeed.":"failed." );
	}

private:
	ModelPtr pModel_;
};
} // End anon namespace

#endif // ENABLE_RELOAD_MODEL

/**
 *	Notification from the file system that something we care about has been
 *	modified at run time. Let us reload model.
 */
void ModelMap::onResourceModified( 
	const BW::StringRef& basePath, const BW::StringRef& resourceID,
	ResourceModificationListener::Action action )
{
	BW_GUARD;

#if ENABLE_RELOAD_MODEL
	if (!Reloader::enable())
	{
		return;
	}

	BW::StringRef ext = BWResource::getExtension( resourceID );
	if ( ext == "model" )//since we don't have ModelManager so we put it here.
	{
		ModelPtr pModel = Model::get( resourceID, false );
		if (pModel)
		{
			queueReloadTask( ReloadContext(pModel), basePath, resourceID );
		}
		return;
	}
#endif//RELOAD_MODEL_SUPPORT

}

BW_END_NAMESPACE

// model_map.cpp
