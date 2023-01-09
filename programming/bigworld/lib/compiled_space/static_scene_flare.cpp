#include "pch.hpp"

#include "static_scene_flare.hpp"
#include "static_scene_provider.hpp"
#include "binary_format.hpp"
#include "string_table.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/draw_operation.hpp"
#include "scene/scene_draw_context.hpp"

#include "romp/lens_effect_manager.hpp"
#include "math/colour.hpp"
#include "cstdmf/profiler.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
StaticSceneFlareHandler::StaticSceneFlareHandler() :
	reader_( NULL ),
	pStream_( NULL )
{
	
}


// ----------------------------------------------------------------------------
StaticSceneFlareHandler::~StaticSceneFlareHandler()
{
	
}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneFlareHandler::typeID() const
{
	return StaticSceneTypes::StaticObjectType::FLARE;
}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneFlareHandler::runtimeTypeID() 
	const
{
	return SceneObject::typeOf<Instance>();
}


// ----------------------------------------------------------------------------
bool StaticSceneFlareHandler::load( BinaryFormat& binFile,
	StaticSceneProvider& staticScene,
	const StringTable& strings,
	SceneObject* pLoadedObjects,
	size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneFlareHandler_load );

	reader_ = &binFile;

	size_t sectionIdx = binFile.findSection( 
		StaticSceneFlareTypes::FORMAT_MAGIC );
	if (sectionIdx == BinaryFormat::NOT_FOUND)
	{
		ASSET_MSG( "'%s' does not contain flares.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	if (binFile.sectionVersion( sectionIdx ) != 
		StaticSceneFlareTypes::FORMAT_VERSION)
	{
		ASSET_MSG( "'%s' has flare data with incompatible version.\n",
			binFile.resourceID() );
		this->unload();
		return false;
	}

	pStream_ = binFile.openSection( sectionIdx );
	if (!pStream_)
	{
		ASSET_MSG( "Failed to map flare section into memory.\n" );
		this->unload();
		return false;
	}

	if (!pStream_->read( flareData_ ))
	{
		ASSET_MSG( "FlareHandler: failed to read data.\n" );
		this->unload();
		return false;
	}

	this->loadFlares( strings, pLoadedObjects, maxObjects );

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneFlareHandler::unload()
{
	freeFlares();

	if (reader_ && pStream_)
	{
		reader_->closeSection( pStream_ );
	}

	pStream_ = NULL;
	reader_ = NULL;
	flareData_.reset();
}


// ----------------------------------------------------------------------------
void StaticSceneFlareHandler::loadFlares( const StringTable& strings,
	SceneObject* pLoadedObjects, size_t maxObjects )
{
	PROFILER_SCOPED( StaticSceneFlareHandler_loadFlares );

	size_t numFlareObjects = flareData_.size();

	// If this is hit, there's a mismatch between static scene 
	// definition and stored trees.
	MF_ASSERT( maxObjects == numFlareObjects );

	MF_ASSERT( instances_.empty() );
	instances_.reserve( numFlareObjects );

	SceneObject sceneObj;
	sceneObj.type<Instance>();
	sceneObj.flags().isDynamic( false );

	for (size_t i = 0; i < numFlareObjects; ++i)
	{
		Instance instance;
		instance.pDef_ = &flareData_[i];
		loadFlare( flareData_[i], strings, instance, i );

		instances_.push_back( instance );
		sceneObj.handle( &instances_.back() );
		pLoadedObjects[i] = sceneObj;
	}
}


// ----------------------------------------------------------------------------
void StaticSceneFlareHandler::freeFlares()
{
	for (size_t i = 0; i < instances_.size(); ++i)
	{
		Instance& instance = instances_[i];
		LensEffectManager::instance().forget( instance.lensEffectID_ );
	}

	instances_.clear();
}


// ----------------------------------------------------------------------------
bool StaticSceneFlareHandler::loadFlare( 
	StaticSceneFlareTypes::Flare& def,
	const StringTable& strings,
	Instance& instance, size_t index )
{
	BW::string resourceID = strings.entry( def.resourceID_ );
	if (resourceID.empty())
	{
		return false;
	}

	DataSectionPtr pFlareRoot = BWResource::openSection( resourceID );
	if (!pFlareRoot)
	{
		return false;
	}

	LensEffect & le = instance.lensEffect_;
	instance.active_ = false;
	if (le.load( pFlareRoot ))
	{
		le.maxDistance( def.maxDistance_ );
		le.area( def.area_ );
		le.fadeSpeed( def.fadeSpeed_ );
		instance.active_ = true;
	}
	instance.lensEffectID_ = size_t(&def);

	return true;
}


// ----------------------------------------------------------------------------
float StaticSceneFlareHandler::loadPercent() const
{
	return 1.f;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class StaticSceneFlareHandler::DrawHandler : public IDrawOperationTypeHandler
{
public:
	virtual void doDraw( const SceneDrawContext & context, 
		const SceneObject * pObjects, size_t count )
	{
		if (!context.renderFlares())
		{
			return;
		}

		for (size_t i = 0; i < count; ++i)
		{
			StaticSceneFlareHandler::Instance* pInstance =
				pObjects[i].getAs<StaticSceneFlareHandler::Instance>();
			MF_ASSERT( pInstance );
			StaticSceneFlareTypes::Flare* pDef = pInstance->pDef_;
			MF_ASSERT( pDef );

			LensEffect & le = pInstance->lensEffect_;

			Vector4 tintColour( pDef->colour_ / 255.f, 1.f );
			uint32 oldColour = le.DEFAULT_COLOUR;

			bool colourApplied = 
				pDef->flags_ & StaticSceneFlareTypes::FLAG_COLOUR_APPLIED;
			if (colourApplied)
			{
				// Multiply our tinting colour with the flare's base colour
				oldColour = le.colour();
				Vector4 flareColour( Colour::getVector4Normalised( oldColour ) );
				flareColour.w *= tintColour.w;
				flareColour.x *= tintColour.x;
				flareColour.y *= tintColour.y;
				flareColour.z *= tintColour.z;
				le.colour( Colour::getUint32FromNormalised( flareColour ) );
			}

			LensEffectManager::instance().add(
				pInstance->lensEffectID_, 
				pDef->position_, le );

			if (colourApplied)
			{
				le.colour( oldColour );
			}
		}
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//static
void StaticSceneFlareHandler::registerHandlers( Scene & scene )
{
	DrawOperation * pDrawOp = scene.getObjectOperation<DrawOperation>();
	MF_ASSERT( pDrawOp != NULL );
	pDrawOp->addHandler<StaticSceneFlareHandler::Instance,
		StaticSceneFlareHandler::DrawHandler>();
}

} // namespace CompiledSpace
} // namespace BW
