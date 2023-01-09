#include "pch.hpp"

#include "texture_streaming_scene_view.hpp"
#include "texture_streaming_manager.hpp"
#include "texture_usage.hpp"
#include "streaming_texture.hpp"

#include "cstdmf/bw_unordered_map.hpp"
#include "math/boundbox.hpp"
#include "scene/scene.hpp"
#include "scene/scene_object.hpp"
#include "scene/object_change_scene_view.hpp"

#if !defined( EDITOR_ENABLED )
#include "moo/geometrics.hpp"
#endif

BW_BEGIN_NAMESPACE

namespace Moo
{

bool TextureStreamingObjectOperation::registerTextureUsage( 
    const SceneObject& object,
    ModelTextureUsageGroup& usageGroup )
{
    ITextureStreamingObjectOperationTypeHandler* pHandler = 
        getHandler( object.type() );
    MF_ASSERT( pHandler );
    return pHandler->registerTextureUsage( object, usageGroup );
}


bool TextureStreamingObjectOperation::updateTextureUsage( 
    const SceneObject& object,
    ModelTextureUsageGroup& usageGroup )
{
    ITextureStreamingObjectOperationTypeHandler* pHandler = 
        getHandler( object.type() );
    MF_ASSERT( pHandler );
    return pHandler->updateTextureUsage( object, usageGroup );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void BaseTextureStreamingSceneViewProvider::beginProcessing()
{
    // Do nothing by default.
    // Some implementations may wish to lock down the list whilst
    // processing. Others may wish to implement a pre-process step
    // on the processing thread to prepare the data.
}


void BaseTextureStreamingSceneViewProvider::endProcessing()
{
    // DO nothing by default
}


void BaseTextureStreamingSceneViewProvider::calculateIdealMips( 
    TextureStreamingUsageCollector& usageCollector,
    const ViewpointData & viewpoint )
{
    Vector3 viewPosition = viewpoint.matrix.applyToOrigin();
    // Looking down Z axis
    Vector3 viewDir = viewpoint.matrix.applyToUnitAxisVector( 2 );

    // Iterate over all usage of the textures and adjust their mip levels 
    // according to their distance from the viewpoint
    const double log2conversion = log( 2 );
    double screenConstant = 
        log( viewpoint.resolution / (tan( viewpoint.fov / 2.0f ) * 2.0f) ) / 
        log2conversion;

    beginProcessing();

    const ModelTextureUsage* pUsageBegin = NULL;
    size_t numUsages = 0;
    getModelUsage( &pUsageBegin, &numUsages );

    for (size_t index = 0; index < numUsages; ++index)
    {
        const ModelTextureUsage& usage = pUsageBegin[index];

        // Calculate closest distance:
        Vector3 objPos = usage.bounds_.center();
        Vector3 camToObjDir = objPos - viewPosition;
        float distToObj = camToObjDir.length();
        float closestDistance = distToObj - usage.bounds_.radius();
        closestDistance = max( closestDistance, 0.1f );

        // By default attempt to load the maximum mip
        const uint32 MAX_MIP = std::numeric_limits<uint32>::max() - 1;
        double requiredMip = MAX_MIP; 
        if (usage.uvDensity_ > ModelTextureUsage::MIN_DENSITY)
        {
            double distanceValue = -log( closestDistance ) / log2conversion;
            double meshConstant = 
                -0.5f * log( usage.uvDensity_ / usage.worldScale_) / 
                log2conversion;

            requiredMip = screenConstant + meshConstant + distanceValue;
        }

        uint32 neededMip = uint32( ceil( max( 0.0, requiredMip ) ) );

        // Heuristic calculations:
        uint32 numPixels = static_cast< uint32 >(MATH_PI * usage.bounds_.radius() * 
            usage.bounds_.radius() * pow( (viewpoint.resolution / 
            (closestDistance * tan( viewpoint.fov / 2 ) * 2)), 2 ));

        // Calculate how far away from the center of the frustum the object is
        float angleToFrustumRadians = 0.0f;
        // Only calculate angle to object if we aren't inside the object! 
        // (the 0 distance)
        if ((distToObj - usage.bounds_.radius()) <= 0.0f)
        {
            // Calculate angle to the object's center
            camToObjDir = camToObjDir / distToObj; // Normalize
            angleToFrustumRadians = acos(camToObjDir.dotProduct(viewDir));

            // Adjust for bounds of the object
            angleToFrustumRadians -= asin( usage.bounds_.radius() / distToObj );
            // Adjust against the frustum
            angleToFrustumRadians -= viewpoint.fov / 2.0f;

            angleToFrustumRadians = 
                Math::clamp(0.0f, angleToFrustumRadians, MATH_PI);
        }

        usageCollector.submitTextureRequirements( usage.pTexture_.get(),
            neededMip, numPixels, angleToFrustumRadians );
    }

    endProcessing();
}


void BaseTextureStreamingSceneViewProvider::drawDebug( uint32 debugMode )
{
    if (debugMode != DebugTextures::DEBUG_TS_OBJECT_BOUNDS)
    {
        // Only supporting drawing object bounds for now
        return;
    }

    drawDebugUsages( 0x80808080 );
}


void BaseTextureStreamingSceneViewProvider::drawDebugUsages( uint32 color )
{
    beginProcessing();

    const ModelTextureUsage* pUsageBegin = NULL;
    size_t numUsages = 0;
    getModelUsage( &pUsageBegin, &numUsages );

    // Draw the bounding spheres of all usages
    for (size_t index = 0; index < numUsages; ++index)
    {
        const ModelTextureUsage& usage = pUsageBegin[index];

        // Disabling to avoid linker errors for Geometrics.
        // Long term fix is for Geometrics to be brought into Moo
#if !defined( EDITOR_ENABLED )
        Geometrics::instance().wireSphere( 
            usage.bounds_.center(), 
            usage.bounds_.radius(), 
            color );
#endif
    }

    endProcessing();
}


uint32 BaseTextureStreamingSceneViewProvider::usageCount()
{
    const ModelTextureUsage* pUsageBegin = NULL;
    size_t numUsages = 0;
    getModelUsage( &pUsageBegin, &numUsages );

    return static_cast<uint32>(numUsages);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class TextureStreamingSceneView::RuntimeProvider : 
    public SceneProvider,
    public BaseTextureStreamingSceneViewProvider
{
public:
    RuntimeProvider();
    ~RuntimeProvider();

    virtual void * getView(
        const SceneTypeSystem::RuntimeTypeID & sceneInterfaceTypeID);
    virtual void onSetScene( Scene* pOldScene, Scene* pNewScene );

    virtual void beginProcessing();
    virtual void endProcessing();
    virtual void getModelUsage( const ModelTextureUsage ** pUsages, 
        size_t* numUsage );

    void ignoreObjectsFrom( SceneProvider* pProvider );
    void unignoreObjectsFrom( SceneProvider* pProvider );
    bool isIgnoredProvider( SceneProvider* pProvider );

private:
    void onObjectsAdded( Scene * pScene, 
        const ObjectChangeSceneView::ObjectsAddedEventArgs& args );
    void onObjectsRemoved( Scene * pScene, 
        const ObjectChangeSceneView::ObjectsAddedEventArgs& args );
    void onObjectsChanged( Scene * pScene, 
        const ObjectChangeSceneView::ObjectsAddedEventArgs& args );

    ModelTextureUsageGroup::Data& fetchUsageForObject( const SceneObject& pObject );

    // Scene object mapping
    typedef BW::unordered_map<SceneObject, ModelTextureUsageGroup::Data> 
        ObjectUsageMap;
    ObjectUsageMap objectUsageMap_;

    TextureStreamingObjectOperation* pTextureStreamingOp_;
    typedef BW::set<SceneProvider*> ProviderSet;
    ProviderSet ignoredProviders_;

    ModelTextureUsageContext usageContext_;
};


TextureStreamingSceneView::RuntimeProvider::RuntimeProvider()
{

}


TextureStreamingSceneView::RuntimeProvider::~RuntimeProvider()
{
}


void * TextureStreamingSceneView::RuntimeProvider::getView( 
    const SceneTypeSystem::RuntimeTypeID & viewTypeID)
{
    void * result = NULL;

    exposeView<ITextureStreamingSceneViewProvider>( this, viewTypeID, result );

    return result;
}


void TextureStreamingSceneView::RuntimeProvider::onSetScene( 
    Scene* pOldScene, Scene* pNewScene )
{
    if (pOldScene)
    {
        ObjectChangeSceneView * pChangeView = 
            pOldScene->getView<ObjectChangeSceneView>();
        pChangeView->objectsAdded().remove<RuntimeProvider, 
            &RuntimeProvider::onObjectsAdded>( this );
        pChangeView->objectsRemoved().remove<RuntimeProvider, 
            &RuntimeProvider::onObjectsRemoved>( this );
        pChangeView->objectsChanged().remove<RuntimeProvider, 
            &RuntimeProvider::onObjectsChanged>( this );

        pTextureStreamingOp_ = NULL;
    }

    // Attach the dynamic texture streaming scene view provider
    if (pNewScene)
    {
        pTextureStreamingOp_ = 
            scene()->getObjectOperation<TextureStreamingObjectOperation>();

        ObjectChangeSceneView * pChangeView = 
            pNewScene->getView<ObjectChangeSceneView>();
        pChangeView->objectsAdded().add<RuntimeProvider, 
            &RuntimeProvider::onObjectsAdded>( this );
        pChangeView->objectsRemoved().add<RuntimeProvider, 
            &RuntimeProvider::onObjectsRemoved>( this );
        pChangeView->objectsChanged().add<RuntimeProvider, 
            &RuntimeProvider::onObjectsChanged>( this );
    }
}


void TextureStreamingSceneView::RuntimeProvider::beginProcessing()
{
    usageContext_.lock();
}


void TextureStreamingSceneView::RuntimeProvider::endProcessing()
{
    usageContext_.unlock();
}


void TextureStreamingSceneView::RuntimeProvider::getModelUsage( 
    const ModelTextureUsage** pUsages, size_t* numUsage )
{
    usageContext_.getUsages( pUsages, numUsage );
}


ModelTextureUsageGroup::Data& 
    TextureStreamingSceneView::RuntimeProvider::fetchUsageForObject( 
        const SceneObject& pObject )
{
    return objectUsageMap_[pObject];
}


void TextureStreamingSceneView::RuntimeProvider::ignoreObjectsFrom( 
    SceneProvider* pProvider )
{
    ignoredProviders_.insert( pProvider );
}


void TextureStreamingSceneView::RuntimeProvider::unignoreObjectsFrom( 
    SceneProvider* pProvider )
{
    ignoredProviders_.erase( pProvider );
}


bool TextureStreamingSceneView::RuntimeProvider::isIgnoredProvider( 
    SceneProvider* pProvider )
{
    return ignoredProviders_.find( pProvider ) != ignoredProviders_.end();
}


void TextureStreamingSceneView::RuntimeProvider::onObjectsAdded( 
    Scene * pScene, 
    const ObjectChangeSceneView::ObjectsAddedEventArgs& args )
{
    MF_ASSERT( pTextureStreamingOp_ );

    // Don't process this event if it's objects are ignored
    if (isIgnoredProvider( args.pProvider_ ))
    {
        return;
    }
    
    usageContext_.lock();

    for (size_t index = 0; index < args.numObjects_; ++index)
    {
        const SceneObject& object = args.pObjects_[index];

        if (!pTextureStreamingOp_->isSupported( object ))
        {
            // Texture streaming unsupported by this object type
            continue;
        }

        // Allocate a usage group for this object:
        ModelTextureUsageGroup::Data& usageData = fetchUsageForObject( object );
        ModelTextureUsageGroup usageGroup( usageData, usageContext_ );

        if (!pTextureStreamingOp_->registerTextureUsage( object, usageGroup ))
        {
            usageGroup.clearTextureUsage();
            objectUsageMap_.erase( object );
        }
        else
        {
            usageGroup.applyObjectChanges();
        }
    }

    usageContext_.unlock();
}


void TextureStreamingSceneView::RuntimeProvider::onObjectsRemoved( 
    Scene * pScene, 
    const ObjectChangeSceneView::ObjectsAddedEventArgs& args )
{
    MF_ASSERT( pTextureStreamingOp_ );

    // Don't process this event if it's objects are ignored
    if (isIgnoredProvider( args.pProvider_ ))
    {
        return;
    }

    usageContext_.lock();

    for (size_t index = 0; index < args.numObjects_; ++index)
    {
        const SceneObject& object = args.pObjects_[index];

        if (!pTextureStreamingOp_->isSupported( object ))
        {
            // Texture streaming unsupported by this object type
            continue;
        }

        // Allocate a usage group for this object:
        ModelTextureUsageGroup::Data& usageData = fetchUsageForObject( object );
        ModelTextureUsageGroup usageGroup( usageData, usageContext_ );
        usageGroup.clearTextureUsage();
        objectUsageMap_.erase( object );
    }

    usageContext_.unlock();
}


void TextureStreamingSceneView::RuntimeProvider::onObjectsChanged( 
    Scene * pScene, 
    const ObjectChangeSceneView::ObjectsAddedEventArgs& args )
{
    MF_ASSERT( pTextureStreamingOp_ );

    // Don't process this event if it's objects are ignored
    if (isIgnoredProvider( args.pProvider_ ))
    {
        return;
    }

    usageContext_.lock();

    for (size_t index = 0; index < args.numObjects_; ++index)
    {
        const SceneObject& object = args.pObjects_[index];

        if (!pTextureStreamingOp_->isSupported( object ))
        {
            // Texture streaming unsupported by this object type
            continue;
        }

        // Update the object's bounds 
        ModelTextureUsageGroup::Data& usageData = fetchUsageForObject( object );
        ModelTextureUsageGroup usageGroup( usageData, usageContext_ );
        
        if (!pTextureStreamingOp_->updateTextureUsage( object, usageGroup ))
        {
            usageGroup.clearTextureUsage();
            objectUsageMap_.erase( object );
        }
        else
        {
            usageGroup.applyObjectChanges();
        }
    }

    usageContext_.unlock();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
TextureStreamingSceneView::TextureStreamingSceneView()
    : pRuntimeProvider_( new RuntimeProvider() )
{

}


TextureStreamingSceneView::~TextureStreamingSceneView()
{
    bw_safe_delete( pRuntimeProvider_ );
}


void TextureStreamingSceneView::onSetScene( 
    Scene* pOldScene, Scene* pNewScene )
{
    if (pOldScene)
    {
        pOldScene->removeProvider( pRuntimeProvider_ );
    }

    // Attach the dynamic texture streaming scene view provider
    if (pNewScene)
    {
        pNewScene->addProvider( pRuntimeProvider_ );
    }
}


void TextureStreamingSceneView::calculateIdealMips(
    TextureStreamingUsageCollector& usageCollector,
    const ViewpointData & viewpoint )
{
    // Iterate over each provider individually
    for (ProviderCollection::iterator iter = providers_.begin();
        iter != providers_.end(); ++iter)
    {
        ProviderInterface* pProvider = *iter;
        
        pProvider->calculateIdealMips( usageCollector, viewpoint );
    }
}


void TextureStreamingSceneView::drawDebug( uint32 debugMode )
{
    // Iterate over each provider individually
    for (ProviderCollection::iterator iter = providers_.begin();
        iter != providers_.end(); ++iter)
    {
        ProviderInterface* pProvider = *iter;

        pProvider->drawDebug( debugMode );
    }
}


void TextureStreamingSceneView::ignoreObjectsFrom( SceneProvider* pProvider )
{
    pRuntimeProvider_->ignoreObjectsFrom( pProvider );
}


void TextureStreamingSceneView::unignoreObjectsFrom( SceneProvider* pProvider )
{
    pRuntimeProvider_->unignoreObjectsFrom( pProvider );
}


uint32 TextureStreamingSceneView::usageCount()
{
    uint32 count = 0;
    // Iterate over each provider individually
    for (ProviderCollection::iterator iter = providers_.begin();
        iter != providers_.end(); ++iter)
    {
        ProviderInterface* pProvider = *iter;
        count += pProvider->usageCount();
    }
    return count;
}

} // namespace Moo

BW_END_NAMESPACE

// texture_usage.cpp