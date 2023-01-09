#include "pch.hpp"
#include "static_scene_speed_tree.hpp"

#if SPEEDTREE_SUPPORT

#include "static_scene_provider.hpp"
#include "binary_format.hpp"
#include "string_table.hpp"
#include "collision_helpers.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/draw_operation.hpp"
#include "scene/collision_operation.hpp"
#include "scene/scene_draw_context.hpp"
#include "scene/spatial_query_operation.hpp"
#include "moo/texture_streaming_scene_view.hpp"
#include "moo/texture_usage.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"
#include "speedtree/speedtree_renderer.hpp"

#include "physics2/sweep_helpers.hpp"
#include "physics2/collision_obstacle.hpp"

#include "cstdmf/profiler.hpp"
#include "moo/draw_context.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
StaticSceneSpeedTreeHandler::StaticSceneSpeedTreeHandler() :
	reader_( NULL ),
	pStream_( NULL )
{
	// Disable culling of speedtrees, as compiled spaces take care
	// of all that.
	speedtree::TSpeedTreeType::s_enableCulling_ = false;
}


// ----------------------------------------------------------------------------
StaticSceneSpeedTreeHandler::~StaticSceneSpeedTreeHandler()
{

}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneSpeedTreeHandler::typeID() const
{
	return StaticSceneTypes::StaticObjectType::SPEED_TREE;
}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneSpeedTreeHandler::runtimeTypeID() 
	const
{
	return SceneObject::typeOf<Instance>();
}


// ----------------------------------------------------------------------------
bool StaticSceneSpeedTreeHandler::load( BinaryFormat& binFile,
							StaticSceneProvider& staticScene,
							const StringTable& strings,
							SceneObject* pLoadedObjects,
							size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneSpeedTreeHandler_load );

	reader_ = &binFile;

	size_t sectionIdx = binFile.findSection( 
		StaticSceneSpeedTreeTypes::FORMAT_MAGIC );
	if (sectionIdx == BinaryFormat::NOT_FOUND)
	{
		ASSET_MSG( "'%s' does not contain speedtree.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	if (binFile.sectionVersion( sectionIdx ) != 
		StaticSceneSpeedTreeTypes::FORMAT_VERSION)
	{
		ASSET_MSG( "'%s' has speedtree data with incompatible version.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	pStream_ = binFile.openSection( sectionIdx );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map SpeedTree section into memory.\n" );
		this->unload();
		return false;
	}

	if (!pStream_->read( treeData_ ))
	{
		ASSET_MSG( "SpeedTreeHandler: failed to read data.\n" );
		this->unload();
		return false;
	}

	this->loadTreeInstances( strings, pLoadedObjects, maxObjects );

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneSpeedTreeHandler::unload()
{
	this->freeTreeInstances();

	if (reader_ && pStream_)
	{
		reader_->closeSection( pStream_ );
	}

	pStream_ = NULL;
	reader_ = NULL;
	treeData_.reset();
}


// ----------------------------------------------------------------------------
void StaticSceneSpeedTreeHandler::loadTreeInstances( const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneSpeedTreeHandler_loadTreeInstances );

	size_t numTrees = treeData_.size();

	// If this is hit, there's a mismatch between static scene 
	// definition and stored trees.
	MF_ASSERT( maxObjects == numTrees );

	MF_ASSERT( instances_.empty() );
	instances_.resize( numTrees );

	SceneObject sceneObj;
	sceneObj.type<Instance>();
	sceneObj.flags().isDynamic( false );

	for (size_t i = 0; i < numTrees; ++i)
	{
		Instance& instance = instances_[i];
		instance.pDef_ = &treeData_[i];
		instance.pOwner_ = this;
		loadSpeedTree( treeData_[i], strings, instance );

		bool castShadow = instance.pDef_->flags_ &
			StaticSceneSpeedTreeTypes::FLAG_CASTS_SHADOW;
		sceneObj.flags().isCastingStaticShadow( castShadow );
		sceneObj.flags().isCastingDynamicShadow( castShadow );

		sceneObj.handle( &instance );
		pLoadedObjects[i] = sceneObj;
	}
}


