#include "script/first_include.hpp"

#include "physical_delegate_space.hpp"

#include "delegate_interface/space_world.hpp"
#include "delegate_interface/space_world_factory.hpp"
#include "delegate_interface/space_physics_delegate.hpp"

#include "cstdmf/bw_namespace.hpp"
#include "cellapp.hpp"

BW_BEGIN_NAMESPACE

PhysicalDelegateSpace::PhysicalDelegateSpace( SpaceID id ) : id_(id) 
{
	MF_ASSERT( IGameDelegate::instance() != NULL );
	
	spaceWorldFactory_ =  IGameDelegate::instance()->getSpaceWorldFactory();
	
	MF_ASSERT( spaceWorldFactory_ != NULL );
	
    createSpacePhysicsDelegate();
}
	

void PhysicalDelegateSpace::createSpacePhysicsDelegate()
{
	MF_ASSERT( !spacePhysicsDelegate_ );

	spacePhysicsDelegate_ = spaceWorldFactory_->CreateSpacePhysicsDelegate();
			
	if (!spacePhysicsDelegate_)
	{
		ERROR_MSG("Failed to create SpacePhysicsDelegate.");
	}
    else
    {
        DEBUG_MSG("Created physics world successfully. Height %f Width %f",         
            this->bounds().height(), this->bounds().width());
    }

}

PhysicalDelegateSpace::~PhysicalDelegateSpace()
{
	this->clear();
    
    spaceWorldFactory_->RemoveSpacePhysicsDelegate(spacePhysicsDelegate_);
}
	
/**
 *	This method requests to load the specified resource, 
 *	possibly using a translation matrix (which is currently ignored),
 *	and assigns it the given SpaceEntryID.
 *	(The loading itself may be performed asynchronously.)
 */
void PhysicalDelegateSpace::loadResource( const SpaceEntryID & mappingID, 
										 const BW::string & path,
										 float * matrix)
{
	if (paths_.find( mappingID ) != paths_.end() )
	{
		INFO_MSG( "PhysicalDelegateSpace::loadResource: Reusing mapping %s"
				  " for entry id %s\n", path.c_str(), mappingID.c_str() );
		return;
	}
	
	spaceWorldFactory_->LoadWorld( path.c_str() );

	INFO_MSG( "PhysicalDelegateSpace::loadResource: Started adding mapping %s"
			  " for entry id %s\n", path.c_str(), mappingID.c_str() );
	
	paths_[mappingID] = path;
}

/**
 *	This method requests to unload the resource specified by the given
 *	SpaceEntryID.
 *	(The unloading itself may be performed asynchronously.)
 */
void PhysicalDelegateSpace::unloadResource( const SpaceEntryID & mappingID )
{
	PathMap::iterator findP = paths_.find( mappingID );
	if ( findP == paths_.end() )
	{
		INFO_MSG( "PhysicalDelegateSpace::unloadResource: Cannot find mapping"
				  " for entry id %s\n", mappingID.c_str() );
		return;
	}
	
	BW::string path = findP->second;
	
	WorldMap::iterator findW = worlds_.find( mappingID );
	if ( findW != worlds_.end() )
	{
		spaceWorldFactory_->RemoveWorldInstance( findW->second );
		INFO_MSG( "PhysicalDelegateSpace::unloadResource: Removing world instance"
				  " for entry id %s (path: %s)\n", mappingID.c_str(), path.c_str() );
		worlds_.erase( findW );
	}
	
	spaceWorldFactory_->RemoveWorld( path.c_str() );
	INFO_MSG( "PhysicalDelegateSpace::unloadResource: Requesting to delete world"
			  " resources for entry id %s (path: %s)\n", mappingID.c_str(), path.c_str() );
	paths_.erase( findP );	
}


/**
 *	This method loads and/or unloads resources, to cover the axis-aligned
 *	rectangle (the updated area that we serve).
 *	Return value: whether any new resources have been loaded.
 */
bool PhysicalDelegateSpace::update( const BW::Rect & rect, bool unloadOnly )
{
	//This loop ensures that the worlds are being loaded. For the worlds which
    //are not yet loaded, it will call CreateWorldInstance, which will
    //succeed as soon as the worlds resources have finished loading.
	for ( PathMap::const_iterator it = paths_.begin(); it != paths_.end(); ++it )
	{
		const SpaceEntryID & id = it->first;
		const BW::string & path = it->second;
		ISpaceWorldPtr world = getLoadedWorld( id, path );
		if ( world == NULL )
		{
			//const char * path = it->second.c_str();
			DEBUG_MSG("PhysicalDelegateSpace::update, calling CreateWorldInstance"
					   " with path [%s]", path.c_str());
			world = spaceWorldFactory_->CreateWorldInstance( path.c_str() );
			if ( world )
			{
				worlds_[ id ] = world;
				INFO_MSG( "PhysicalDelegateSpace::update: Added a world "
						  "instance for entry id %s (path: %s)\n", 
						  id.c_str(), path.c_str() );
			}
			else
			{
				if ( spaceWorldFactory_->IsLoadingWorldFailed( path.c_str() ) )
				{
					ERROR_MSG( "PhysicalDelegateSpace::update: "
							   " loading world (id %s, path %s) failed! "
							   " Removing the path", 
								id.c_str(), path.c_str() );

					paths_.erase( id );
					
					//Break the loop here since the paths_ has been modified.
					//Further updates will happen on the next call to update
					//in a few moments.
					break;
				}
			}		
		}
	}  
	
	//ToDo: implement when load balancing is supported!
	return false;
}

