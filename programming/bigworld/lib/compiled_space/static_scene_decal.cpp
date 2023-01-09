#include "pch.hpp"

#include "static_scene_decal.hpp"
#include "static_scene_provider.hpp"
#include "binary_format.hpp"
#include "string_table.hpp"

#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/draw_operation.hpp"
#include "scene/scene_draw_context.hpp"

#include "cstdmf/profiler.hpp"

#include "moo/deferred_decals_manager.hpp"

namespace BW {
namespace CompiledSpace {

// ----------------------------------------------------------------------------
StaticSceneDecalHandler::StaticSceneDecalHandler() :
    reader_( NULL ),
    pStream_( NULL )
{
    
}


// ----------------------------------------------------------------------------
StaticSceneDecalHandler::~StaticSceneDecalHandler()
{
    
}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneDecalHandler::typeID() const
{
    return StaticSceneTypes::StaticObjectType::DECAL;
}


// ----------------------------------------------------------------------------
SceneTypeSystem::RuntimeTypeID StaticSceneDecalHandler::runtimeTypeID() 
    const
{
    return SceneObject::typeOf<Instance>();
}


// ----------------------------------------------------------------------------
bool StaticSceneDecalHandler::load( BinaryFormat& binFile,
    StaticSceneProvider& staticScene,
    const StringTable& strings,
    SceneObject* pLoadedObjects,
    size_t maxObjects )
{
    PROFILER_SCOPED( StaticSceneDecalHandler_load );

    reader_ = &binFile;

    pStream_ = binFile.findAndOpenSection(
        StaticSceneDecalTypes::FORMAT_MAGIC,
        StaticSceneDecalTypes::FORMAT_VERSION,
        "StaticSceneDecal" );
    if (!pStream_)
    {
        this->unload();
        return false;
    }

    if (!pStream_->read( decalData_ ))
    {
        ASSET_MSG( "StaticSceneDecalHandler: failed to read data.\n" );
        this->unload();
        return false;
    }

    this->loadDecals( strings, pLoadedObjects, maxObjects );
    return true;
}


// ----------------------------------------------------------------------------
void StaticSceneDecalHandler::unload()
{
    this->freeDecals();

    if (reader_ && pStream_)
    {
        reader_->closeSection( pStream_ );
    }

    pStream_ = NULL;
    reader_ = NULL;
    decalData_.reset();
}


// ----------------------------------------------------------------------------
void StaticSceneDecalHandler::loadDecals( const StringTable& strings,
    SceneObject* pLoadedObjects, size_t maxObjects )
{
    PROFILER_SCOPED( StaticSceneDecalHandler_loadDecals );

    size_t numDecalbjects = decalData_.size();

    // If this is hit, there's a mismatch between static scene 
    // definition and stored decals.
    MF_ASSERT( maxObjects == numDecalbjects );

    MF_ASSERT( instances_.empty() );
    instances_.resize( numDecalbjects );

    SceneObject sceneObj;
    sceneObj.type<Instance>();
    sceneObj.flags().isDynamic( false );

    for (size_t i = 0; i < numDecalbjects; ++i)
    {
        Instance& instance = instances_[i];
        instance.pDef_ = &decalData_[i];
        this->loadDecal( decalData_[i], strings, instance, i );

        sceneObj.handle( &instance );
        pLoadedObjects[i] = sceneObj;
    }
}


// ----------------------------------------------------------------------------
void StaticSceneDecalHandler::freeDecals()
{
    for (size_t i = 0; i < instances_.size(); ++i)
    {
        Instance& instance = instances_[i];
        
        if (instance.instanceID_ != -1)
        {
            DecalsManager::instance().removeStaticDecal(
                instance.instanceID_ );
            instance.instanceID_ = -1;
        }
    }

    instances_.clear();
}


// ----------------------------------------------------------------------------
bool StaticSceneDecalHandler::loadDecal( 
    StaticSceneDecalTypes::Decal& def,
    const StringTable& strings,
    Instance& instance, size_t index )
{
    DecalsManager::Decal::Desc desc;

    desc.m_transform = def.worldTransform_;
    desc.m_map1 = strings.entry( def.diffuseTexture_ );
    desc.m_map2 = strings.entry( def.bumpTexture_ );
    desc.m_priority = def.priority_;
    desc.m_materialType = (DecalsManager::Decal::EMaterialType)def.materialType_;
    desc.m_influenceType = def.influenceType_;

    MF_ASSERT( desc.m_materialType < DecalsManager::Decal::MATERIAL_COUNT );

    instance.instanceID_ = DecalsManager::instance().createStaticDecal( desc );
    if (instance.instanceID_ == -1)
    {
        return false;
    }

    return true;
}


// ----------------------------------------------------------------------------
float StaticSceneDecalHandler::loadPercent() const
{
    return 1.f;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
class StaticSceneDecalHandler::DrawHandler : public IDrawOperationTypeHandler
{
public:
    virtual void doDraw( const SceneDrawContext & context, 
        const SceneObject * pObjects, size_t count )
    {
        for (size_t i = 0; i < count; ++i)
        {
            StaticSceneDecalHandler::Instance* pInstance =
                pObjects[i].getAs<StaticSceneDecalHandler::Instance>();
            MF_ASSERT( pInstance );

            DecalsManager::instance().markVisible( pInstance->instanceID_ );
        }
    }
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//static
void StaticSceneDecalHandler::registerHandlers( Scene & scene )
{
    DrawOperation * pDrawOp = scene.getObjectOperation<DrawOperation>();
    MF_ASSERT( pDrawOp != NULL );
    pDrawOp->addHandler<StaticSceneDecalHandler::Instance,
        StaticSceneDecalHandler::DrawHandler>();
}


} // namespace CompiledSpace
} // namespace BW
