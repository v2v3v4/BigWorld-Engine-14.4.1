#ifndef PHYSICAL_CHUNK_SPACE_HPP_
#define PHYSICAL_CHUNK_SPACE_HPP_

#include "physical_space.hpp"
#include "network/basictypes.hpp"
#include "cstdmf/bw_namespace.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk_loading/edge_geometry_mappings.hpp"
#include "chunk_loading/geometry_mapper.hpp"

BW_BEGIN_NAMESPACE

typedef SmartPointer< ChunkSpace > ChunkSpacePtr;

class PhysicalChunkSpace: public IPhysicalSpace
{
public:
	
	PhysicalChunkSpace( SpaceID id, GeometryMapper & mapper );
	~PhysicalChunkSpace() {}
	
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

	/* Override from IPhysicalSpace */
	BoundingBox bounds() const /* override */;

	/* Override from IPhysicalSpace */
	BoundingBox subBounds() const /* override */;

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
	
	/**
	 *	A Chunk Space getter for cases that really require the underlying
	 *	Chunk Space. This 
	 */
	ChunkSpacePtr pChunkSpace() const { return pChunkSpace_; }

private:

	/**
	 *	This class is used by loadResource to notify the result of loading.
	 */
	class LoadResourceCallback : public AddMappingAsyncCallback 
	{
	public:
		LoadResourceCallback( SpaceID spaceID );
		virtual ~LoadResourceCallback() {};

		virtual void onAddMappingAsyncCompleted(
				SpaceEntryID spaceEntryID,
				AddMappingAsyncCallback::AddMappingResult result );

	private:
		SpaceID spaceID_;
	};


	ChunkSpacePtr pChunkSpace_;
	EdgeGeometryMappings	geometryMappings_;

};

BW_END_NAMESPACE

#endif // PHYSICAL_CHUNK_SPACE_HPP_