// ----------------------------------------------------------------------------
void StaticSceneSpeedTreeHandler::freeTreeInstances()
{
	for (size_t i = 0; i < instances_.size(); ++i)
	{
		bw_safe_delete( instances_[i].pSpeedTree_ );
	}

	instances_.clear();
}


// ----------------------------------------------------------------------------
bool StaticSceneSpeedTreeHandler::loadSpeedTree( 
			StaticSceneSpeedTreeTypes::SpeedTree& def,
			const StringTable& strings,
			Instance& instance )
{
	// load the speedtree
	instance.pSpeedTree_ = new speedtree::SpeedTreeRenderer();
	instance.pSpeedTree_->load( strings.entry( def.resourceID_ ),
		def.seed_, def.worldTransform_ );

	instance.transformInverse_.invert( def.worldTransform_ );

	return true;
}


// ----------------------------------------------------------------------------
float StaticSceneSpeedTreeHandler::loadPercent() const
{
	return 1.f;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneSpeedTreeHandler::DrawHandler : public IDrawOperationTypeHandler
{
public:
	virtual void doDraw( const SceneDrawContext & context, 
		const SceneObject * pObjects, size_t count )
	{
		const Matrix& lodCameraView = context.lodCameraView();
		Matrix lodInvView;
		lodInvView.invert(lodCameraView);

		ClientSpacePtr cameraSpace = DeprecatedSpaceHelpers::cameraSpace();
		MF_ASSERT( cameraSpace != NULL );
		speedtree::SpeedTreeRenderer::beginFrame( &cameraSpace->enviro(),
			context.drawContext().renderingPassType(),
			lodInvView);

		for (size_t i = 0; i < count; ++i)
		{
			StaticSceneSpeedTreeHandler::Instance* pInstance =
				pObjects[i].getAs<StaticSceneSpeedTreeHandler::Instance>();
			MF_ASSERT( pInstance );
			MF_ASSERT( pInstance->pDef_ );

			if (!pInstance->pSpeedTree_)
			{
				continue;
			}

            pInstance->pSpeedTree_->draw( pInstance->pDef_->worldTransform_ );
		}

		speedtree::SpeedTreeRenderer::endFrame();
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneSpeedTreeHandler::CollisionHandler : public ICollisionTypeHandler
{
public:
	virtual bool doCollide( const SceneObject & object,
		const Vector3 & source, const Vector3 & extent,
		const SweepParams& sp, CollisionState & cs ) const
	{
		return this->doCollideInternal( object, source, extent, sp, cs );
	}

	virtual bool doCollide( const SceneObject& object,
		const WorldTriangle & source, const Vector3 & extent,
		const SweepParams & sp, CollisionState & cs ) const
	{
		return this->doCollideInternal( object, source, extent, sp, cs );
	}

private:
	template< typename T >
	bool doCollideInternal( const SceneObject& sceneObject,
		const T & source, const Vector3 & extent,
		const SweepParams & sp, CollisionState & cs ) const
	{	
		StaticSceneSpeedTreeHandler::Instance* pInstance =
			sceneObject.getAs<StaticSceneSpeedTreeHandler::Instance>();
		MF_ASSERT( pInstance );
		MF_ASSERT( pInstance->pDef_ );

		if (!pInstance->pSpeedTree_)
		{
			return false;
		}

		const BSPTree* pBSP = pInstance->pSpeedTree_->bsp();
		if (pBSP && this->doCollideInternal( sceneObject, pInstance, *pBSP,
			source, extent, sp, cs ))
		{
			return true;
		}

		return false;
	}

	template< typename T >
	bool doCollideInternal( 
		const SceneObject& sceneObject,
		StaticSceneSpeedTreeHandler::Instance* pInstance,
		const BSPTree & bsp,
		const T & source, const Vector3 & extent,
		const SweepParams & sp, CollisionState & cs ) const
	{
		CollisionObstacle ob( &pInstance->pDef_->worldTransform_,
			&pInstance->transformInverse_, sceneObject );

		BSPCollisionInterface collisionInterface( ob, bsp );

		return SweepHelpers::collideIntoObject<T>(
			source, sp, collisionInterface,
			bsp.boundingBox(), pInstance->transformInverse_,
			cs );
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneSpeedTreeHandler::SpatialQueryHandler : 
	public ISpatialQueryOperationTypeHandler
{
public:
	virtual void getWorldTransform( const SceneObject& obj, 
		Matrix* xform )
	{
		StaticSceneSpeedTreeHandler::Instance* pItem = 
			obj.getAs<StaticSceneSpeedTreeHandler::Instance>();
		MF_ASSERT( pItem );
		*xform = pItem->pDef_->worldTransform_;
	}

	virtual void getWorldVisibilityBoundingBox( 
		const SceneObject& obj, 
		AABB* bb )
	{
		StaticSceneSpeedTreeHandler::Instance* pItem = 
			obj.getAs<StaticSceneSpeedTreeHandler::Instance>();
		MF_ASSERT( pItem );
		BoundingBox modelBB = pItem->pSpeedTree_->boundingBox();
		modelBB.transformBy( pItem->pDef_->worldTransform_ );

		*bb = modelBB;
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneSpeedTreeHandler::TextureStreamingHandler : 
	public Moo::ITextureStreamingObjectOperationTypeHandler
{
public:

	virtual bool registerTextureUsage( const SceneObject& object,
		Moo::ModelTextureUsageGroup& usageGroup ) 
	{
		StaticSceneSpeedTreeHandler::Instance* pItem =
			object.getAs<StaticSceneSpeedTreeHandler::Instance>();
		MF_ASSERT( pItem );

		updateBounds( pItem, usageGroup );
		pItem->pSpeedTree_->generateTextureUsage( usageGroup );

		return true;
	}

	virtual bool updateTextureUsage( const SceneObject& object, 
		Moo::ModelTextureUsageGroup& usageGroup ) 
	{
		StaticSceneSpeedTreeHandler::Instance* pItem =
			object.getAs<StaticSceneSpeedTreeHandler::Instance>();
		MF_ASSERT( pItem );

		updateBounds( pItem, usageGroup );

		return true;
	}

	void updateBounds( StaticSceneSpeedTreeHandler::Instance* pItem,
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		BoundingBox modelBB = pItem->pSpeedTree_->boundingBox();
		modelBB.transformBy( pItem->pDef_->worldTransform_ );

		usageGroup.setWorldScale_FromTransform( pItem->pDef_->worldTransform_ );
		usageGroup.setWorldBoundSphere( Sphere( modelBB ) );
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//static
void StaticSceneSpeedTreeHandler::registerHandlers( Scene & scene )
{
	DrawOperation * pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<StaticSceneSpeedTreeHandler::Instance,
		StaticSceneSpeedTreeHandler::DrawHandler>();

	CollisionOperation * pColOp =
		scene.getObjectOperation<CollisionOperation>();
	MF_ASSERT( pColOp != NULL );
	pColOp->addHandler<StaticSceneSpeedTreeHandler::Instance,
		StaticSceneSpeedTreeHandler::CollisionHandler>();

	SpatialQueryOperation * pSpatOp = 
		scene.getObjectOperation<SpatialQueryOperation>();
	MF_ASSERT( pSpatOp != NULL );
	pSpatOp->addHandler<StaticSceneSpeedTreeHandler::Instance,
		StaticSceneSpeedTreeHandler::SpatialQueryHandler>();

	Moo::TextureStreamingObjectOperation * pTextureStreamingOp =
		scene.getObjectOperation<Moo::TextureStreamingObjectOperation>();
	pTextureStreamingOp->addHandler<StaticSceneSpeedTreeHandler::Instance,
		StaticSceneSpeedTreeHandler::TextureStreamingHandler>();
}

} // namespace CompiledSpace
} // namespace BW

#endif // SPEEDTREE_SUPPORT
