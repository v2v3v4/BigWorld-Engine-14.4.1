#ifndef PHYSICAL_SPACE_HPP
#define PHYSICAL_SPACE_HPP

#define LARGE_NUMBER_FOR_EMPTY_LOADED_RECT 1000000000000.f

#include "math/rectt.hpp"
#include "math/boundbox.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

class SpaceEntryID;

/**
 *	This abstract interface is used to represent a Physical space with mapped 
 *	resources, e.g. PhysicalChunkSpace or PhysicalDelegateSpace.
 */
class IPhysicalSpace : public ReferenceCount
{
public:
	typedef BW::list< BW::Rect > BoundsList;
	virtual ~IPhysicalSpace() {}
	
	/**
	 *	This method requests to load the specified resource, 
	 *	possibly using a translation matrix,
	 *	and assigns it the given SpaceEntryID.
	 *	(The loading itself may be performed asynchronously.)
	 */
	virtual void loadResource( const SpaceEntryID & mappingID, 
							   const BW::string & path,
							   float * matrix = NULL ) = 0; 
	
	/**
	 *	This method requests to unload the resource specified by the given
	 *	SpaceEntryID.
	 *	(The unloading itself may be performed asynchronously.)
	 */
	virtual void unloadResource( const SpaceEntryID & mappingID ) = 0;
	
	
	/**
	 *	This method loads and/or unloads resources, to cover the axis-aligned
	 *	rectangle (the updated area that we serve).
	 *	Return value: whether any new resources have been loaded.
	 */
	virtual bool update( const BW::Rect & rect, bool unloadOnly ) = 0;
	
	/**
	 *	This method does the best it can to determine an axis-aligned
	 *	rectangle that has been loaded. If there is no loaded resource, 
	 *	then a very big rectangle is returned.
	 */
	virtual void getLoadedRect( BW::Rect & rect ) const = 0;

	/**
	 *  This method fills the list of Rects with loadable bounds of all mappings.
	 *  @param rects reference to list of Rects to be filled with loadable bounds of all mappings.
	 *  @return false if a full answer cannot be provided immediately, perhaps
	 *  because resources are still being loaded, true otherwise.
	 */
	virtual bool getLoadableRects( BoundsList & rects ) const = 0;
	
	/**
	 *	This method returns the minimal axis-aligned bounding box that contains
	 *	all physical objects (including the terrain) in the space.
	 */
	virtual BoundingBox bounds() const = 0;

	/**
	 *	This method returns the minimal loaded axis-aligned bounding box that
	 *	contains all physical objects (including the terrain) in the space.
	 */
	virtual BoundingBox subBounds() const	{ return this->bounds(); }

	/**
	 *	This function goes through any resources that have been requested to be 
	 *	loaded, and cancels their loading process.
	 */
	virtual void cancelCurrentlyLoading() = 0;

	/**
	 *	This method returns whether or not the space is fully unloaded.
	 */
	virtual bool isFullyUnloaded() const = 0;

	/**
	 *	This method should prepare the space to be reused.
	 */
	virtual void reuse() = 0;

	/**
	 *	This method unloads all resources and clears all state.
	 */
	virtual void clear() = 0;
};

BW_END_NAMESPACE

#endif //PHYSICAL_SPACE_HPP
