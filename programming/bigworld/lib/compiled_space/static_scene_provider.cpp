#include "pch.hpp"
#include "static_scene_provider.hpp"
#include "string_table.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/intersection_set.hpp"
#include "scene/collision_operation.hpp"
#include "scene/scene_intersect_context.hpp"
#include "scene/object_change_scene_view.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/profiler.hpp"
#include "math/boundbox.hpp"
#include "math/convex_hull.hpp"
#include "math/sphere.hpp"

#include "space/client_space.hpp"
#include "space/debug_draw.hpp"
#include "physics2/sweep_helpers.hpp"

namespace BW {
namespace CompiledSpace {

namespace 
{

bool verifyAllObjectsOfType( SceneTypeSystem::RuntimeTypeID type, 
	SceneObject* pObjects, size_t count )
{
	for (size_t i = 0; i < count; ++i)
	{
		if (pObjects->type() != type)
		{
			return false;
		}
	}
	return true;
}

} // End anonymous namespace

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
#if ENABLE_SPACE_DEBUG_DRAW
class StaticSceneProvider::DebugDraw
{
public:
	static void registerWatchers()
	{
		if (s_debug_initializedWatchers_)
		{
			return;
		}
		s_debug_initializedWatchers_ = true;

		MF_WATCH("Render/CompiledSpace/StaticScene_drawBBs", 
			s_debug_drawBBs_);
		MF_WATCH("Render/CompiledSpace/StaticScene_drawTree", 
			s_debug_drawTreeLevel_);
	}

	static void draw( StaticSceneProvider& provider )
	{
		if (s_debug_drawBBs_)
		{
			SpaceDebugDraw::drawBoundingBoxes( 
				&provider.sceneObjectBounds_[0], 
				provider.sceneObjectBounds_.size(), 
				BB_COLOUR );
		}
		if (s_debug_drawTreeLevel_)
		{
			SpaceDebugDraw::drawOctreeBoundingBoxes( 
				provider.sceneOctree_, 
				s_debug_drawTreeLevel_ - 1, TREE_BB_COLOUR );
		}
	}

private:
	static bool s_debug_initializedWatchers_;
	static const uint32 BB_COLOUR = 0xFFFF00FF;
	static const uint32 TREE_BB_COLOUR = 0xFFFF88FF;

	static void drawOctree( Octree& tree, size_t level, uint32 colour )
	{
		SpaceDebugDraw::drawOctreeBoundingBoxes( tree, level, colour );
	}

