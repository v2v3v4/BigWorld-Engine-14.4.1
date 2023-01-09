#ifndef PHYSICAL_DELEGATE_SPACE_HPP_
#define PHYSICAL_DELEGATE_SPACE_HPP_

#include "physical_space.hpp"
#include "network/basictypes.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/shared_ptr.hpp"

#include "delegate_interface/game_delegate.hpp"

#include <map>

BW_BEGIN_NAMESPACE

class ISpaceWorld;
typedef shared_ptr<ISpaceWorld> ISpaceWorldPtr;

class ISpacePhysicsDelegate;
typedef shared_ptr<ISpacePhysicsDelegate> ISpacePhysicsDelegatePtr;

class PhysicalDelegateSpace: public IPhysicalSpace 
{
public:
	
	PhysicalDelegateSpace( SpaceID id );
	~PhysicalDelegateSpace();
	
	/**
	 *	This method requests to load the specified resource, 
	 *	possibly using a translation matrix,
	 *	and assigns it the given SpaceEntryID.
	 *	(The loading itself may be performed asynchronously.)
	 */
	void loadResource( const SpaceEntryID & mappingID, 
					   const BW::string & path,
					   float * matrix = NULL ); 
	
	/**
	 *	This method requests to unload the resource specified by the given
	 *	SpaceEntryID.
	 *	(The unloading itself may be performed asynchronously.)
	 */
	void unloadResource( const SpaceEntryID & mappingID );
	
	
	/**
	 *	This method loads and/or unloads resources, to cover the axis-aligned
	 *	rectangle (the updated area that we serve).
	 *	Return value: whether any new resources have been loaded.
	 */
	bool update( const BW::Rect & rect, bool unloadOnly );
	
	/**
	 *	This method does the best it can to determine an axis-aligned
	 *	rectangle that has been loaded. If there is no loaded resource, 
	 *	then a very big rectangle is returned.
	 */
	void getLoadedRect( BW::Rect & rect ) const;
	
	bool getLoadableRects( IPhysicalSpace::BoundsList & rects ) const /* override */;

	/**
	 *	This method returns the minimal axis-aligned bounding box that contains
	 *	all physical objects (including the terrain) in the space.
	 */
	BoundingBox bounds() const;

	/**
	 *	This function goes through any resources that have been requested to be 
	 *	loaded, and cancels their loading process.
	 */
	void cancelCurrentlyLoading();

	/**
	 *	This method returns whether or not the space is fully unloaded.
	 */
	bool isFullyUnloaded() const;

	/**
	 *	This method prepares the space to be reused.
	 */
	void reuse() /* override */;

	/**
	 *	This method unloads all resources and clears all state.
	 */
	void clear();
	

private:
	SpaceID id_;
	
	typedef std::map< SpaceEntryID, BW::string > PathMap;
	PathMap paths_;
	
	typedef std::map< SpaceEntryID, ISpaceWorldPtr > WorldMap;
	WorldMap worlds_;
	
	ISpaceWorldFactory* spaceWorldFactory_;
        
	ISpacePhysicsDelegatePtr	spacePhysicsDelegate_;

	ISpaceWorldPtr getLoadedWorld(const SpaceEntryID & id, 
								   const BW::string & path) const;
	
	void createSpacePhysicsDelegate();
};

BW_END_NAMESPACE

#endif // PHYSICAL_DELEGATE_SPACE_HPP_
