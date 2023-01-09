#include "pch.hpp"

#include "dynamic_scene_provider.hpp"

#include "scene/scene_listener.hpp"
#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/tick_scene_view.hpp"
#include "scene/intersection_set.hpp"
#include "scene/collision_operation.hpp"
#include "scene/scene_intersect_context.hpp"
#include "scene/object_change_scene_view.hpp"

#include "space/debug_draw.hpp"

#include "cstdmf/bw_vector.hpp"
#include "math/matrix.hpp"
#include "math/vector3.hpp"
#include "math/boundbox.hpp"
#include "math/convex_hull.hpp"
#include "math/math_extra.hpp"
#include "physics2/worldtri.hpp"
#include "physics2/sweep_helpers.hpp"

namespace BW
{

#if ENABLE_SPACE_DEBUG_DRAW
class DynamicSceneProvider::DebugDraw
{
public:
	static void registerWatchers()
	{
		if (s_debug_initializedWatchers_)
		{
			return;
		}
		s_debug_initializedWatchers_ = true;

		MF_WATCH("Render/CompiledSpace/DynamicScene_drawCollisionBBs", 
			s_debug_drawCollisionBBs_);
		MF_WATCH("Render/CompiledSpace/DynamicScene_drawCollisionTree", 
			s_debug_drawCollisionTreeLevel_);
		MF_WATCH("Render/CompiledSpace/DynamicScene_drawVisibilityBBs", 
			s_debug_drawVisibilityBBs_);
		MF_WATCH("Render/CompiledSpace/DynamicScene_drawVisibilityTree", 
			s_debug_drawVisibilityTreeLevel_);
	}

	static void draw( DynamicSceneProvider& provider )
	{
		if (s_debug_drawCollisionBBs_)
		{
			drawBoundingBoxes( provider.objectHandles_, 
				provider.objectCollisionBBs_, COLLISION_BB_COLOUR );
		}
		if (s_debug_drawVisibilityBBs_)
		{
			drawBoundingBoxes( provider.objectHandles_, 
				provider.objectVisibilityBBs_, VISIBILITY_BB_COLOUR );
		}
		if (s_debug_drawCollisionTreeLevel_)
		{
			drawOctree( provider.collisionTree_, 
				s_debug_drawCollisionTreeLevel_ - 1, COLLISION_TREE_BB_COLOUR );
		}
		if (s_debug_drawVisibilityTreeLevel_)
		{
			drawOctree( provider.visibilityTree_, 
				s_debug_drawVisibilityTreeLevel_ - 1, VISIBILITY_TREE_BB_COLOUR );
		}
	}

private:
	static const uint32 COLLISION_BB_COLOUR = 0xFFFF0000;
	static const uint32 VISIBILITY_BB_COLOUR = 0xFFFFFF00;
	static const uint32 COLLISION_TREE_BB_COLOUR = 0xFFFF8800;
	static const uint32 VISIBILITY_TREE_BB_COLOUR = 0xFFFF8800;

	static void drawBoundingBoxes( const ObjectHandleCollection& handles, 
		const BoundingBoxCollection& boxCollection, uint32 colour )
	{
		for (ObjectHandleCollection::const_iterator iter = handles.begin();
			iter != handles.end(); ++iter)
		{
			const DynamicObjectHandle& handle = *iter;

			SpaceDebugDraw::drawBoundingBox( 
				boxCollection[handle.index_], colour );
		}
	}

	static void drawOctree( Octree& tree, size_t level, uint32 colour )
	{
		SpaceDebugDraw::drawOctreeBoundingBoxes( tree, level, colour );
	}

