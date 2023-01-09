#ifndef _DYNAMIC_SCENE_PROVIDER_HPP
#define _DYNAMIC_SCENE_PROVIDER_HPP

#include "forward_declarations.hpp"
#include "dynamic_scene_provider_types.hpp"

#include <scene/scene_provider.hpp>
#include <scene/tick_scene_view.hpp>
#include <scene/collision_scene_view.hpp>
#include <scene/intersect_scene_view.hpp>
#include <cstdmf/bw_vector.hpp>
#include <cstdmf/object_pool.hpp>
#include <cstdmf/lookup_table.hpp>
#include <math/loose_octree.hpp>

namespace BW
{

class ObjectChangeSceneView;

class DynamicSceneProvider : 
	public SceneProvider,
	public ITickSceneProvider,
	public ICollisionSceneViewProvider,
	public IIntersectSceneViewProvider,
	public ITickSceneListener
{
public:
	
	DynamicSceneProvider();

	void setPartitionSizeHint( float nodeSizeHint );
	void resizeScene( const AABB& sceneBB );

	DynamicObjectHandle addObject( const SceneObject & obj, 
		const Matrix & worldXForm, 
		const AABB & collisionBB, 
		const AABB & visibilityBB);
	void removeObject( const DynamicObjectHandle & handle );
	void updateObject( const DynamicObjectHandle & handle, 
		const Matrix & worldXForm, 
		const AABB & collisionBB,
		const AABB & visibleBB );

private:
	// Scene view interfaces
	virtual void * getView( 
		const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID );
	virtual void onSetScene( Scene* pOldScene, Scene* pNewScene );

	virtual void tick( float dTime );
	virtual void updateAnimations( float dTime );
	virtual void onPreTick();
	virtual void onPostTick();
	
	virtual bool collide( const Vector3 & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & cs ) const;

	virtual bool collide( const WorldTriangle & source,
		const Vector3 & extent,
		const SweepParams& sp,
		CollisionState & cs ) const;

	virtual size_t intersect( const SceneIntersectContext& context, 
		const ConvexHull & hull, 
		IntersectionSet & intersection) const;

private:
	class Detail;
	class DebugDraw;

	DynamicObjectHandle insertObject_internal( const SceneObject & object,
		const Matrix & worldXform, 
		const AABB & collisionBB, 
		const AABB & visibilityBB);
	void removeObject_internal( const DynamicObjectHandle & handle );
	void updateObject_internal( const DynamicObjectHandle & handle, 
		const Matrix & worldXForm, 
		const AABB & collisionBB,
		const AABB & visibleBB );

	typedef DynamicLooseOctree Octree;
	typedef BW::vector<DynamicObjectHandle> OctreeNodeContents;
	typedef BW::LookUpTable< OctreeNodeContents > OctreeContents; 

	struct DynamicObjectStorageInfo
	{
		Octree::NodeIndex collisionNode_;
		Octree::NodeIndex visibilityNode_;
	};

	typedef BW::vector<SceneObject> SceneObjectCollection;
	typedef BW::vector<DynamicObjectHandle> ObjectHandleCollection;
	SceneObjectCollection objects_;
	ObjectHandleCollection objectHandles_;

	typedef HandleTable< DynamicObjectHandle > DynamicObjectHandlePool;
	typedef LookUpTable<AABB> BoundingBoxCollection;
	typedef LookUpTable<size_t> SceneObjectLookup;
	typedef LookUpTable<Matrix> TransformCollection;
	typedef LookUpTable<DynamicObjectStorageInfo> DynamicObjectTable;
	typedef LookUpTable<uint8> DirtyObjectTable;

	DynamicObjectHandlePool dynamicObjectHandles_;
	SceneObjectLookup objectLookup_;
	BoundingBoxCollection objectCollisionBBs_;
	BoundingBoxCollection objectVisibilityBBs_;
	TransformCollection objectWorldXform_;
	DynamicObjectTable objectStorageInfo_;
	
	BoundingBoxCollection latestObjectCollisionBBs_;
	BoundingBoxCollection latestObjectVisibilityBBs_;
	TransformCollection latestObjectWorldXform_;
	DirtyObjectTable objectDirtyFlags_;

	Octree collisionTree_;
	OctreeContents collisionTreeContents_;
	Octree visibilityTree_;
	OctreeContents visibilityTreeContents_;

	ObjectChangeSceneView* pObjectChangeView_;
	uint32 numDirtyObjects_;
	float partitionSizeHint_;
};

} // namespace BW

#endif // _DYNAMIC_SCENE_PROVIDER_HPP