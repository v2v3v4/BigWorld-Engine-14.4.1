#include "pch.hpp"

#include "py_attachment_entity_embodiment.hpp"
#include "compiled_space.hpp"
#include "moo/lod_settings.hpp"

#include "scene/scene.hpp"
#include "scene/tick_scene_view.hpp"
#include "scene/draw_operation.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/scene_draw_context.hpp"
#include "moo/texture_streaming_scene_view.hpp"

namespace BW {
namespace CompiledSpace {


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class PyAttachmentEntityEmbodiment::TickHandler :
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
			MF_ASSERT( obj.isType<PyAttachmentEntityEmbodiment>() );

			PyAttachmentEntityEmbodiment* pAttachment =
				obj.getAs<PyAttachmentEntityEmbodiment>();
			MF_ASSERT( pAttachment != NULL );

			pAttachment->doTick( dTime );
		}
	}

	virtual void doUpdateAnimations( 
		SceneObject* pObjects, 
		size_t objectCount, float dTime )
	{
		for (size_t i = 0; i < objectCount; ++i)
		{
			SceneObject& obj = pObjects[i];
			MF_ASSERT( obj.isType<PyAttachmentEntityEmbodiment>() );

			PyAttachmentEntityEmbodiment* pAttachment =
				obj.getAs<PyAttachmentEntityEmbodiment>();
			MF_ASSERT( pAttachment != NULL );

			pAttachment->doUpdateAnimations( dTime );
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class PyAttachmentDrawHandler : 
	public IDrawOperationTypeHandler
{
public:
	void doDraw( const SceneDrawContext & context, 
		const SceneObject * pObjects, size_t count )
	{
		using namespace CompiledSpace;
		for (size_t i = 0; i < count; ++i)
		{
			PyAttachmentEntityEmbodiment* pInst =
				pObjects[i].getAs<PyAttachmentEntityEmbodiment>();
			
			pInst->doDraw( context.drawContext() );
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class PyAttachmentEntityEmbodiment::TextureStreamingHandler : 
	public Moo::ITextureStreamingObjectOperationTypeHandler
{
public:
	virtual bool registerTextureUsage( 
		const SceneObject& object,
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		using namespace CompiledSpace;
		PyAttachmentEntityEmbodiment* pInst =
			object.getAs<PyAttachmentEntityEmbodiment>();

		updateBounds( pInst->pAttachment_.get(), usageGroup );
		pInst->pAttachment_->generateTextureUsage( usageGroup );

		return true;
	}


	virtual bool updateTextureUsage( 
		const SceneObject& object, 
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		using namespace CompiledSpace;
		PyAttachmentEntityEmbodiment* pInst =
			object.getAs<PyAttachmentEntityEmbodiment>();

		updateBounds( pInst->pAttachment_.get(), usageGroup );

		return true;
	}


	void updateBounds( PyAttachment* pAttachment, 
		Moo::ModelTextureUsageGroup& usageGroup )
	{
		Matrix worldXform = pAttachment->worldTransform();
		BoundingBox textureUsageBB;
		pAttachment->textureUsageWorldBox( textureUsageBB, worldXform );

		usageGroup.setWorldScale_FromTransform( worldXform );
		usageGroup.setWorldBoundSphere( Sphere( textureUsageBB ) );
	}
};


// ----------------------------------------------------------------------------
// static
void PyAttachmentEntityEmbodiment::registerHandlers( Scene& scene )
{
	TickOperation* pOp = scene.getObjectOperation<TickOperation>();
	MF_ASSERT( pOp != NULL );
	pOp->addHandler<PyAttachmentEntityEmbodiment, TickHandler>();

	DrawOperation* pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<PyAttachmentEntityEmbodiment, PyAttachmentDrawHandler>();

	ScriptObjectQueryOperation* pScriptObjectOp =
		scene.getObjectOperation<ScriptObjectQueryOperation>();
	MF_ASSERT( pScriptObjectOp != NULL );

	pScriptObjectOp->addHandler<PyAttachmentEntityEmbodiment,
		ScriptObjectQuery<PyAttachmentEntityEmbodiment> >();

	Moo::TextureStreamingObjectOperation* pTextureStreamingOp=
		scene.getObjectOperation<Moo::TextureStreamingObjectOperation>();
	MF_ASSERT( pTextureStreamingOp != NULL );

	pTextureStreamingOp->addHandler<PyAttachmentEntityEmbodiment,
		PyAttachmentEntityEmbodiment::TextureStreamingHandler >();
}


// ----------------------------------------------------------------------------
PyAttachmentEntityEmbodiment::PyAttachmentEntityEmbodiment(
	const PyAttachmentPtr& pAttachment ) :
		IEntityEmbodiment( pAttachment ),
		localBB_( Vector3::ZERO, Vector3::ZERO ),
		worldTransform_( Matrix::identity ),
		pAttachment_( pAttachment ),
		pEnteredSpace_( NULL ),
		needsSync_( false ),
		sceneObject_( this, SceneObjectFlags( 
			SceneObjectFlags::IS_DYNAMIC_OBJECT ) )
{
	MF_ASSERT( pAttachment );
	MF_ASSERT( !pAttachment->isAttached() );

	pAttachment_->attach( this );

	sceneObject_.flags().isCastingDynamicShadow( pAttachment->castsShadow() );

}


// ----------------------------------------------------------------------------
PyAttachmentEntityEmbodiment::~PyAttachmentEntityEmbodiment()
{
	pAttachment_->detach();
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::doTick( float dTime )
{
	pAttachment_->tick( dTime );
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::doMove( float dTime )
{
	pAttachment_->move( dTime );
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::doUpdateAnimations( float dTime )
{
	const float lod = LodSettings::instance().calculateLod( worldTransform_ );
	pAttachment_->updateAnimations( worldTransform_, lod );
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::syncTransform()
{
	if (pEnteredSpace_)
	{
		Matrix worldXform = this->doWorldTransform();
		BoundingBox collisionBB;
		pAttachment_->localBoundingBox( collisionBB, false );
		if (!collisionBB.insideOut())
		{
			collisionBB.transformBy( worldXform );
		}

		BoundingBox visibilityBB;
		pAttachment_->localVisibilityBox( visibilityBB, false );
		if (!visibilityBB.insideOut())
		{
			visibilityBB.transformBy( worldXform );
		}

		pEnteredSpace_->dynamicScene().updateObject( dynamicObjectHandle_,
			worldXform, collisionBB, visibilityBB );
	}
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::doWorldTransform( const Matrix & transform )
{
	worldTransform_ = transform;
	this->syncTransform();
}


// ----------------------------------------------------------------------------
const Matrix & PyAttachmentEntityEmbodiment::doWorldTransform() const
{
	return worldTransform_;
}


// ----------------------------------------------------------------------------
const AABB & PyAttachmentEntityEmbodiment::doLocalBoundingBox() const
{
	BoundingBox bb;
	pAttachment_->localBoundingBox( bb, true );
	localBB_ = bb;
	return localBB_;
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::doDraw(Moo::DrawContext& drawContext )
{
	MF_ASSERT( pAttachment_->isInWorld() );

	pAttachment_->draw( drawContext, worldTransform_ );
}


// ----------------------------------------------------------------------------
bool PyAttachmentEntityEmbodiment::doIsOutside() const
{
	return true;
}


// ----------------------------------------------------------------------------
bool PyAttachmentEntityEmbodiment::doIsRegionLoaded( Vector3 testPos,
													float radius ) const
{
	return pEnteredSpace_ && pEnteredSpace_->loadStatus() >= 1.f;
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::doEnterSpace( ClientSpacePtr pSpace,
												bool transient )
{
	MF_ASSERT( pSpace != NULL );
	MF_ASSERT( pEnteredSpace_ == NULL );
	MF_ASSERT( !pAttachment_->isInWorld() );

	pEnteredSpace_ =
		static_cast<CompiledSpace*>( pSpace.get() );
	 
	Matrix worldXform = this->doWorldTransform();
	BoundingBox collisionBB;
	pAttachment_->localBoundingBox( collisionBB, false );
	if (!collisionBB.insideOut())
	{
		collisionBB.transformBy( worldXform );
	}

	BoundingBox visibilityBB;
	pAttachment_->localVisibilityBox( visibilityBB, false );
	if (!visibilityBB.insideOut())
	{
		visibilityBB.transformBy( worldXform );
	}

	dynamicObjectHandle_ = 
		pEnteredSpace_->dynamicScene().addObject( sceneObject_,
		worldXform, collisionBB, visibilityBB );

	pAttachment_->enterWorld();
}


// ----------------------------------------------------------------------------
void PyAttachmentEntityEmbodiment::doLeaveSpace( bool transient )
{
	MF_ASSERT( pEnteredSpace_ != NULL );
	MF_ASSERT( pAttachment_->isInWorld() );

	pAttachment_->leaveWorld();
	pEnteredSpace_->dynamicScene().removeObject( dynamicObjectHandle_ );

	pEnteredSpace_ = NULL;
}


// ----------------------------------------------------------------------------
const Matrix & PyAttachmentEntityEmbodiment::getMatrix() const
{
	return worldTransform_;
}


// ----------------------------------------------------------------------------
bool PyAttachmentEntityEmbodiment::setMatrix( const Matrix & m )
{
	PyAttachmentEntityEmbodiment::doWorldTransform( m );
	return true;
}

} // namespace CompiledSpace
} // namespace BW
