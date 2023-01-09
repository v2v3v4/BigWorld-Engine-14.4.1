#include "pch.hpp"

#include "static_scene_model.hpp"
#include "static_scene_provider.hpp"
#include "binary_format.hpp"
#include "string_table.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/tick_scene_view.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/draw_operation.hpp"
#include "scene/collision_operation.hpp"
#include "scene/scene_draw_context.hpp"
#include "scene/spatial_query_operation.hpp"

#include "model/super_model.hpp"
#include "model/model_animation.hpp"
#include "model/super_model_animation.hpp"
#include "model/super_model_dye.hpp"

#include "moo/texture_streaming_scene_view.hpp"
#include "moo/texture_usage.hpp"

#include "physics2/collision_obstacle.hpp"
#include "physics2/collision_interface.hpp"
#include "physics2/sweep_helpers.hpp"

#include "collision_helpers.hpp"

#include "cstdmf/profiler.hpp"

namespace BW {
namespace CompiledSpace {


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticSceneModelHandler::StaticSceneModelHandler() :
	reader_( NULL ),
	pStream_( NULL )
{
	
}


// ----------------------------------------------------------------------------
StaticSceneModelHandler::~StaticSceneModelHandler()
{

}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneModelHandler::typeID() const
{
	return StaticSceneTypes::StaticObjectType::STATIC_MODEL;
}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneModelHandler::runtimeTypeID() 
	const
{
	return SceneObject::typeOf<Instance>();
}


// ----------------------------------------------------------------------------
bool StaticSceneModelHandler::load( BinaryFormat& binFile,
							  StaticSceneProvider& staticScene,
							  const StringTable& strings,
							  SceneObject* pLoadedObjects,
							  size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneModelHandler_load );

	reader_ = &binFile;

