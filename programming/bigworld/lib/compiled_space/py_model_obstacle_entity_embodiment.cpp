#include "pch.hpp"

#include "compiled_space.hpp"
#include "py_model_obstacle_entity_embodiment.hpp"
#include "collision_helpers.hpp"

#include "moo/lod_settings.hpp"
#include "scene/scene.hpp"
#include "scene/tick_scene_view.hpp"
#include "scene/draw_operation.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/collision_operation.hpp"
#include "scene/scene_draw_context.hpp"
#include "model/model.hpp"
#include "model/super_model.hpp"
#include "moo/texture_streaming_scene_view.hpp"
#include "moo/texture_usage.hpp"
#include "physics2/sweep_helpers.hpp"


namespace BW {
namespace CompiledSpace {


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class PyModelObstacleEntityEmbodiment::TickHandler :
	public ITickOperationTypeHandler
{
public:
	virtual void doTick(
		SceneObject* pObjects,
		size_t objectCount, float dTime )
	{
		for (size_t i = 0; i < objectCount; ++i)
		{
			SceneObject& obj = pObjects[i];
			MF_ASSERT( obj.isType<PyModelObstacleEntityEmbodiment>() );

			PyModelObstacleEntityEmbodiment* pObstacle =
				obj.getAs<PyModelObstacleEntityEmbodiment>();
			MF_ASSERT( pObstacle != NULL );

			pObstacle->doTick( dTime );
		}
	}

	virtual void doUpdateAnimations( 
		SceneObject* pObjects, 
		size_t objectCount, float dTime )
	{
		for (size_t i = 0; i < objectCount; ++i)
		{
			SceneObject& obj = pObjects[i];
			MF_ASSERT( obj.isType<PyModelObstacleEntityEmbodiment>() );

			PyModelObstacleEntityEmbodiment* pObstacle =
				obj.getAs<PyModelObstacleEntityEmbodiment>();
			MF_ASSERT( pObstacle != NULL );

			pObstacle->doUpdateAnimations( dTime );
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class PyModelObstacleEntityEmbodiment::DrawHandler :
	public IDrawOperationTypeHandler
{
public:
	void doDraw( const SceneDrawContext & context, 
		const SceneObject * pObjects, size_t count )
	{
		for (size_t i = 0; i < count; ++i)
		{
			using namespace CompiledSpace;
			PyModelObstacleEntityEmbodiment* pInst =
				pObjects[i].getAs<PyModelObstacleEntityEmbodiment>();

			pInst->doDraw( context.drawContext() );
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class PyModelObstacleEntityEmbodiment::CollisionHandler :
	public ICollisionTypeHandler
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
		PyModelObstacleEntityEmbodiment* pInstance =
			sceneObject.getAs<PyModelObstacleEntityEmbodiment>();
		MF_ASSERT( pInstance != NULL );

		SuperModel* pSuperModel = pInstance->pObstacle_->superModel();
		if (!pSuperModel)
		{
			return false;
		}

		int numModels = pSuperModel->nModels();
		for (int i = 0; i < numModels; i++)
		{
			const ModelPtr & pModel =
				pSuperModel->topModel( i );
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
		PyModelObstacleEntityEmbodiment* pInstance,
		const ModelPtr & pModel,
		const T & source, const Vector3 & extent,
		const SweepParams & sp, CollisionState & cs ) const
	{
		const BSPTree * pBSPTree = pModel->decompose();
		if (!pBSPTree)
		{
			return false;
		}

		CollisionObstacle ob( &pInstance->worldTransform_,
			&pInstance->inverseWorldTransform_, sceneObject );

		BSPCollisionInterface collisionInterface( ob, *pBSPTree );

		return SweepHelpers::collideIntoObject<T>(
			source, sp, collisionInterface,
			pBSPTree->boundingBox(), 
			pInstance->inverseWorldTransform_,
			cs );
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class PyModelObstacleEntityEmbodiment::TextureStreamingHandler : 
	public Moo::ITextureStreamingObjectOperationTypeHandler
{
public:
	virtual bool registerTextureUsage( 
		const SceneObject& object,
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		using namespace CompiledSpace;
		PyModelObstacleEntityEmbodiment* pInst =
			object.getAs<PyModelObstacleEntityEmbodiment>();

		updateBounds( pInst, usageGroup );
		pInst->pObstacle_->superModel()->generateTextureUsage( usageGroup );

		return true;
	}


	virtual bool updateTextureUsage( 
		const SceneObject& object, 
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		using namespace CompiledSpace;
		PyModelObstacleEntityEmbodiment* pInst =
			object.getAs<PyModelObstacleEntityEmbodiment>();

		updateBounds( pInst, usageGroup );

		return true;
	}


	void updateBounds( PyModelObstacleEntityEmbodiment* pInst, 
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		Matrix worldXform = pInst->worldTransform_;
		AABB bb = pInst->localBB_;
		bb.transformBy( worldXform );
		
		usageGroup.setWorldScale_FromTransform( worldXform );
		usageGroup.setWorldBoundSphere( Sphere( bb ) );
	}
};


// ----------------------------------------------------------------------------
// static
void PyModelObstacleEntityEmbodiment::registerHandlers( Scene& scene )
{
	TickOperation* pOp = scene.getObjectOperation<TickOperation>();
	MF_ASSERT( pOp != NULL );
	pOp->addHandler<PyModelObstacleEntityEmbodiment, TickHandler>();

	DrawOperation* pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<PyModelObstacleEntityEmbodiment, DrawHandler>();

	CollisionOperation* pColOp = scene.getObjectOperation<CollisionOperation>();
	MF_ASSERT( pColOp != NULL );
	pColOp->addHandler<PyModelObstacleEntityEmbodiment, CollisionHandler>();

	ScriptObjectQueryOperation* pScriptObjectOp =
		scene.getObjectOperation<ScriptObjectQueryOperation>();
	MF_ASSERT( pScriptObjectOp != NULL );

	pScriptObjectOp->addHandler<PyModelObstacleEntityEmbodiment,
		ScriptObjectQuery<PyModelObstacleEntityEmbodiment> >();

	Moo::TextureStreamingObjectOperation* pTextureStreamingOp=
		scene.getObjectOperation<Moo::TextureStreamingObjectOperation>();
	MF_ASSERT( pTextureStreamingOp != NULL );

	pTextureStreamingOp->addHandler<PyModelObstacleEntityEmbodiment,
		PyModelObstacleEntityEmbodiment::TextureStreamingHandler >();
}


// ----------------------------------------------------------------------------
PyModelObstacleEntityEmbodiment::PyModelObstacleEntityEmbodiment(
	const PyModelObstaclePtr& pObstacle ) :
		IEntityEmbodiment( pObstacle ),
		localBB_( Vector3::ZERO, Vector3::ZERO ),
		pObstacle_( pObstacle ),
		pEnteredSpace_( NULL ),
		sceneObject_( this, SceneObjectFlags( 
			SceneObjectFlags::IS_DYNAMIC_OBJECT | 
			SceneObjectFlags::IS_CASTING_DYNAMIC_SHADOW) )
{
	MF_ASSERT( pObstacle_ );
	MF_ASSERT( !pObstacle_->attached() );

	pObstacle_->attach();

	this->syncTransforms();
}


// ----------------------------------------------------------------------------
PyModelObstacleEntityEmbodiment::~PyModelObstacleEntityEmbodiment()
{
	pObstacle_->detach();
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::doTick( float dTime )
{
	pObstacle_->tick( dTime );
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::doMove( float dTime )
{
	if (pObstacle_->dynamic())
	{
		this->syncTransforms();
	}
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::syncTransforms()
{
	worldTransform_ = pObstacle_->worldTransform();
	inverseWorldTransform_.invert( worldTransform_ );
	if (pEnteredSpace_)
	{
		Matrix worldXform = this->doWorldTransform();
		BoundingBox collisionBB;
		pObstacle_->localBoundingBox( collisionBB, false );
		collisionBB.transformBy( worldXform );

		pEnteredSpace_->dynamicScene().updateObject( dynamicObjectHandle_,
			worldXform, collisionBB, collisionBB );
	}
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::doUpdateAnimations( float dTime )
{
	pObstacle_->updateAnimations( worldTransform_ );
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::doWorldTransform(
	const Matrix & /* transform */ )
{
}


// ----------------------------------------------------------------------------
const Matrix & PyModelObstacleEntityEmbodiment::doWorldTransform() const
{
	return worldTransform_;
}


// ----------------------------------------------------------------------------
const AABB & PyModelObstacleEntityEmbodiment::doLocalBoundingBox() const
{
	BoundingBox bb;
	pObstacle_->localBoundingBox( bb, true );
	localBB_ = bb;
	return localBB_;
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::doDraw( Moo::DrawContext & drawContext )
{
	MF_ASSERT( pObstacle_->inWorld() );

	pObstacle_->draw( drawContext, worldTransform_ );
}


// ----------------------------------------------------------------------------
bool PyModelObstacleEntityEmbodiment::doIsOutside() const
{
	return true;
}


// ----------------------------------------------------------------------------
bool PyModelObstacleEntityEmbodiment::doIsRegionLoaded( Vector3 testPos,
													float radius ) const
{
	return pEnteredSpace_ && pEnteredSpace_->loadStatus() >= 1.f;
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::doEnterSpace( ClientSpacePtr pSpace,
												bool transient )
{
	MF_ASSERT( pSpace != NULL );
	MF_ASSERT( pEnteredSpace_ == NULL );
	MF_ASSERT( !pObstacle_->inWorld() );

	pEnteredSpace_ =
		static_cast<CompiledSpace*>( pSpace.get() );

	Matrix worldXform = this->doWorldTransform();
	BoundingBox collisionBB;
	pObstacle_->localBoundingBox( collisionBB, false );
	collisionBB.transformBy( worldXform );

	BoundingBox invalidVisibilityBB;
	dynamicObjectHandle_ = 
		pEnteredSpace_->dynamicScene().addObject( sceneObject_,
		worldXform, collisionBB, invalidVisibilityBB );

	pObstacle_->enterWorld();
}


// ----------------------------------------------------------------------------
void PyModelObstacleEntityEmbodiment::doLeaveSpace( bool transient )
{
	MF_ASSERT( pEnteredSpace_ != NULL );
	MF_ASSERT( pObstacle_->inWorld() );

	pObstacle_->leaveWorld();
	pEnteredSpace_->dynamicScene().removeObject( dynamicObjectHandle_ );

	pEnteredSpace_ = NULL;
}



} // namespace CompiledSpace
} // namespace BW
