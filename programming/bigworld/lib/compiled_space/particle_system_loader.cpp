#include "pch.hpp"

#include "particle_system_loader.hpp"
#include "static_scene_provider.hpp"
#include "binary_format.hpp"
#include "string_table.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/profiler.hpp"
#include "moo/draw_context.hpp"
#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/draw_operation.hpp"
#include "scene/scene_draw_context.hpp"

#include "space/dynamic_scene_provider.hpp"
#include "particle/particle_system_manager.hpp"
#include "resmgr/auto_config.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
ParticleSystemLoader::ParticleSystemLoader( 
	DynamicSceneProvider& dynamicScene ) :
	pReader_( NULL ),
	pStream_( NULL ),
	dynamicScene_( dynamicScene )
{
	ParticleSystemManager::init();
}


// ----------------------------------------------------------------------------
ParticleSystemLoader::~ParticleSystemLoader()
{
	ParticleSystemManager::fini();
}


// ----------------------------------------------------------------------------
bool ParticleSystemLoader::doLoadFromSpace( ClientSpace * pSpace,
	BinaryFormat& reader,
	const DataSectionPtr& pSpaceSettings,
	const Matrix& mappingTransform,
	const StringTable& stringTable )
{
	PROFILER_SCOPED( ParticleSystemLoader_doLoadFromSpace );

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection(
		ParticleSystemTypes::FORMAT_MAGIC,
		ParticleSystemTypes::FORMAT_VERSION,
		"ParticleSystemLoader" );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map particle system section into memory.\n" );
		this->unload();
		return false;
	}

	if (!pStream_->read( systemData_ ))
	{
		ASSET_MSG( "ParticleSystemLoader: failed to read data.\n" );
		this->unload();
		return false;
	}

	this->loadParticleSystems( stringTable );

	return true;
}


// ----------------------------------------------------------------------------
bool ParticleSystemLoader::doBind()
{
	addToScene();

	return true;
}


// ----------------------------------------------------------------------------
void ParticleSystemLoader::doUnload()
{
	freeParticleSystems();
	systemData_.reset();

	if (pReader_ && pStream_)
	{
		pReader_->closeSection( pStream_ );
	}

	pReader_ = NULL;
	pStream_ = NULL;
}


// ----------------------------------------------------------------------------
void ParticleSystemLoader::loadParticleSystems( const StringTable& strings )
{
	PROFILER_SCOPED( ParticleSystemLoader_loadParticleSystems );

	size_t numParticleObjects = systemData_.size();

	MF_ASSERT( instances_.empty() );
	instances_.resize( numParticleObjects );

	for (size_t i = 0; i < numParticleObjects; ++i)
	{
		Instance& instance = instances_[i];
		instance.pDef_ = &systemData_[i];
		instance.pLoader_ = this;
		instance.index_ = uint32(i);
		instance.dynamicObjectHandle_ = 
			DynamicObjectHandle::INVALID();
		loadParticleSystem( systemData_[i], strings, instance );

		if (instance.system_ == NULL)
		{
			continue;
		}
	}
}


// ----------------------------------------------------------------------------
void ParticleSystemLoader::freeParticleSystems()
{
	removeFromScene();

	for (size_t i = 0; i < instances_.size(); ++i)
	{
		Instance& instance = instances_[i];
		if (instance.system_ == NULL)
		{
			continue;
		}

		instance.system_->detach();
		instance.system_->leaveWorld();
		instance.system_ = NULL;
	}

	instances_.clear();
}


// ----------------------------------------------------------------------------
bool ParticleSystemLoader::loadParticleSystem( 
	ParticleSystemTypes::ParticleSystem& def,
	const StringTable& strings,
	Instance& instance )
{
	BW::string resourceID = strings.entry( def.resourceID_ );
	if (resourceID.empty())
	{
		return false;
	}

	DataSectionPtr pSystemRoot = BWResource::openSection( resourceID );
	if (!pSystemRoot)
	{
		return false;
	}

	instance.system_ = ParticleSystemManager::instance().loader().load(
		resourceID );
	if (!instance.system_)
	{
		return false;
	}
	
	// Initialize seedtime
	instance.seedTime_ = def.seedTime_;

	instance.system_->isStatic(true);
	instance.system_->attach( &instance );
	instance.system_->enterWorld();

	return true;
}


// ----------------------------------------------------------------------------
void ParticleSystemLoader::addToScene()
{
	SceneObject sceneObj;
	sceneObj.type<Instance>();
	sceneObj.flags().isDynamic( true );

	for (size_t i = 0; i < instances_.size(); ++i)
	{
		Instance& instance = instances_[i];
		if (instance.system_ == NULL)
		{
			continue;
		}

		sceneObj.handle( &instance );

		Vector3 position = instance.pDef_->worldTransform_.applyToOrigin();
		AABB bb( position, position );
		instance.dynamicObjectHandle_ = 
			dynamicScene_.addObject( sceneObj, instance.pDef_->worldTransform_, 
			bb, bb);
	}
}