	size_t sectionIdx = binFile.findSection( 
		StaticSceneModelTypes::FORMAT_MAGIC );
	if (sectionIdx == BinaryFormat::NOT_FOUND)
	{
		ASSET_MSG( "'%s' does not contain static model.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	if (binFile.sectionVersion( sectionIdx ) != 
		StaticSceneModelTypes::FORMAT_VERSION)
	{
		ASSET_MSG( "'%s' has static model data with incompatible version.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	pStream_ = binFile.openSection( sectionIdx, true );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map StaticModel section into memory.\n" );
		this->unload();
		return false;
	}

	if (!pStream_->read( modelData_ ) || 
		!pStream_->read( resourceRefs_ ))
	{
		ASSET_MSG( "StaticModelHandler: failed to read data.\n" );
		this->unload();
		return false;
	}

	this->loadModelInstances( strings, pLoadedObjects, maxObjects );

	return true;
}


// ----------------------------------------------------------------------------
bool StaticSceneModelHandler::bind()
{
	for (size_t i = 0; i < this->numModels(); ++i)
	{
		Instance& instance = instances_[i];
		
		if (!instance.pSuperModel_)
		{
			continue;
		}

		if (!instance.pAnimation_)
		{
			// only need to do this once
			// must be called from main thread).
			instance.pSuperModel_->cacheDefaultTransforms(
				instance.pDef_->worldTransform_ );
		}	
	}

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneModelHandler::unload()
{
	this->freeModelInstances();

	if (reader_ && pStream_)
	{
		reader_->closeSection( pStream_ );
	}

	pStream_ = NULL;
	reader_ = NULL;
	resourceRefs_.reset();
	modelData_.reset();
}


// ----------------------------------------------------------------------------
size_t StaticSceneModelHandler::numModels() const
{
	return modelData_.size();
}


// ----------------------------------------------------------------------------
void StaticSceneModelHandler::loadModelInstances( const StringTable& strings,
		SceneObject* pLoadedObjects, size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneModelHandler_loadModelInstances );

	// If this is hit, there's a mismatch between static scene 
	// definition and stored models.
	MF_ASSERT( maxObjects == 0 || pLoadedObjects )
	MF_ASSERT( maxObjects == modelData_.size() );

	MF_ASSERT( instances_.empty() );
	instances_.resize( this->numModels() );

	SceneObject sceneObj;
	sceneObj.type<Instance>();
	sceneObj.flags().isDynamic( false );
	
	for (size_t i = 0; i < this->numModels(); ++i)
	{
		Instance& instance = instances_[i];
		instance.pDef_ = &modelData_[i];

		instance.pOwner_ = this;

		loadSuperModel( modelData_[i], strings, instance );

		bool isCastingShadow = instance.pDef_->flags_ & 
			StaticSceneModelTypes::FLAG_CASTS_SHADOW;

		sceneObj.flags().isCastingStaticShadow( isCastingShadow );
		sceneObj.flags().isCastingDynamicShadow( isCastingShadow );

		sceneObj.handle( &instance );
		pLoadedObjects[i] = sceneObj;
	}
}


// ----------------------------------------------------------------------------
void StaticSceneModelHandler::freeModelInstances()
{
	for (size_t i = 0; i < instances_.size(); ++i)
	{
		instances_[i].pAnimation_ = NULL;
		bw_safe_delete( instances_[i].pSuperModel_ );
	}

	instances_.clear();
}


// ----------------------------------------------------------------------------
bool StaticSceneModelHandler::loadSuperModel( 
			StaticSceneModelTypes::Model& def,
			const StringTable& strings,
			Instance& instance )
{
	// TODO: fix up super model to allow us not to use this vector of strings.
	BW::vector<BW::string> modelIds;
	
	for (uint16 i = def.resourceIDs_.first_;
		i <= def.resourceIDs_.last_; ++i)
	{
		if (i != uint16(-1))
		{
			modelIds.push_back( strings.entry(resourceRefs_[i]) );
		}
	}

	instance.pSuperModel_ = new SuperModel( modelIds );

	// Turn off boundbox checking in drawing as this system will take
	// care of all that.
	instance.pSuperModel_->checkBB( false );
	
	instance.pSuperModel_->beginRead();

	const bool bLoadSuccess = instance.pSuperModel_->nModels() > 0;
	if (bLoadSuccess)
	{
		// Animation
		const char* animationName = strings.entry( def.animationName_ );
		if (animationName)
		{
			instance.pAnimation_ =
				instance.pSuperModel_->getAnimation( animationName );

			instance.pAnimation_->time = 0.f;
			instance.pAnimation_->blendRatio = 1.0f;

			if (instance.pAnimation_->pSource(
				*instance.pSuperModel_ ) == NULL)
			{
				ASSET_MSG( "SuperModel can't find its animation %s\n",
					animationName );
				instance.pAnimation_ = NULL;
			}
		}

		// Dyes
		uint16 count = (def.tintDyes_.last_ - def.tintDyes_.first_) + 1;
		if (count > 0 && count % 2 == 0)
		{
			size_t numDyes = count / 2;
			instance.materialFashions_.reserve(
				instance.materialFashions_.size() + numDyes );

			for (uint16 i = def.tintDyes_.first_;
				i < def.tintDyes_.last_; i += 2)
			{
				const char* dye = strings.entry( resourceRefs_[i] );
				const char* tint = strings.entry( resourceRefs_[i+1] );
				if (dye && tint)
				{
					SuperModelDyePtr pDye =
						instance.pSuperModel_->getDye( dye, tint );
					if (pDye)
					{
						instance.materialFashions_.push_back( pDye );
					}
					else
					{
						ASSET_MSG( "SuperModel cannot find dye->tint mapping: "
							"%s->%s\n", dye, tint );
					}
				}
			}
		}
	}

	instance.pSuperModel_->endRead();
	
	if (!bLoadSuccess)
	{
		WARNING_MSG( "StaticModelHandler: No models loaded into SuperModel\n" );
		bw_safe_delete( instance.pSuperModel_ );
		return false;
	}
	else
	{
		instance.transformInverse_.invert( def.worldTransform_ );
		return true;
	}
}


// ----------------------------------------------------------------------------
float StaticSceneModelHandler::loadPercent() const
{
	return 1.f;
}


// ----------------------------------------------------------------------------
void StaticSceneModelHandler::Instance::setSuperModel( SuperModel* pModel )
{
	pSuperModel_ = pModel;

	// Trigger a cache of the default transforms
	setTransform( pDef_->worldTransform_ );
}


// ----------------------------------------------------------------------------
void StaticSceneModelHandler::Instance::setTransform( const Matrix& transform )
{
	// TODO: should we really modify the static def transform?
	pDef_->worldTransform_ = transform;
	transformInverse_.invert( transform );

	if (pSuperModel_)
	{
		pSuperModel_->cacheDefaultTransforms(
			pDef_->worldTransform_ );

		pSuperModel_->updateLodsOnly(
			pDef_->worldTransform_,
			Model::LOD_AUTO_CALCULATE );
	}
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneModelHandler::TickHandler : public ITickOperationTypeHandler
{
public:
	virtual void doTick( SceneObject* pObjects,
		size_t objectCount, float dTime )
	{
	}

	virtual void doUpdateAnimations( SceneObject* pObjects,
		size_t objectCount, float dTime )
	{
		for (size_t i = 0; i < objectCount; ++i)
		{
			StaticSceneModelHandler::Instance* pInstance =
				pObjects[i].getAs<StaticSceneModelHandler::Instance>();
			MF_ASSERT( pInstance );
			MF_ASSERT( pInstance->pDef_ );

			if (!pInstance->pSuperModel_ || !pInstance->pAnimation_)
			{
				continue;
			}

			ModelAnimation* pModelAnim =
				pInstance->pAnimation_->pSource( *pInstance->pSuperModel_ );
			if (pModelAnim)
			{
				// Update animation time
				pInstance->pAnimation_->time += dTime
					* pInstance->pDef_->animationMultiplier_;

				float duration = pModelAnim->duration_;
				pInstance->pAnimation_->time -=
					int64( pInstance->pAnimation_->time / duration ) * duration;
			}


			TmpTransforms preTransformFashions;
			preTransformFashions.push_back( pInstance->pAnimation_.get() );

			pInstance->pSuperModel_->updateAnimations(
				pInstance->pDef_->worldTransform_,
				&preTransformFashions, // preTransformFashions
				NULL, // pPostFashions
				Model::LOD_AUTO_CALCULATE ); // atLod
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneModelHandler::DrawHandler : public IDrawOperationTypeHandler
{
public:
	virtual void doDraw( const SceneDrawContext & context,
		const SceneObject * pObjects, size_t count )
	{
		for (size_t i = 0; i < count; ++i)
		{
			StaticSceneModelHandler::Instance* pInstance =
				pObjects[i].getAs<StaticSceneModelHandler::Instance>();
			MF_ASSERT( pInstance );
			MF_ASSERT( pInstance->pDef_ );

			if (!pInstance->pSuperModel_)
			{
				continue;
			}

			if (!pInstance->pAnimation_)
			{
				pInstance->pSuperModel_->updateLodsOnly(
					pInstance->pDef_->worldTransform_,
					Model::LOD_AUTO_CALCULATE );
			}

			pInstance->pSuperModel_->draw( context.drawContext(),
				pInstance->pDef_->worldTransform_,
				&pInstance->materialFashions_ );
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneModelHandler::CollisionHandler : public ICollisionTypeHandler
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
		StaticSceneModelHandler::Instance* pInstance =
			sceneObject.getAs<StaticSceneModelHandler::Instance>();
		MF_ASSERT( pInstance );
		MF_ASSERT( pInstance->pDef_ );

		if (!pInstance->pSuperModel_)
		{
			return false;
		}
		
		int numModels = pInstance->pSuperModel_->nModels();
		for (int i = 0; i < numModels; i++)
		{
			const ModelPtr & pModel =
				pInstance->pSuperModel_->topModel( i );
			if (!pModel)
			{
				continue;
			}

			if (this->doCollideInternal( sceneObject, pInstance, pModel,
				source, extent, sp, cs ))
			{
				return true;
			}
		}

		return false;
	}

	template< typename T >
	bool doCollideInternal( 
		const SceneObject& sceneObject,
		StaticSceneModelHandler::Instance* pInstance, const ModelPtr & pModel,
		const T & source, const Vector3 & extent,
		const SweepParams & sp, CollisionState & cs ) const
	{
		const BSPTree * pBSPTree = pModel->decompose();
		if (!pBSPTree)
		{
			return false;
		}

		CollisionObstacle ob( &pInstance->pDef_->worldTransform_,
			&pInstance->transformInverse_, sceneObject );

		BSPCollisionInterface collisionInterface( ob, *pBSPTree );

		return SweepHelpers::collideIntoObject<T>(
			source, sp, collisionInterface,
			pBSPTree->boundingBox(), pInstance->transformInverse_,
			cs );
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneModelHandler::SpatialQueryHandler : 
	public ISpatialQueryOperationTypeHandler
{
public:
	virtual void getWorldTransform( const SceneObject& obj, 
		Matrix* xform )
	{
		StaticSceneModelHandler::Instance* pItem = 
			obj.getAs<StaticSceneModelHandler::Instance>();
		MF_ASSERT( pItem );
		*xform = pItem->pDef_->worldTransform_;
	}

	virtual void getWorldVisibilityBoundingBox(
		const SceneObject& obj, 
		AABB* bb )
	{
		StaticSceneModelHandler::Instance* pItem = 
			obj.getAs<StaticSceneModelHandler::Instance>();
		MF_ASSERT( pItem );

		BoundingBox modelBB;
		pItem->pSuperModel_->localVisibilityBox( modelBB );
		modelBB.transformBy( pItem->pDef_->worldTransform_ );
		
		*bb = modelBB;
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneModelHandler::TextureStreamingHandler : 
	public Moo::ITextureStreamingObjectOperationTypeHandler
{
public:
	
	virtual bool registerTextureUsage( const SceneObject& object,
		Moo::ModelTextureUsageGroup& usageGroup ) 
	{
		StaticSceneModelHandler::Instance* pItem =
			object.getAs<StaticSceneModelHandler::Instance>();
		MF_ASSERT( pItem );

		updateBounds( pItem, usageGroup );
		pItem->pSuperModel_->generateTextureUsage( usageGroup );

		return true;
	}

	virtual bool updateTextureUsage( const SceneObject& object, 
		Moo::ModelTextureUsageGroup& usageGroup ) 
	{
		StaticSceneModelHandler::Instance* pItem =
			object.getAs<StaticSceneModelHandler::Instance>();
		MF_ASSERT( pItem );

		updateBounds( pItem, usageGroup );

		return true;
	}

	void updateBounds( StaticSceneModelHandler::Instance* pItem,
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		BoundingBox modelBB;
		pItem->pSuperModel_->localVisibilityBox( modelBB );
		modelBB.transformBy( pItem->pDef_->worldTransform_ );

		usageGroup.setWorldScale_FromTransform( pItem->pDef_->worldTransform_ );
		usageGroup.setWorldBoundSphere( Sphere( modelBB ) );
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//static
void StaticSceneModelHandler::registerHandlers( Scene & scene )
{
	TickOperation* pOp = scene.getObjectOperation<TickOperation>();
	MF_ASSERT( pOp != NULL );
	pOp->addHandler<StaticSceneModelHandler::Instance,
		StaticSceneModelHandler::TickHandler>();

	DrawOperation * pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<StaticSceneModelHandler::Instance,
		StaticSceneModelHandler::DrawHandler>();

	CollisionOperation * pColOp =
		scene.getObjectOperation<CollisionOperation>();
	MF_ASSERT( pColOp != NULL );
	pColOp->addHandler<StaticSceneModelHandler::Instance,
		StaticSceneModelHandler::CollisionHandler>();

	SpatialQueryOperation * pSpatOp = 
		scene.getObjectOperation<SpatialQueryOperation>();
	MF_ASSERT( pSpatOp != NULL );
	pSpatOp->addHandler<StaticSceneModelHandler::Instance,
		StaticSceneModelHandler::SpatialQueryHandler>();

	Moo::TextureStreamingObjectOperation * pTextureStreamingOp =
		scene.getObjectOperation<Moo::TextureStreamingObjectOperation>();
	pTextureStreamingOp->addHandler<StaticSceneModelHandler::Instance,
		StaticSceneModelHandler::TextureStreamingHandler>();
}


} // namespace CompiledSpace
} // namespace BW
