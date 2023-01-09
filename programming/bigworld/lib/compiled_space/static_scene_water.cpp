#include "pch.hpp"

#include "static_scene_water.hpp"
#include "static_scene_provider.hpp"
#include "binary_format.hpp"
#include "string_table.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/draw_operation.hpp"
#include "scene/scene_draw_context.hpp"

#include "cstdmf/profiler.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
StaticSceneWaterHandler::StaticSceneWaterHandler() :
	reader_( NULL ),
	pStream_( NULL )
{
	
}


// ----------------------------------------------------------------------------
StaticSceneWaterHandler::~StaticSceneWaterHandler()
{

}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneWaterHandler::typeID() const
{
	return StaticSceneTypes::StaticObjectType::WATER;
}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneWaterHandler::runtimeTypeID() const
{
	return SceneObject::typeOf<Instance>();
}


// ----------------------------------------------------------------------------
bool StaticSceneWaterHandler::load( BinaryFormat& binFile,
						 StaticSceneProvider& staticScene,
						 const StringTable& strings,
						 SceneObject* pLoadedObjects,
						 size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneWaterHandler_load );

	reader_ = &binFile;

	size_t sectionIdx = binFile.findSection( 
		StaticSceneWaterTypes::FORMAT_MAGIC );
	if (sectionIdx == BinaryFormat::NOT_FOUND)
	{
		ASSET_MSG( "'%s' does not contain water.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	if (binFile.sectionVersion( sectionIdx ) != 
		StaticSceneWaterTypes::FORMAT_VERSION)
	{
		ASSET_MSG( "'%s' has water data with incompatible version.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	pStream_ = binFile.openSection( sectionIdx );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map Water section into memory.\n" );
		this->unload();
		return false;
	}

	if (!pStream_->read( waterData_ ))
	{
		ASSET_MSG( "WaterHandler: failed to read data.\n" );
		this->unload();
		return false;
	}

	this->loadWaterInstances( strings, pLoadedObjects, maxObjects );

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneWaterHandler::unload()
{
	freeWaterInstances();

	if (reader_ && pStream_)
	{
		reader_->closeSection( pStream_ );
	}

	pStream_ = NULL;
	reader_ = NULL;
	waterData_.reset();
}


// ----------------------------------------------------------------------------
void StaticSceneWaterHandler::loadWaterInstances( const StringTable& strings,
	SceneObject* pLoadedObjects, size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneWaterHandler_loadWaterInstances );

	size_t numWaterObjects = waterData_.size();

	// If this is hit, there's a mismatch between static scene 
	// definition and stored trees.
	MF_ASSERT( maxObjects == numWaterObjects );

	MF_ASSERT( instances_.empty() );
	instances_.resize( numWaterObjects );

	SceneObject sceneObj;
	sceneObj.type<Instance>();
	sceneObj.flags().isDynamic( false );

	for (size_t i = 0; i < numWaterObjects; ++i)
	{
		Instance& instance = instances_[i];
		instance.pDef_ = &waterData_[i];
		instance.pWater_ = NULL; // Created on demand

		loadWaterObject( waterData_[i], strings, instance );

		sceneObj.handle( &instances_.back() );
		pLoadedObjects[i] = sceneObj;
	}
}


// ----------------------------------------------------------------------------
void StaticSceneWaterHandler::freeWaterInstances()
{
	for (size_t i = 0; i < instances_.size(); ++i)
	{
		if (instances_[i].pWater_ == NULL)
		{
			continue;
		}

		Water::deleteWater( instances_[i].pWater_ );
		instances_[i].pWater_ = NULL;
	}

	instances_.clear();
}


// ----------------------------------------------------------------------------
bool StaticSceneWaterHandler::loadWaterObject( 
	StaticSceneWaterTypes::Water& def,
	const StringTable& strings,
	Instance& instance )
{
	instance.resources_.foamTexture_ = strings.entry( def.foamTexture_ );
	instance.resources_.reflectionTexture_ = strings.entry( def.reflectionTexture_ );
	instance.resources_.waveTexture_ = strings.entry( def.waveTexture_ );
	instance.resources_.transparencyTable_ = strings.entry( def.transparencyTable_ );

	return true;
}


// ----------------------------------------------------------------------------
float StaticSceneWaterHandler::loadPercent() const
{
	return 1.f;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneWaterHandler::DrawHandler : public IDrawOperationTypeHandler
{
public:
	virtual void doDraw( const SceneDrawContext & context,
		const SceneObject * pObjects, size_t count )
	{
		for (size_t i = 0; i < count; ++i)
		{
			StaticSceneWaterHandler::Instance* pInstance =
				pObjects[i].getAs<StaticSceneWaterHandler::Instance>();
			MF_ASSERT( pInstance );
			MF_ASSERT( pInstance->pDef_ );

			if (!pInstance->pWater_)
			{
				// create the water if this is the first time
				pInstance->pWater_ = new Water( pInstance->pDef_->state_,
					pInstance->resources_, NULL );
			}
			
			const AABB & worldBB = pInstance->pWater_->bb();

			Waters::addToDrawList( pInstance->pWater_ );

			// TODO: Hook up drawing of indoor water
			pInstance->pWater_->outsideVisible( true );
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//static
void StaticSceneWaterHandler::registerHandlers( Scene & scene )
{
	DrawOperation * pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<StaticSceneWaterHandler::Instance,
		StaticSceneWaterHandler::DrawHandler>();
}

} // namespace CompiledSpace
} // namespace BW