// ----------------------------------------------------------------------------
void ParticleSystemLoader::removeFromScene()
{
	for (size_t i = 0; i < instances_.size(); ++i)
	{
		Instance& instance = instances_[i];
		if (instance.system_ == NULL)
		{
			continue;
		}

		dynamicScene_.removeObject( instance.dynamicObjectHandle_ );
		instance.dynamicObjectHandle_ =
			DynamicObjectHandle::INVALID();
	}
}


// ----------------------------------------------------------------------------
float ParticleSystemLoader::percentLoaded() const
{
	return 1.f;
}


// ----------------------------------------------------------------------------
const Matrix & ParticleSystemLoader::Instance::getMatrix() const
{
	return pDef_->worldTransform_;
}


// ----------------------------------------------------------------------------
bool ParticleSystemLoader::Instance::setMatrix( const Matrix & m )
{
	// Ignore the set
	return false;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static BasicAutoConfig<int> s_staggerInterval( 
	"environment/chunkParticleStagger", 50 );

class ParticleSystemLoader::TickHandler : public ITickOperationTypeHandler
{
public:
	TickHandler() :
		tickCount_( 0 )
	{

	}

	virtual void doTick( SceneObject* pObjects,
		size_t objectCount, float dTime )
	{
		for (size_t i = 0; i < objectCount; ++i)
		{
			ParticleSystemLoader::Instance* pInstance =
				pObjects[i].getAs<ParticleSystemLoader::Instance>();
			MF_ASSERT( pInstance );
			MF_ASSERT( pInstance->pDef_ );

			if (!pInstance->system_ )
			{
				continue;
			}

			float seedTime = pInstance->pDef_->seedTime_;
			if (pInstance->seedTime_ > 0.f)
			{		
				if (((pInstance->index_ + tickCount_++) % s_staggerInterval.value()) == 0)
				{		
					pInstance->system_->setDoUpdate();
					pInstance->seedTime_ -= dTime;
				}
			}

			bool updated = pInstance->system_->tick( dTime );

			if (updated)
			{
				ParticleSystemLoader* pLoader = 
					pInstance->pLoader_;
				
				BoundingBox worldCollisionBB;
				BoundingBox worldVisibilityBB;
				Matrix worldXform = pInstance->pDef_->worldTransform_;
				pInstance->system_->worldBoundingBox( worldCollisionBB, worldXform );
				pInstance->system_->worldVisibilityBoundingBox( worldVisibilityBB );
				pLoader->dynamicScene_.updateObject( 
					pInstance->dynamicObjectHandle_, 
					worldXform,
					worldCollisionBB,
					worldVisibilityBB);
			}
		}
	}

	virtual void doUpdateAnimations( SceneObject* pObjects,
		size_t objectCount, float dTime )
	{
	}

private:
	uint32 tickCount_;
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class ParticleSystemLoader::DrawHandler : public IDrawOperationTypeHandler
{
private:
	class ReflectionVisibleStateBlock : public Moo::GlobalStateBlock
	{
	public:
		virtual	void	apply( Moo::ManagedEffect* effect, ApplyMode mode )
		{
			if (mode == APPLY_MODE)
			{
				// disable zwrite
				Moo::rc().pushRenderState( D3DRS_ZWRITEENABLE ); 
				Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE ); 
			}
			else if (mode == UNDO_MODE)
			{
				Moo::rc().popRenderState(); 
			}
		}
	};
	ReflectionVisibleStateBlock	reflectionGlobalState_;
public:
	virtual void doDraw( const SceneDrawContext & context,
		const SceneObject * pObjects, size_t count )
	{
		for (size_t i = 0; i < count; ++i)
		{
			ParticleSystemLoader::Instance* pInstance =
				pObjects[i].getAs<ParticleSystemLoader::Instance>();
			MF_ASSERT( pInstance );
			ParticleSystemTypes::ParticleSystem* pDef = pInstance->pDef_;
			MF_ASSERT( pDef );

			if (!pInstance->system_)
			{
				continue;
			}

			bool isReflectionVisible = pDef->flags_ & 
				ParticleSystemTypes::FLAG_REFLECTION_VISIBLE;
			if (Moo::rc().reflectionScene() && 
				!isReflectionVisible)
			{
				return;
			}

			if (isReflectionVisible)
			{
				context.drawContext().pushGlobalStateBlock( &reflectionGlobalState_ );
			}

			float distance = (pDef->worldTransform_.applyToOrigin() -
				Moo::rc().invView().applyToOrigin()).length();	

			pInstance->system_->draw( context.drawContext(),
				pDef->worldTransform_, distance );

			if (isReflectionVisible)
			{
				context.drawContext().popGlobalStateBlock( &reflectionGlobalState_ );
			}
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//static
void ParticleSystemLoader::registerHandlers( Scene & scene )
{
	DrawOperation * pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<ParticleSystemLoader::Instance,
		ParticleSystemLoader::DrawHandler>();

	TickOperation* pTickOp = scene.getObjectOperation<TickOperation>();
	MF_ASSERT( pTickOp != NULL );
	pTickOp->addHandler<ParticleSystemLoader::Instance,
		ParticleSystemLoader::TickHandler>();
}
} // namespace CompiledSpace
} // namespace BW