	static bool s_debug_drawBBs_;
	static size_t s_debug_drawTreeLevel_;
};

bool StaticSceneProvider::DebugDraw::s_debug_initializedWatchers_ = false;
bool StaticSceneProvider::DebugDraw::s_debug_drawBBs_ = false;
size_t StaticSceneProvider::DebugDraw::s_debug_drawTreeLevel_ = 0;

#endif

// ----------------------------------------------------------------------------
//static
void StaticSceneProvider::registerHandlers( Scene & scene )
{
	StaticSceneModelHandler::registerHandlers( scene );

#if SPEEDTREE_SUPPORT
	StaticSceneSpeedTreeHandler::registerHandlers( scene );
#endif

	StaticSceneWaterHandler::registerHandlers( scene );
	StaticSceneFlareHandler::registerHandlers( scene );
	StaticSceneDecalHandler::registerHandlers( scene );
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticSceneProvider::StaticSceneProvider( FourCC formatMagic ) :
	formatMagic_( formatMagic ),
	pReader_( NULL ),
	pStream_( NULL )
{
#if ENABLE_SPACE_DEBUG_DRAW
	DebugDraw::registerWatchers();
#endif
}


// ----------------------------------------------------------------------------
StaticSceneProvider::~StaticSceneProvider()
{

}


// ----------------------------------------------------------------------------
bool StaticSceneProvider::doLoadFromSpace( ClientSpace * pSpace,
								 BinaryFormat& reader, 
								 const DataSectionPtr& pSpaceSettings,
								 const Matrix& transform, 
								 const StringTable& strings )
{
	PROFILER_SCOPED( StaticSceneProvider_doLoadFromSpace );

	typedef StaticSceneTypes::TypeHeader TypeHeader;

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection(
		formatMagic_,
		StaticSceneTypes::FORMAT_VERSION,
		"StaticSceneProvider" );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map '%s' into memory.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}
	
	// Load the scene spatial data
	if (!pStream_->read( sceneOctree_ ) || 
		!pStream_->read( sceneOctreeNodeContents_.storage() ) ||
		!pStream_->read( sceneOctreeContents_) ||
		!pStream_->read( sceneObjectBounds_ ) ||
		!pStream_->read( sceneObjectSphereBounds_ ))
	{
		ASSET_MSG( "StaticSceneProvider in '%s' has incomplete spatial scene data.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	if (!pStream_->read( typeHeaders_ ))
	{
		ASSET_MSG( "StaticSceneProvider in '%s' has incomplete data.\n",
			pReader_->resourceID() );
		this->unload();
		return false;
	}

	// Preallocate object instance array
	size_t numObjectsTotal = 0;
	for (size_t i = 0; i < typeHeaders_.size(); ++i)
	{
		numObjectsTotal += typeHeaders_[i].numObjects_;
	}

	objects_.resize( numObjectsTotal );

	// Load each type
	size_t typeStartIndex = 0;
	for (size_t i = 0; i < typeHeaders_.size(); ++i)
	{
		SceneTypeSystem::RuntimeTypeID typeID =
			typeHeaders_[i].typeID_;

		if (!typeHandlers_.exists( typeID ))
		{
			ASSET_MSG( "StaticScene binary contains unknown "
				"object typeID %d.\n", typeID );
			continue;
		}

		IStaticSceneTypeHandler* pHandler =
			typeHandlers_[typeID];
		MF_ASSERT( pHandler != NULL );

		uint32 numObjectsExpected = typeHeaders_[i].numObjects_;
		SceneObject* pObjectBufferBegin = NULL;
		if (numObjectsExpected != 0)
		{
			pObjectBufferBegin = &objects_[typeStartIndex];
		}

		if (!pHandler->load( *pReader_, *this, strings,
				pObjectBufferBegin, numObjectsExpected))
		{
			ASSET_MSG( "StaticScene: failed to load "
				"objects with typeID %d.\n", typeID );
		}

		MF_ASSERT( verifyAllObjectsOfType( pHandler->runtimeTypeID(),
			pObjectBufferBegin, numObjectsExpected) );

		typeStartIndex += typeHeaders_[i].numObjects_;
	}

	return true;
}


// ----------------------------------------------------------------------------
bool StaticSceneProvider::doBind()
{
	if (isValid())
	{
		space().scene().addProvider( this );
	}

	bool result = true;
	for (size_t i = 0; i < typeHeaders_.size(); ++i)
	{
		SceneTypeSystem::RuntimeTypeID typeID =
			typeHeaders_[i].typeID_;

		if (typeHandlers_.exists( typeID ))
		{
			IStaticSceneTypeHandler* pHandler =
				typeHandlers_[typeID];
			MF_ASSERT( pHandler != NULL );

			result &= pHandler->bind();
		}
	}

	if (objects_.size())
	{
		ObjectChangeSceneView* pChangeView = 
			space().scene().getView<ObjectChangeSceneView>();
		pChangeView->notifyObjectsAdded( this, &objects_.front(), objects_.size() );
	}

	return result;
}


// ----------------------------------------------------------------------------
void StaticSceneProvider::doUnload()
{
	if (objects_.size())
	{
		ObjectChangeSceneView* pChangeView = 
			space().scene().getView<ObjectChangeSceneView>();
		pChangeView->notifyObjectsRemoved( this, &objects_.front(), objects_.size() );
	}
	
	space().scene().removeProvider( this );

	for (size_t i = 0; i < typeHeaders_.size(); ++i)
	{
		SceneTypeSystem::RuntimeTypeID typeID =
			typeHeaders_[i].typeID_;

		if (typeHandlers_.exists( typeID ))
		{
			IStaticSceneTypeHandler* pHandler =
				typeHandlers_[typeID];
			MF_ASSERT( pHandler != NULL );

			pHandler->unload();
		}
	}

	typeHeaders_.reset();

	if (pStream_ && pReader_)
	{
		pReader_->closeSection( pStream_ );
	}

	pStream_ = NULL;
	pReader_ = NULL;
}


// ----------------------------------------------------------------------------
bool StaticSceneProvider::isValid() const
{
	return typeHeaders_.valid();
}


// ----------------------------------------------------------------------------
float StaticSceneProvider::percentLoaded() const
{
	return 1.f;
}


// ----------------------------------------------------------------------------
void * StaticSceneProvider::getView(
	const SceneTypeSystem::RuntimeTypeID & viewTypeID )
{
	void * result = NULL;

	exposeView<ITickSceneProvider>(this, viewTypeID, result);
	exposeView<ICollisionSceneViewProvider>(this, viewTypeID, result);
	exposeView<IIntersectSceneViewProvider>(this, viewTypeID, result);

	return result;
}


// ----------------------------------------------------------------------------
void StaticSceneProvider::tick( float dTime )
{
	if (objects_.empty())
	{
		return;
	}

	Scene* pScene = this->scene();
	MF_ASSERT( pScene != NULL );

	TickOperation* pOp = pScene->getObjectOperation<TickOperation>();
	MF_ASSERT( pOp );

	size_t offset = 0;
	for (size_t typeIdx = 0; typeIdx < typeHeaders_.size(); ++typeIdx)
	{
		size_t numObjects = typeHeaders_[typeIdx].numObjects_;
		if (numObjects == 0)
		{
			continue;
		}
		
		MF_ASSERT( offset + numObjects <= objects_.size() );
		SceneObject* pObjects = &objects_[offset];

		pOp->tick( pObjects[0].type(),
			pObjects, numObjects, dTime );

		offset += numObjects;
	}

#if ENABLE_SPACE_DEBUG_DRAW
	DebugDraw::draw( *this );
#endif
}


// ----------------------------------------------------------------------------
void StaticSceneProvider::updateAnimations( float dTime )
{
	if (objects_.empty())
	{
		return;
	}

	Scene* pScene = this->scene();
	MF_ASSERT( pScene != NULL );

	TickOperation* pOp = pScene->getObjectOperation<TickOperation>();
	MF_ASSERT( pOp );

	size_t offset = 0;
	for (size_t typeIdx = 0; typeIdx < typeHeaders_.size(); ++typeIdx)
	{
		size_t numObjects = typeHeaders_[typeIdx].numObjects_;
		if (numObjects == 0)
		{
			continue;
		}

		MF_ASSERT( offset+numObjects <= objects_.size() );
		SceneObject* pObjects = &objects_[offset];

		pOp->updateAnimations( pObjects[0].type(),
			pObjects, numObjects, dTime );

		offset += numObjects;
	}
}


// ----------------------------------------------------------------------------
size_t StaticSceneProvider::intersect( const SceneIntersectContext& context,
	const ConvexHull& hull, 
	IntersectionSet & intersection ) const
{
	// Make sure these objects are contextually interesting.
	if (!context.includeStaticObjects())
	{
		return 0;
	}

	Octree::RuntimeNodeDataReferences nodeReferences;
	sceneOctree_.intersect( hull, nodeReferences );

	size_t numIntersected = 0;
	for (Octree::RuntimeNodeDataReferences::iterator iter = nodeReferences.begin();
		iter != nodeReferences.end(); ++iter)
	{
		Octree::NodeDataReference nodeRef = *iter;

		const DataSpan & span = sceneOctreeNodeContents_[nodeRef];
		for (size_t i = span.first(); i <= span.last(); ++i)
		{
			size_t sceneObjectIndex = sceneOctreeContents_[i];

			// Do a simple test against a bound sphere instead of expensive BB
			const Sphere & sphere = sceneObjectSphereBounds_[ sceneObjectIndex ];
			if (hull.intersects( sphere.center(), sphere.radius() ))
			{
				intersection.insert( objects_[sceneObjectIndex] );
				++numIntersected;
			}
		}
	}

	return numIntersected;
}


// ----------------------------------------------------------------------------
class StaticSceneProvider::Detail
{
public:
	template <class SweepStartT, class SweepEndT>
	static bool collideReferences( 
		const StaticSceneProvider & provider,
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

			const DataSpan & span = provider.sceneOctreeNodeContents_[nodeRef];
			for (size_t i = span.first(); i <= span.last(); ++i)
			{
				size_t sceneObjectIndex = provider.sceneOctreeContents_[i];
				const SceneObject& object = provider.objects_[sceneObjectIndex];

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


// ----------------------------------------------------------------------------
bool StaticSceneProvider::collide( const Vector3 & source,
								  const Vector3 & extent,
								  const SweepParams& sp,
								  CollisionState & cs ) const
{
	if (objects_.empty())
	{
		return false;
	}

	Octree::RuntimeNodeDataReferences nodeReferences;
	sceneOctree_.intersect( source, extent, nodeReferences );

	return Detail::collideReferences( *this, source, extent, sp, cs, 
		nodeReferences);
}


// ----------------------------------------------------------------------------
bool StaticSceneProvider::collide( const WorldTriangle & source,
								  const Vector3 & extent,
								  const SweepParams& sp,
								  CollisionState & cs ) const
{
	if (objects_.empty())
	{
		return false;
	}

	const AABB & shapeBB = sp.shapeBox();
	Octree::RuntimeNodeDataReferences nodeReferences;
	sceneOctree_.intersect( shapeBB, nodeReferences );

	return Detail::collideReferences( *this, source, extent, sp, cs, 
		nodeReferences);
}


// ----------------------------------------------------------------------------
void StaticSceneProvider::addTypeHandler( IStaticSceneTypeHandler* pHandler )
{
	MF_ASSERT( pHandler != NULL );
	typeHandlers_[pHandler->typeID()] = pHandler;
}


// ----------------------------------------------------------------------------
void StaticSceneProvider::removeTypeHandler( IStaticSceneTypeHandler* pHandler )
{
	MF_ASSERT( pHandler != NULL );
	if (typeHandlers_.exists( pHandler->typeID() ) && 
		typeHandlers_[pHandler->typeID()] == pHandler)
	{
		typeHandlers_[pHandler->typeID()] = NULL;
	}
}


// ----------------------------------------------------------------------------
const SceneObject& StaticSceneProvider::getObject( size_t idx ) const
{
	return objects_[idx];
}

} // namespace CompiledSpace
} // namespace BW
