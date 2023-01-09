#ifndef SPACE_WORLD_FACTORY_HPP
#define SPACE_WORLD_FACTORY_HPP

#include "space_world.hpp"
#include "space_physics_delegate.hpp"

BW_BEGIN_NAMESPACE

/**
 *	ISpaceWorldFactory is an interface for loading and creating space worlds.
 *  It also defines SpacePhysicsDelegate factory interface.
 *
 *	The workflow is:
 *	1) request the World resources to be loaded from a path p: LoadWorld(p)
 *	2) it is possible to cancel the request if it is not completed yet: 
 *	CancelLoadingWorld(p)
 *	3) check completion: IsFullyLoadedWorld(p)
 *	4) if completed, request a World Instance: inst = GetWorldInstance(p)
 *	5) when not needed anymore: RemoveWorldInstance(inst)
 *	6) when no more new instances of this World will ever be needed, delete
 *	the World resources: RemoveWorld(p)
 *	
 */
class ISpaceWorldFactory
{
public:
	/**
	 *	Asynchronously load all the resources for the given world.
	 */
	virtual void LoadWorld(const char* path, 
							bool blockUntilLoaded = false,
							bool filterObjects = true) = 0;

	/**
	 *	Cancel the resource loading request for the given world.
	 *	Do nothing if this world is not currently being loaded.	 *	
	 */
	virtual void CancelLoadingWorld(const char* path) = 0;
	
	/**
	 *	Are all the resources for the given world completely loaded?
	 */
	virtual bool IsFullyLoadedWorld(const char* path) = 0;

	/**
	 *	Did the resource loading for the given world failed?
	 */
	virtual bool IsLoadingWorldFailed(const char* path) = 0;	
	
	/**
	 *	Create an instance of a World given that its resources
	 *	are completely loaded, otherwise return a NULL pointer.
	 *	If this instance has not been added to the EmpireMgr yet - add it.
	 */
	virtual ISpaceWorldPtr CreateWorldInstance(const char* path) = 0;

	/**
	 *	Delete all references for the given world, cancel its current loading
	 *	request if there is any, and remove it from the EmpireMgr. 
	 *	If as the result no references to the world resources will
	 *	remain, the resources will be automatically unloaded.
	 *	Note: Any World Instance must be separately removed in addition.
	 */
	virtual void RemoveWorld(const char* path) = 0;
	
	/**
	 *	Delete the given world Instance, without actually unloading the world
	 *	resources.
	 */	
	virtual void RemoveWorldInstance(ISpaceWorldPtr& world) = 0;
        
	/**
	 * Create an instance of a ISpacePhysicsDelegate, which represents a wrapper of the
	 * "physical world" and other cell-related aspects in the delegate
	 */
	virtual ISpacePhysicsDelegatePtr CreateSpacePhysicsDelegate() = 0;

	/**
	 *	Deletes an instance ISpacePhysicsDelegate, when the appropriate cell is destroyed.
	 */
	virtual void RemoveSpacePhysicsDelegate(ISpacePhysicsDelegatePtr physicsCell) = 0;
	
}; 


BW_END_NAMESPACE

#endif // SPACE_WORLD_FACTORY_HPP