ISpaceWorldPtr PhysicalDelegateSpace::getLoadedWorld(
												const SpaceEntryID & id, 
												const BW::string & path) const
{
	WorldMap::const_iterator findW = worlds_.find( id );
	ISpaceWorldPtr world;

	if ( findW != worlds_.end() ) // We have a world instance
	{
		world = findW->second;
		MF_ASSERT(world != NULL);
	} 
	
	return world;
}

/**
 *	This method does the best it can to determine an axis-aligned
 *	rectangle that has been loaded. If there is no loaded resource, 
 *	then a very big rectangle is returned.
 */
void PhysicalDelegateSpace::getLoadedRect( BW::Rect & rect ) const
{
	rect = BW::Rect( FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX );
	size_t loadedPaths = 0;
	
	for ( PathMap::const_iterator it = paths_.begin(); it != paths_.end(); ++it )
	{
		MF_ASSERT(spaceWorldFactory_);
		if ( spaceWorldFactory_->IsFullyLoadedWorld(it->second.c_str()) ) 
		{
			++loadedPaths;
		}
	}
	// if all the requested worlds have finished loading, return the
	// -FLT_MAX..FLT_MAX rectangle, which indicates for the cellapp mgr that
    // all the required "geometry" has finished loading, and the onAllSpaceGeometryLoaded
	// callback should be called
	if ( loadedPaths != 0 && loadedPaths == paths_.size() ) // all worlds are loaded
	{	
		rect = BW::Rect( -FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX );
	}
	else
	{
		// otherwise, when some worlds haven't finished loading (or there are no worlds at all)
		// return the empty rectangle
		rect = BW::Rect( 0, 0, 0, 0 );
	}
}


/*
 *  Overrides from IPhysicalSpace
 */
bool PhysicalDelegateSpace::getLoadableRects( IPhysicalSpace::BoundsList & rects ) const /* override */
{
	/*
	 * TODO WRITE THE CORRECT IMPLEMENTATION (Despair version)
	 */
	return false;
}


/**
 *	This method returns the minimal axis-aligned bounding box that contains
 *	all physical objects (including the terrain) in the space.
 */
BoundingBox PhysicalDelegateSpace::bounds() const
{
    MF_ASSERT( spacePhysicsDelegate_ );

    float minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
    spacePhysicsDelegate_->getBounds( &minX, &maxX, &minY, &maxY, &minZ, &maxZ );
    
    return BoundingBox( Vector3( minX, minY, minZ ), Vector3( maxX, maxY, maxZ ) );
}

/**
 *	This function goes through any resources that have been requested to be 
 *	loaded, and cancels their loading process.
 */
void PhysicalDelegateSpace::cancelCurrentlyLoading()
{
	for ( PathMap::const_iterator it = paths_.begin(); it != paths_.end(); ++it )
	{
		const SpaceEntryID & id = it->first;
		// If we do not have a world instance yet, it may still being loaded:
		if ( worlds_.find( id ) == worlds_.end() ) 
		{
			spaceWorldFactory_->CancelLoadingWorld( it->second.c_str() );
		} 		
	}
}

/**
 *	This method returns whether or not the space is fully unloaded.
 */
bool PhysicalDelegateSpace::isFullyUnloaded() const
{
	return paths_.empty();
}


/**
 *	This method prepares the space to be reused.
 */
void PhysicalDelegateSpace::reuse()
{
	this->clear();
}


/**
 *	This method unloads all resources and clears all state.
 */
void PhysicalDelegateSpace::clear()
{
	// First, cancel currently loading worlds:
	this->cancelCurrentlyLoading();
	
	// Second, remove all world instances:
	for ( WorldMap::iterator it = worlds_.begin(); it != worlds_.end(); ++it )
	{
		spaceWorldFactory_->RemoveWorldInstance( it->second );
		INFO_MSG( "PhysicalDelegateSpace::clear: Removing world instance"
				  " for entry id %s \n", it->first.c_str() );
	}
	worlds_.clear();
	
	// Third, ask to remove all world resources:
	for ( PathMap::iterator it = paths_.begin(); it != paths_.end(); ++it )
	{
		spaceWorldFactory_->RemoveWorld( it->second.c_str() );
		INFO_MSG( "PhysicalDelegateSpace::clear: Requesting to delete world"
				  " resources for entry id %s (path: %s)\n", 
				  it->first.c_str(), it->second.c_str() );
	}
	paths_.clear();
}

BW_END_NAMESPACE