	static bool s_debug_initializedWatchers_;
	static bool s_debug_drawCollisionBBs_;
	static bool s_debug_drawVisibilityBBs_;
	static size_t s_debug_drawCollisionTreeLevel_;
	static size_t s_debug_drawVisibilityTreeLevel_;
};

bool DynamicSceneProvider::DebugDraw::s_debug_initializedWatchers_ = false;
bool DynamicSceneProvider::DebugDraw::s_debug_drawCollisionBBs_ = false;
size_t DynamicSceneProvider::DebugDraw::s_debug_drawCollisionTreeLevel_ = 0;
bool DynamicSceneProvider::DebugDraw::s_debug_drawVisibilityBBs_ = false;
size_t DynamicSceneProvider::DebugDraw::s_debug_drawVisibilityTreeLevel_ = 0;

#endif

DynamicSceneProvider::DynamicSceneProvider()
	: numDirtyObjects_( 0 )
	, partitionSizeHint_( -1.0f )
	, pObjectChangeView_( NULL )
{
	// Basic initial sizes for dynamic scene until something
	// in the space mappings loads
	collisionTree_.initialise( Vector3::ZERO, 2000.0f, 1 );
	visibilityTree_.initialise( Vector3::ZERO, 2000.0f, 1 );

#if ENABLE_SPACE_DEBUG_DRAW
	DebugDraw::registerWatchers();
#endif
}


void * DynamicSceneProvider::getView( 
	const SceneTypeSystem::RuntimeTypeID & viewTypeID)
{
	void * result = NULL;

	exposeView<ITickSceneProvider>(this, viewTypeID, result);
	exposeView<ICollisionSceneViewProvider>(this, viewTypeID, result);
	exposeView<IIntersectSceneViewProvider>(this, viewTypeID, result);

	return result;
}


void DynamicSceneProvider::onSetScene( Scene* pOldScene, Scene* pNewScene )
{
	if (pOldScene)
	{
		TickSceneView* pTickScene = pOldScene->getView<TickSceneView>();
		pTickScene->removeListener( this );
		pObjectChangeView_ = NULL;
	}

	if (pNewScene)
	{
		pObjectChangeView_ = pNewScene->getView<ObjectChangeSceneView>();
		TickSceneView* pTickScene = pNewScene->getView<TickSceneView>();
		pTickScene->addListener( this );
	}
}


DynamicObjectHandle DynamicSceneProvider::addObject( 
	const SceneObject& obj, 
	const Matrix & worldXForm, const AABB & collisionBB, 
	const AABB & visibilityBB)
{
	DynamicObjectHandle handle = 
		insertObject_internal( obj, worldXForm, collisionBB, visibilityBB );

	pObjectChangeView_->notifyObjectsAdded( this, &obj, 1 );

	return handle;
}


void DynamicSceneProvider::removeObject( const DynamicObjectHandle & handle )
{
	MF_ASSERT( dynamicObjectHandles_.isValid( handle ) );
	if (!dynamicObjectHandles_.isValid( handle ))
	{
		return;
	}

	pObjectChangeView_->notifyObjectsRemoved( this, 
		&objects_[objectLookup_[handle.index_]], 1 );

	// Remove the object
	removeObject_internal( handle );
}


void DynamicSceneProvider::updateObject( const DynamicObjectHandle & handle, 
	const Matrix & worldXForm, 
	const AABB & collisionBB,
	const AABB & visibleBB )
{
	MF_ASSERT( dynamicObjectHandles_.isValid( handle ) );
	if (!dynamicObjectHandles_.isValid( handle ))
	{
		return;
	}

	updateObject_internal( handle, worldXForm, collisionBB, visibleBB );
}


class DynamicSceneProvider::Detail
{
public:
	static void clearTreeContents( OctreeContents& contents )
	{
		for (OctreeContents::iterator iter = contents.begin(); 
			iter != contents.end(); ++iter)
		{
			OctreeNodeContents& nodeContents = *iter;
			nodeContents.clear();
		}
	}

	static void insertObjectNode( DynamicObjectHandle handle, 
		BoundingBoxCollection& bbCollection,
		Octree& tree, OctreeContents& contents, Octree::NodeIndex *newIndex, 
		bool updateHeirachy )
	{
		Octree::NodeIndex nodeIndex = tree.insert( bbCollection[handle.index_] );
		if (nodeIndex != Octree::INVALID_NODE)
		{
			Octree::NodeDataReference dataRef = tree.dataOnLeaf(nodeIndex);
			contents[dataRef].push_back( handle );
			if (updateHeirachy)
			{
				tree.updateNodeBound( nodeIndex );
			}
		}

		*newIndex = nodeIndex;
	}

	static void removeNodeContents( OctreeNodeContents& contents,
		const DynamicObjectHandle& handle )
	{
		OctreeNodeContents::iterator findResult = 
			std::find( contents.begin(), contents.end(), handle );

		MF_ASSERT( findResult != contents.end() );

		std::swap( *findResult, contents.back() );
		contents.pop_back();
	}

	static void removeObjectNode( DynamicObjectHandle handle,
		Octree::NodeIndex oldIndex, Octree& tree, OctreeContents& contents )
	{
		if (oldIndex == Octree::INVALID_NODE)
		{
			return;
		}

		Octree::NodeDataReference dataRef = tree.dataOnLeaf(oldIndex);
		removeNodeContents( contents[dataRef], handle);
	}

	template <class SweepStartT, class SweepEndT>
	static bool collideReferences( 
		const DynamicSceneProvider & provider,
		const SweepStartT & source,
		const SweepEndT & extent,
		const SweepParams& sp,
		CollisionState & cs,
		Octree::RuntimeNodeDataReferences& nodeReferences)
	{
		const Scene* pScene = provider.scene();
		MF_ASSERT( pScene != NULL );

		const CollisionOperation* pOp =
			pScene->getObjectOperation<CollisionOperation>();
		MF_ASSERT( pOp );

		for (Octree::RuntimeNodeDataReferences::iterator iter = nodeReferences.begin();
			iter != nodeReferences.end(); ++iter)
		{
			Octree::NodeDataReference nodeRef = *iter;

			OctreeNodeContents contents = 
				provider.collisionTreeContents_[nodeRef];
			for (OctreeNodeContents::iterator itemIter = contents.begin();
				itemIter != contents.end(); ++itemIter)
			{
				const DynamicObjectHandle& objHandle = *itemIter;
				const SceneObject& object = 
					provider.objects_[ provider.objectLookup_[objHandle.index_]];

				if (object.isValid() &&
					pOp->collide( object, source, extent, sp, cs ))
				{
					return true;
				}
			}
		}

		return false;
	}
};


void DynamicSceneProvider::tick( float dTime )
{
	if (objects_.empty())
	{
		return;
	}

	Scene* pScene = this->scene();
	MF_ASSERT( pScene != NULL );

	TickOperation* pOp = pScene->getObjectOperation<TickOperation>();
	MF_ASSERT( pOp );
	pOp->tick( &objects_[0], objects_.size(), dTime );

#if ENABLE_SPACE_DEBUG_DRAW
	DebugDraw::draw( *this );
#endif
}


void DynamicSceneProvider::onPreTick()
{
	// Nothing to be done
}


void DynamicSceneProvider::onPostTick()
{
	if (!numDirtyObjects_)
	{
		return;
	}

	// Reset entire trees and reinsert everything
	collisionTree_.reinitialise();
	visibilityTree_.reinitialise();

	Detail::clearTreeContents( collisionTreeContents_ );
	Detail::clearTreeContents( visibilityTreeContents_ );

	// Allocate the scene object buffer for pushing to an update message
	BW::vector<SceneObject> updatedObjects;
	updatedObjects.reserve( numDirtyObjects_ );

	// Feed all the latest data into the core trees
	for (ObjectHandleCollection::iterator iter = objectHandles_.begin();
		iter != objectHandles_.end(); ++iter)
	{
		const DynamicObjectHandle& handle = *iter;

		if (objectDirtyFlags_[handle.index_])
		{
			objectDirtyFlags_[handle.index_] = false;

			// Copy the latest data
			objectCollisionBBs_[ handle.index_ ] =
				latestObjectCollisionBBs_[ handle.index_ ];
			objectVisibilityBBs_[ handle.index_ ] =
				latestObjectVisibilityBBs_[ handle.index_ ];
			objectWorldXform_[ handle.index_ ] =
				latestObjectWorldXform_[ handle.index_ ];

			updatedObjects.push_back( objects_[ objectLookup_[handle.index_]] );
		}
	}

	// Now insert all dynamic objects into these trees again and update their
	// tree info
	for (ObjectHandleCollection::iterator iter = objectHandles_.begin();
		iter != objectHandles_.end(); ++iter)
	{
		const DynamicObjectHandle& handle = *iter;

		DynamicObjectStorageInfo& treeInfo = objectStorageInfo_[handle.index_];
		Detail::insertObjectNode( handle, objectCollisionBBs_, collisionTree_, 
			collisionTreeContents_, &treeInfo.collisionNode_, false);
	}

	for (ObjectHandleCollection::iterator iter = objectHandles_.begin();
		iter != objectHandles_.end(); ++iter)
	{
		const DynamicObjectHandle& handle = *iter;

		DynamicObjectStorageInfo& treeInfo = objectStorageInfo_[handle.index_];
		Detail::insertObjectNode( handle, objectVisibilityBBs_, visibilityTree_, 
			visibilityTreeContents_, &treeInfo.visibilityNode_, false);
	}

	collisionTree_.updateHeirarchy();
	visibilityTree_.updateHeirarchy();

	// Now push the list of changed objects out to listeners
	// Note, the number of updated objects may not be the same as the number
	// reported, as some may have been removed since being updated.
	MF_ASSERT( updatedObjects.size() <= numDirtyObjects_ );
	pObjectChangeView_->notifyObjectsChanged( this,
		&updatedObjects.front(), updatedObjects.size() );
	numDirtyObjects_ = 0;
}


void DynamicSceneProvider::updateAnimations( float dTime )
{
	if (objects_.empty())
	{
		return;
	}

	Scene* pScene = this->scene();
	MF_ASSERT( pScene != NULL );

	TickOperation* pOp = pScene->getObjectOperation<TickOperation>();
	MF_ASSERT( pOp );
	pOp->updateAnimations( &objects_[0], objects_.size(), dTime );
}


bool DynamicSceneProvider::collide( const Vector3 & source,
	const Vector3 & extent,
	const SweepParams& sp,
	CollisionState & cs ) const
{
	Octree::RuntimeNodeDataReferences nodeReferences;
	collisionTree_.intersect( source, extent, nodeReferences );

	return Detail::collideReferences( *this, source, extent, sp, cs, 
		nodeReferences);
}


bool DynamicSceneProvider::collide( const WorldTriangle & source,
	const Vector3 & extent,
	const SweepParams& sp,
	CollisionState & cs ) const
{
	
	const AABB & shapeBB = sp.shapeBox();
	Octree::RuntimeNodeDataReferences nodeReferences;
	collisionTree_.intersect( shapeBB, nodeReferences );

	return Detail::collideReferences( *this, source, extent, sp, cs, 
		nodeReferences);
}


size_t DynamicSceneProvider::intersect( const SceneIntersectContext& context,
	const ConvexHull& hull, 
	IntersectionSet & intersection) const
{
	// Make sure these objects are contextually interesting.
	if (!context.includeDynamicObjects())
	{
		return 0;
	}

	Octree::RuntimeNodeDataReferences nodeReferences;
	visibilityTree_.intersect( hull, nodeReferences );

	size_t numIntersected = 0;
	for (Octree::RuntimeNodeDataReferences::iterator iter = nodeReferences.begin();
		iter != nodeReferences.end(); ++iter)
	{
		Octree::NodeDataReference nodeRef = *iter;

		OctreeNodeContents contents = 
			visibilityTreeContents_[nodeRef];
		for (OctreeNodeContents::iterator itemIter = contents.begin();
			itemIter != contents.end(); ++itemIter)
		{
			const DynamicObjectHandle& objHandle = *itemIter;
			const AABB & bb = objectVisibilityBBs_[objHandle.index_];

			if (bb.insideOut())
			{
				continue;
			}

			if (hull.intersects( bb ))
			{
				intersection.insert( 
					objects_[ objectLookup_[objHandle.index_]] );
				++numIntersected;
			}
		}
	}

	// Make sure we aren't returning more objects than actually exist in
	// the provider. Would indicate that something is wrong.
	MF_ASSERT( numIntersected <= objects_.size() );

	return numIntersected;
}


void DynamicSceneProvider::setPartitionSizeHint( float partitionSizeHint )
{
	partitionSizeHint_ = partitionSizeHint;
}


void DynamicSceneProvider::resizeScene( const AABB& sceneBB )
{
	PROFILER_SCOPED( DynamicSceneProvider_ResizeScene );

	const float DEFAULT_METERS_PER_NODE = 20.0f;
	const uint32 MAX_OCTREE_DEPTH = 8;
	float nodeSize = DEFAULT_METERS_PER_NODE;
	if (partitionSizeHint_ > 0.0f)
	{
		nodeSize = partitionSizeHint_;
	}

	// TODO: Take all the current members of the trees and resize them
	float maxDimension = std::max( sceneBB.width(), 
		std::max( sceneBB.height(), sceneBB.depth() ) );
	uint32 nodesPerEdge = static_cast<uint32>(maxDimension / nodeSize);
	uint32 maxDepth = log2ceil(nodesPerEdge);

	maxDepth = Math::clamp( 1u, maxDepth, MAX_OCTREE_DEPTH );

	OctreeContents oldCollisionContents;
	oldCollisionContents.storage().swap( collisionTreeContents_.storage() );
	OctreeContents oldVisibilityContents;
	oldVisibilityContents.storage().swap( visibilityTreeContents_.storage() );

	collisionTree_.reset();
	visibilityTree_.reset();

	collisionTree_.initialise( sceneBB.centre(), maxDimension, maxDepth);
	visibilityTree_.initialise( sceneBB.centre(), maxDimension, maxDepth);

	// Now insert all dynamic objects into these trees again and update their
	// tree info
	
	for (OctreeContents::iterator iter = oldCollisionContents.begin();
		iter != oldCollisionContents.end(); ++iter)
	{
		OctreeContents::ValueType& nodeContents = *iter;
		for (OctreeContents::ValueType::iterator objIter = nodeContents.begin();
			objIter != nodeContents.end(); ++objIter)
		{
			DynamicObjectHandle& handle = *objIter;

			DynamicObjectStorageInfo& treeInfo = objectStorageInfo_[handle.index_];
			Detail::insertObjectNode( handle, objectCollisionBBs_, collisionTree_, 
				collisionTreeContents_, &treeInfo.collisionNode_, false);
		}
	}
	collisionTree_.updateHeirarchy();

	for (OctreeContents::iterator iter = oldVisibilityContents.begin();
		iter != oldVisibilityContents.end(); ++iter)
	{
		OctreeContents::ValueType& nodeContents = *iter;
		for (OctreeContents::ValueType::iterator objIter = nodeContents.begin();
			objIter != nodeContents.end(); ++objIter)
		{
			DynamicObjectHandle& handle = *objIter;

			DynamicObjectStorageInfo& treeInfo = objectStorageInfo_[handle.index_];
			Detail::insertObjectNode( handle, objectVisibilityBBs_, visibilityTree_, 
				visibilityTreeContents_, &treeInfo.visibilityNode_, false);
		}
	}
	visibilityTree_.updateHeirarchy();
}


DynamicObjectHandle 
	DynamicSceneProvider::insertObject_internal( 
	const SceneObject& object,
	const Matrix & worldXform, 
	const AABB & collisionBB, 
	const AABB & visibilityBB )
{
	DynamicObjectHandle handle = dynamicObjectHandles_.create();

	objects_.push_back( object );
	objectHandles_.push_back( handle );

	objectLookup_[handle.index_] = objects_.size() - 1;
	objectCollisionBBs_[handle.index_] = collisionBB;
	objectVisibilityBBs_[handle.index_] = visibilityBB;
	objectWorldXform_[handle.index_] = worldXform;
	objectDirtyFlags_[handle.index_] = false;

	// Insert the object into the trees
	DynamicObjectStorageInfo treeInfo;
	Detail::insertObjectNode( handle, objectCollisionBBs_, collisionTree_, 
		collisionTreeContents_, &treeInfo.collisionNode_, true);
	Detail::insertObjectNode( handle, objectVisibilityBBs_, visibilityTree_, 
		visibilityTreeContents_, &treeInfo.visibilityNode_, true);
	
	objectStorageInfo_[handle.index_] = treeInfo;

	return handle;
}


void DynamicSceneProvider::updateObject_internal( 
	const DynamicObjectHandle & handle, 
	const Matrix & worldXForm, 
	const AABB & collisionBB,
	const AABB & visibleBB )
{
	MF_ASSERT( dynamicObjectHandles_.isValid( handle ) );

	// Update the object data
	latestObjectWorldXform_[handle.index_] = worldXForm;
	latestObjectCollisionBBs_[handle.index_] = collisionBB;
	latestObjectVisibilityBBs_[handle.index_] = visibleBB;

	if (!objectDirtyFlags_[handle.index_])
	{
		// Trigger a rebuild at the end of tick
		// and allow smart sizing of update buffers
		numDirtyObjects_++;
	}
	objectDirtyFlags_[handle.index_] = true;
}

void DynamicSceneProvider::removeObject_internal( 
	const DynamicObjectHandle & handle )
{
	MF_ASSERT( dynamicObjectHandles_.isValid( handle ) );

	// Remove the objects from the trees
	DynamicObjectStorageInfo& treeInfo = objectStorageInfo_[handle.index_];
	Detail::removeObjectNode( handle, treeInfo.collisionNode_,
		collisionTree_, collisionTreeContents_ );
	Detail::removeObjectNode( handle, treeInfo.visibilityNode_,
		visibilityTree_, visibilityTreeContents_ );

	// Leave the data in the slots behind since they'll be used for something
	// else soon.

	// Remove the object from the list of Objects though.
	// Swap the position of the old object with the outgoing one.
	size_t removedObjectIndex = objectLookup_[handle.index_];
	objects_[ removedObjectIndex ] = objects_.back();	
	DynamicObjectHandle movedObjectHandle = objectHandles_.back();
	objectHandles_[ removedObjectIndex ] = movedObjectHandle;

	objectLookup_[ movedObjectHandle.index_ ] = removedObjectIndex;
	objectLookup_[handle.index_] = size_t( ~0 );
	
	objects_.pop_back();
	objectHandles_.pop_back();
}


} // namespace BW
