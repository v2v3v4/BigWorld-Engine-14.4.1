#include "pch.hpp"

#include "dynamic_shadow.hpp"
#include "sticker_decal.hpp"

#include "cstdmf/dogwatch.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_tree.hpp"
#include "chunk/chunk_terrain.hpp"
#include "terrain/terrain2/terrain_renderer2.hpp"
#include "duplo/chunk_embodiment.hpp"
#include "duplo/chunk_attachment.hpp"
#include "duplo/pymodel.hpp"

#include "effect_visual_context.hpp"
#include "render_context.hpp"
#include "render_target.hpp"
#include "effect_material.hpp"
#include "moo_utilities.hpp"
#include "viewport_setter.hpp"
#include "complex_effect_material.hpp"

#include "romp/time_of_day.hpp"
#include "moo/geometrics.hpp"
#include "moo/lod_settings.hpp"

#include "cstdmf/watcher.hpp"
#include "cstdmf/watcher_path_request.hpp"

#include "speedtree/speedtree_renderer.hpp"
#include "speedtree/speedtree_tree_type.hpp"

#include "shadows_lib/shadow_math.hpp"
#include "shadow_manager.hpp"
#include "moo/custom_AA.hpp"
#include "moo/draw_context.hpp"
#include "moo/fullscreen_quad.hpp"
#include "renderer.hpp"
#include "render_event.hpp"

#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "scene/draw_helpers.hpp"
#include "scene/intersection_set.hpp"
#include "scene/scene_draw_context.hpp"
#include "scene/scene_intersect_context.hpp"

#include "moo/render_context_callback.hpp"


#ifdef UMBRA_ENABLE
#include "chunk/chunk_umbra.hpp"
#include "chunk/umbra_draw_item.hpp"
#endif

#include <algorithm>

BW_BEGIN_NAMESPACE

//--------------------------------------------------------------------------------------------------
// Unnamed namespace.
//--------------------------------------------------------------------------------------------------

#define BW_SHADOW_WRAPPER_WATCHER(var)                      \
    MF_WATCH("Render/Shadows/bw/" #var,              \
    var, Watcher::WT_READ_WRITE, " ")                       \

namespace
{
    //----------------------------------------------------------------------------------------------
    //-- Watchers
    //----------------------------------------------------------------------------------------------

    //-- common

    bool g_enableReceivePass = true;
    bool g_enableCastPass = true;

    bool g_enableSimplifiedTree = false;
    bool g_enableTreeShadowCasting = true;
    bool g_enableTerrainShadowCasting = true;
    bool g_enableObjectsShadowCasting = true;

    float g_splitSchemeLambda = 0.6f;

    bool g_enableSimplifiedMatrices = true;

    //-- distances

    float g_lightCamDistDynamic = 500.0f;
    float g_lightCamDistTerrain = 300.0f;

    //-- ESM (terrain shadows)

    float g_esmK = 16.0f;
    bool  g_esmBlureEnabled = true;
    float g_esmGaussianRho = 5.0f;
    float g_esmGaussianW = 2.0f;

    //-- TODO: must have

    bool g_enableJitteringSuppressing = true;
    bool g_enableCullItems = true;
    //bool g_enableDebugCamera = false;
    //bool g_enableAtlases = false;
    //bool g_psmEnabled = false;

    // Statistic
    uint g_currentNumSplit       = 0;
    uint g_currentDrawChunkItems = 0;

    uint32 g_numDrawCalls = 0;
    uint32 g_numDrawPrimitives = 0;

    //----------------------------------------------------------------------------------------------

    bool _createEffect(Moo::EffectMaterialPtr& mat, const BW::string& name)
    {
        BW_GUARD;

        if (!mat) mat = new Moo::EffectMaterial;
        if(mat->initFromEffect(name)) {
            return true;
        }
        mat = NULL;
        return false;
    }

    //----------------------------------------------------------------------------------------------

    bool _createRenderTarget(Moo::RenderTargetPtr& rt, uint width, uint height,
        D3DFORMAT pixelFormat,  const BW::string& name, 
        bool reuseMainZBuffer = false, 
        Moo::RenderTargetPtr pDepthStencilParent = NULL)
    {
        BW_GUARD;

        if(Moo::rc().supportsTextureFormat(pixelFormat)) {
            if(!rt) rt = new Moo::RenderTarget(name);
            if(rt->create(width, height, reuseMainZBuffer, pixelFormat, pDepthStencilParent.get())) 
            {
                return true;
            }
        }
        rt = NULL;
        return false;
    }

    //----------------------------------------------------------------------------------------------

    void _drawDebugTexturedRect(
        const Vector2& topLeft, const Vector2& bottomRight, DX::BaseTexture* texture, 
        Moo::Colour color = Moo::Colour(0,0,0,0))
    {
        BW_GUARD;

        Moo::Material::setVertexColour();
        Moo::rc().setRenderState(D3DRS_ZENABLE, FALSE);
        Moo::rc().setRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        Moo::rc().setTexture(0, texture);
        Geometrics::texturedRect(topLeft, bottomRight, color, false);
        Moo::rc().setTexture(0, NULL);
    }

    //----------------------------------------------------------------------------------------------
    // Items drawing
    //----------------------------------------------------------------------------------------------

    enum ITEM_TYPE {
        ITEM_TYPE_MODEL         = 1,
        ITEM_TYPE_EMBODIMENT    = 2,
        ITEM_TYPE_TREE          = 4
    };

    //----------------------------------------------------------------------------------------------

    void _castItemModel( Moo::DrawContext& drawContext, ChunkModel* cm )
    {
        BW_GUARD;
        if( cm->wantsDraw() )
        {
            cm->draw( drawContext );
        }
    }

    void _castItemTree( Moo::DrawContext& drawContext, ChunkTree* ct )
    {
        BW_GUARD;
        if( ct->wantsDraw() ) 
        {
            ct->draw( drawContext );
        }
    }

    void _castItemEmbodiment( Moo::DrawContext& drawContext, ChunkEmbodiment* ce )
    {
        BW_GUARD;
        if( ce->wantsDraw() ) 
        {
            ce->draw( drawContext );
        }
    }

    template<class T> void 
        _castItem( Moo::DrawContext& drawContext, ChunkItem* ci )
    {
        BW_GUARD;
        T* ct = dynamic_cast<T*>(ci);
        if(ct && ct->wantsDraw()) {
            ct->draw( drawContext );
        }
    }
    
    //----------------------------------------------------------------------------------------------

    void _castItems( Moo::DrawContext& drawContext, const Chunk::Items & items, SHADOW_TYPE shadowType, int itemTypes, ChunkEmbodiment* playerChunkEmbodiment) 
    {
        BW_GUARD;

        for(size_t i = 0; i < items.size(); ++i) 
        {
            ChunkItem* ci  = items[i].get();
            
            if( ci->sceneObject().flags().isCastingDynamicShadow() ) 
            {
                const std::type_info& ti = typeid(*ci);

#ifndef EDITOR_ENABLED // use static cast in client builds only.
                if((ITEM_TYPE_MODEL & itemTypes) && ti == typeid(ChunkModel)) 
                {
                    _castItemModel( drawContext, static_cast<ChunkModel*>(ci) );
                }
                else if((ITEM_TYPE_TREE & itemTypes) && ti == typeid(ChunkTree))
                {
                    _castItemTree( drawContext, static_cast<ChunkTree*> (ci) );
                }
                else if((ITEM_TYPE_EMBODIMENT & itemTypes))
                {
                    // TODO: remove dynamic_cast.
                    ChunkEmbodiment* ce = dynamic_cast<ChunkEmbodiment*>( ci );
                    if( ce ) 
                    {
                        _castItemEmbodiment( drawContext, ce );
                    }
                }
#else 
                if(ITEM_TYPE_MODEL & itemTypes) 
                    _castItem<ChunkModel>( drawContext, ci );
                if(ITEM_TYPE_TREE & itemTypes) 
                    _castItem<ChunkTree>( drawContext, ci );
                if(ITEM_TYPE_EMBODIMENT & itemTypes) 
                    _castItem<ChunkEmbodiment>( drawContext, ci );              
#endif // EDITOR_ENABLED
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    void _castChunk( Moo::DrawContext& drawContext, Chunk& c, SHADOW_TYPE shadowType,
        int itemTypes, const VectorNoDestructor<Chunk*>& others ) 
    {
        BW_GUARD;

        if( itemTypes & (ITEM_TYPE_EMBODIMENT | ITEM_TYPE_MODEL | ITEM_TYPE_TREE) )
        {
            // dyno and self items
            
            Moo::rc().push();
            Moo::rc().world(c.transform());

            {
                static DogWatch dw("dynoItems");
                ScopedDogWatch sdw(dw);

                _castItems( drawContext, c.dynoItems(), shadowType, itemTypes, NULL );
            }

            {
                static DogWatch dw("selfItems");
                ScopedDogWatch sdw(dw);

                _castItems( drawContext, c.selfItems(), shadowType, itemTypes, NULL );
            }
            
            Moo::rc().pop();

            {
                static DogWatch dw("lenders");
                ScopedDogWatch sdw(dw);
                
                Moo::rc().push();

                const Chunk::Lenders& ls = c.lenders();
                for(size_t i = 0; i < ls.size(); ++i) 
                {
                    const Chunk* lender = ls[i]->pLender_;
                    
                    if ( std::find( others.begin(), others.end(), lender ) != others.end() )
                    {
                        // Skip the lender, all of which will still be drawn
                        continue;
                    }

                    Moo::rc().world( lender->transform() );
                    _castItems( drawContext, ls[i]->items_, shadowType, itemTypes, NULL );
                }
                
                Moo::rc().pop();
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    
    void _cullChunk( 
        const BW::vector<Chunk*>&   chunksCasters, 
        const Matrix&                viewProjectionMatrix, 
        VectorNoDestructor<Chunk*>&  outChunks
    )
    {
        BW_GUARD;

        BW::vector<Chunk*>::const_iterator chunkIt = chunksCasters.begin();

        for ( ; chunkIt != chunksCasters.end(); ++chunkIt )
        {
            Chunk& c = **chunkIt;
            if( c.isBound() ) 
            {
                BoundingBox bb = c.visibilityBox();
                bb.calculateOutcode( viewProjectionMatrix );

                if( !bb.combinedOutcode() ) 
                {
                    outChunks.push_back( &c );
                }
            }
        }
    }

    //----------------------------------------------------------------------------------------------

    struct ShadowSplitInfo
    {
        typedef BW::vector<Chunk*>::const_iterator ChunkIt;
        typedef Chunk::Items::const_iterator ChunkItemIt;
        typedef const Chunk::Items & ChunkItemsList;

        BW::vector<Chunk*>        chunks_;
        BW::vector<ChunkItem*>    items_;
        IntersectionSet sceneObjects_;

    private:
        
        void cullItems( ChunkItemsList itemList, 
            const Matrix & chunkTransform, 
            const Matrix & viewProjectionMatrix, 
            int options )
        {
            using namespace Moo;

            static DogWatch dw("cullItems");
            ScopedDogWatch sdw(dw);

            ChunkItemIt itemIt = itemList.begin();

            for ( ; itemIt != itemList.end(); ++itemIt )
            {
                ChunkItem* item = (*itemIt).get();

#if UMBRA_ENABLE
                if ( g_enableCullItems )
                {
                    UmbraDrawShadowItem* pShadowItem = item->pUmbraDrawShadowItem();
                    if ( !pShadowItem || pShadowItem->isDynamic() )
                    {
                        items_.push_back( item );
                    }
                    else
                    {
                        BoundingBox bb = pShadowItem->bbox();
                        bb.transformBy( pShadowItem->transform() );
                        bb.calculateOutcode( viewProjectionMatrix );
                        
                        if( !bb.combinedOutcode() ) 
                        {
                            items_.push_back( item );
                        }
                    }
                }
                else
#endif // UMBRA_ENABLE
                {
                    items_.push_back( item );
                }
            }
        }

        void cullChunksNative( const Matrix & viewProjectionMatrix, int options )
        {
            using namespace Moo;

            BW_GUARD;

            clear();

            ChunkSpace*   cs = ChunkManager::instance().cameraSpace().get();
            EnviroMinder* em = &ChunkManager::instance().cameraSpace()->enviro();
            
            if( NULL == cs || NULL == em ) 
                return;

            GridChunkMap& gcm = cs->gridChunks();

            int min_x = cs->minGridX();
            int min_y = cs->minGridY();
            int max_x = cs->maxGridX();
            int max_y = cs->maxGridY();

            for( int i = min_x; i <= max_x; ++i ) 
            {
                for( int j = min_y; j <= max_y; ++j ) 
                {
                    BW::vector<Chunk*>& chunkList = gcm[ std::make_pair(i, j) ];

                    for( size_t ci = 0; ci < chunkList.size(); ++ci ) 
                    {
                        Chunk& c = *chunkList[ci];

                        if( c.isBound() ) 
                        {
                            BoundingBox bb = c.visibilityBox();
                            bb.calculateOutcode( viewProjectionMatrix );

                            if( !bb.combinedOutcode() ) 
                            {
                                chunks_.push_back( &c );

                                if ( options == DynamicShadow::QM_DINAMYC_CAST_ALL )
                                {
                                    cullItems( c.selfItems(), c.transform(), viewProjectionMatrix, options );
                                }

                                cullItems( c.dynoItems(), c.transform(), viewProjectionMatrix, options );
                            }
                        }
                    }
                }
            }
        }

    public:

        void cullChunks( const Matrix & viewProjectionMatrix, int options )
        {
            using namespace Moo;

            BW_GUARD;

#if UMBRA_ENABLE
            if ( ChunkManager::instance().umbra()->castShadowCullingEnabled() )
            {
                this->cullChunksUmbraUsage( ChunkManager::instance().umbraChunkShadowCasters(), viewProjectionMatrix, options );
            }
            else
#endif // UMBRA_ENABLE
            {
                this->cullChunksNative( viewProjectionMatrix, options );
            }
        }

        struct DynamicShadowObjectFilter
        {
            bool operator()(const SceneObject& obj) const
            {
                return !obj.flags().isCastingDynamicShadow();
            }
        };

        void cullScene( const Matrix & viewProjectionMatrix, int options )
        {
            using namespace Moo;

            BW_GUARD;

            clear();

            ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
            if (pSpace == NULL)
            {
                return;
            }

            Scene& scene = pSpace->scene();

            SceneIntersectContext cullContext( scene );
            cullContext.onlyShadowCasters( true );
            cullContext.includeStaticObjects( 
                options == DynamicShadow::QM_DINAMYC_CAST_ALL );

            DrawHelpers::cull( cullContext, scene, viewProjectionMatrix, 
                sceneObjects_ );

            // Perform post filtering on the objects
            for (uint32 typeIndex = 0; typeIndex < sceneObjects_.maxType(); 
                ++typeIndex)
            {
                IntersectionSet::SceneObjectList& objects =
                    sceneObjects_.objectsOfType( typeIndex );

                objects.erase(std::remove_if( objects.begin(), objects.end(), 
                    DynamicShadowObjectFilter() ), objects.end() );
            }
        }

#if UMBRA_ENABLE
        // We provide culling objects. You can run on different threads.
        void cullChunksUmbraUsage( const BW::vector<Chunk*>& allChunk, const Matrix & viewProjectionMatrix, int options )
        {
            BW_GUARD;

            clear();

            ChunkIt chunkIt = allChunk.begin();
            for ( ; chunkIt != allChunk.end(); ++chunkIt )
            {
                Chunk& c = **chunkIt;
                Matrix transform = c.transform();

                if( c.isBound() ) 
                {
                    BoundingBox bb = c.visibilityBox();
                    bb.calculateOutcode( viewProjectionMatrix );

                    if( !bb.combinedOutcode() ) 
                    {
                        chunks_.push_back( &c );

                        cullItems( c.shadowItems(), transform, viewProjectionMatrix, options );
                    }
                }
            }
        }
#endif // UMBRA_ENABLE

        void cast( const Matrix& lodCameraView, Moo::DrawContext& drawContext )
        {
            BW_GUARD;

            ClientSpacePtr pSpace = DeprecatedSpaceHelpers::cameraSpace();
            if (pSpace == NULL)
            {
                return;
            }

            Moo::rc().push();

            Scene& scene = pSpace->scene();

            SceneDrawContext sceneDrawContext( scene, drawContext );
            sceneDrawContext.lodCameraView( lodCameraView );
            DrawHelpers::drawSceneVisibilitySet( sceneDrawContext, scene, 
                sceneObjects_ );


            IRendererPipeline* rp = Renderer::instance().pipeline();
            BW::Moo::ShadowManager* selector = rp->dynamicShadow();
            if ( selector )
            {
                BW::Moo::ShadowManager::RenderItemsList& itemsList = selector->getRenderItems();
                if ( !itemsList.empty() )
                {
                    BW::Moo::ShadowManager::RenderItemsList::iterator itemIt = itemsList.begin();
                    for ( ;itemIt != itemsList.end(); ++itemIt )
                    {
                        (*itemIt)->render( drawContext );
                    }
                }
            }

            Moo::rc().pop();
        }

        void castItems( Moo::DrawContext& drawContext )
        {
            BW_GUARD;

            Chunk* chunk = NULL;

            Moo::rc().push();

            for ( BW::vector<ChunkItem*>::iterator itemIt = items_.begin(); itemIt != items_.end(); ++itemIt )
            {
                ChunkItem* item = *itemIt;

                // Check the item still exists in the world, as the item
                // may have been removed from the world since the last frame.
                // and this list of items comes from the previous frame's drawn
                // objects
                if ( !item->chunk() )
                {
                    continue;
                }

                if ( chunk != item->chunk() )
                {
                    chunk = item->chunk();

                    // Lenders
                    const Chunk::Lenders& ls = chunk->lenders();

                    for( size_t i = 0; i < ls.size(); ++i ) 
                    {
                        const Chunk* lender = ls[i]->pLender_;

                        if ( std::find( chunks_.begin(), chunks_.end(), lender ) != chunks_.end() )
                        {
                            // Skip the Lender, all of which will still be drawn
                            continue;
                        }

                        Moo::rc().world( lender->transform() );

                        const Chunk::Items & lenderItems = ls[i]->items_;
                        Chunk::Items::const_iterator lItemIt = lenderItems.begin();

                        for ( ; lItemIt != lenderItems.end(); ++lItemIt )
                        {
                            ChunkItem* item = ( *lItemIt ).get();

                            // The list of items to cast in
                            // ChunkManager::umbraChunkShadowCasters_ is
                            // collected from the last frame's draw.
                            // UmbraChunkItemShadowCasters add themselves to the
                            // list in their draw method, but shadows are
                            // rendered before the draw for this frame.
                            // Check if the item has not been removed from the
                            // scene during tick just before shadow casting
                            const bool stillInFrame = true;
                    
                            if (item->sceneObject().flags().isCastingDynamicShadow() &&
                                item->wantsDraw() &&
                                stillInFrame ) 
                            {
                                ++g_currentDrawChunkItems;
                                item->draw( drawContext );
                            }
                        }
                    }

                    Moo::rc().world( chunk->transform() );
                }

                if (item->sceneObject().flags().isCastingDynamicShadow()) 
                {
                    // The list of items to cast in
                    // ChunkManager::umbraChunkShadowCasters_ is
                    // collected from the last frame's draw.
                    // UmbraChunkItemShadowCasters add themselves to the
                    // list in their draw method, but shadows are
                    // rendered before the draw for this frame.
                    // Check if the item has not been removed from the
                    // scene during tick just before shadow casting
                    const bool stillInFrame = true;

                    if (item->wantsDraw() && stillInFrame)
                    {
                        ++g_currentDrawChunkItems;
                        item->draw( drawContext );
                    }
                }
            }

            // -----------------------

            IRendererPipeline* rp = Renderer::instance().pipeline();
            BW::Moo::ShadowManager* selector = rp->dynamicShadow();
            if ( selector )
            {
                BW::Moo::ShadowManager::RenderItemsList& itemsList = selector->getRenderItems();
                if ( !itemsList.empty() )
                {
                    BW::Moo::ShadowManager::RenderItemsList::iterator itemIt = itemsList.begin();
                    for ( ;itemIt != itemsList.end(); ++itemIt )
                    {
                        (*itemIt)->render( drawContext );
                    }
                }
            }

            Moo::rc().pop();
        }

        void clear()
        {
            sceneObjects_.clear();
            items_.clear();
            chunks_.clear();
        }
    };

    //----------------------------------------------------------------------------------------------

    void _castAllItems( Moo::DrawContext& drawContext, 
        const Matrix& lodCameraView, 
        const Matrix& cullViewProj,
        SHADOW_TYPE shadowType, int itemTypes, int options )
    {
        BW_GUARD;

        static VectorNoDestructor<Chunk*> s_drawChunks;
        s_drawChunks.clear();

        static ShadowSplitInfo s_drawSplitInfo;
        
        //s_drawSplitInfo.cullChunks( cullViewProj, options );
        //s_drawSplitInfo.castItems( drawContext );
    
        s_drawSplitInfo.cullScene( cullViewProj, options );
        s_drawSplitInfo.cast( lodCameraView, drawContext );

        Chunk::nextMark();
    }

    //----------------------------------------------------------------------------------------------

    struct BiasParams 
    {
        float biasConstant;
        float biasSlopeScale;
        float maxZ;
        float texelSizeWorldX;
        float texelSizeWorldY;
    };

    void _castObjects(
        Moo::DrawContext& drawContext, 
        const Matrix& lodCameraView, 
        const Matrix& view, 
        const Matrix& proj, 
        const Matrix& cullViewProj,
        SHADOW_TYPE shadowType, 
        int itemType,
        Moo::EffectMaterialPtr& terrainEffectMaterial, int options )
    {
        BW_GUARD;

        if(abs(view.getDeterminant()) < 0.0001) { // a bug in the hangar
            return;
        }

        //-- retrieve matrices
        Moo::rc().view(view);
        Moo::rc().projection(proj);
        Moo::rc().updateViewTransforms();

        //-- update shared constants.
        Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_VIEW);

        //Moo::Visual::s_pDrawOverride = casterOverride.get();
        PyStickerModel::s_enabledDrawing = false;

#if SPEEDTREE_SUPPORT
        float oldTreeLodeMode =  0.0f;
        if( g_enableTreeShadowCasting && (itemType & ITEM_TYPE_TREE) ) 
        {
            // speedtree::TSpeedTreeType::s_isShadowCasterPass = true;
            // after updating viewProj matrices.
            // Must be called in ChunkManager::instance().draw();
            // speedtree::SpeedTreeRenderer::beginFrame(NULL); //pass EnvMinder?
            
            if( g_enableSimplifiedTree ) 
            {
                oldTreeLodeMode = speedtree::SpeedTreeRenderer::lodMode( 0.0f );
            }
        }
#endif // SPEEDTREE_SUPPORT

        //---------------------------------------------------------------------
        // Draw objects
        //---------------------------------------------------------------------

        Matrix invView(view);
        invView.invert();

        drawContext.begin( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
        {
            static DogWatch dw("castAllItems");
            ScopedDogWatch sdw(dw);

            _castAllItems( drawContext, lodCameraView, cullViewProj, shadowType, itemType, options );
        }

        drawContext.end( Moo::DrawContext::OPAQUE_CHANNEL_MASK );
        drawContext.flush( Moo::DrawContext::OPAQUE_CHANNEL_MASK );

        //ChunkManager::instance().camera(oldCamera, oldSpace);

        //---------------------------------------------------------------------
        // Restore states
        //---------------------------------------------------------------------
#if SPEEDTREE_SUPPORT
        if( g_enableTreeShadowCasting && (itemType & ITEM_TYPE_TREE) ) 
        {
            static DogWatch dw("drawDefferedTrees");
            ScopedDogWatch sdw(dw);

            if( g_enableSimplifiedTree ) 
            {
                speedtree::SpeedTreeRenderer::lodMode(oldTreeLodeMode);
            }
        }
#endif // SPEEDTREE_SUPPORT

        PyStickerModel::s_enabledDrawing = true;
    }
    
    //----------------------------------------------------------------------------------------------
    //-- Matrices calculation
    //----------------------------------------------------------------------------------------------

    void _findChunksInBB(VectorNoDestructor<Chunk*>& outChunks, const BoundingBox& bb)
    {
        ChunkSpace* cs = ChunkManager::instance().cameraSpace().get();
        if(cs) {
            EnviroMinder* em = &ChunkManager::instance().cameraSpace()->enviro();
            if(em) {
                GridChunkMap& gcm = cs->gridChunks();
                int min_x = cs->minGridX();
                int min_y = cs->minGridY();
                int max_x = cs->maxGridX();
                int max_y = cs->maxGridY();
                for(int i = min_x; i <= max_x; ++i) {
                    for(int j = min_y; j <= max_y; ++j) {                       
                        std::pair<int, int> coord(i, j);
                        BW::vector<Chunk*>& chunks = gcm[coord];
                        for(size_t ci = 0; ci < chunks.size(); ++ci) {
                            Chunk& c = *chunks[ci];
                            if(c.isBound() && c.visibilityBox().intersects(bb)) {
                                outChunks.push_back(&c);
                            }
                        }
                    }
                }
                //Chunk::nextMark();
            }
        }
    }

    Vector3 _retriveLightDir()
    {
        Vector3 lightDir(0.f, 0.f, 0.f);

        ClientSpacePtr cs = DeprecatedSpaceHelpers::cameraSpace();
        if( cs ) 
        {
            EnviroMinder& enviro =  cs->enviro();
            lightDir = enviro.timeOfDay()->lighting().mainLightDir();
            lightDir.normalise();
        }

        return lightDir;
    }

    //-- New calculation lightViewProj - Matrix
    //----------------------------------------------------------------------------------------------
    // TODO: return const
    bool _calculateLightMatricesNew(
        Matrix& outLightView, Matrix& outLightProj, Matrix& outLightTexture, // output params
        Moo::Camera& mainCamera, Matrix& mainView,                           // main camera params      
        Vector3& lightDir, float lightDistance,                              // light params
        float nearPlane, float farPlane)                                     // split params
    {
        BW_GUARD;

        //-- DEBUG: START

        //mainCamera = Moo::Camera(1.f, 2.f, MATH_PI, 1);
        //mainView.lookAt(Vector3(0,0,0), Vector3(0, 0, 1.f), Vector3(0, 1.f, 0));
        //lightDir = Vector3(0, 0, -1.f);
        //lightDistance = 10.f;
        //nearPlane = 1.f;
        //farPlane = 2.f;

        //-- DEBUG: END

        //-- Bound by a set of points which will be built
        // viewProj matrix light source
        static VectorNoDestructor<Vector3> points;
        points.clear();

        //-- frustum split
        Moo::Camera splitCamera(mainCamera);
        splitCamera.nearPlane(nearPlane);
        splitCamera.farPlane(farPlane);

        //-- split view and projection matrices
        Matrix splitProj;
        splitProj.perspectiveProjection(splitCamera.fov(), splitCamera.aspectRatio(), 
            splitCamera.nearPlane(), splitCamera.farPlane());

        Matrix splitViewProj(mainView);
        splitViewProj.postMultiply(splitProj);

        Matrix splitViewInv;
        splitViewInv.invert(mainView);

        Matrix splitViewProjInv;
        splitViewProjInv.invert(splitViewProj);

        //-- split frustum vertices in world space
        Vector3 splitFrustumPointsView[8] = {
            //-- near plane (locks from main camera)
            //-- 0 1
            //-- 3 2
            splitCamera.nearPlanePoint(-1.f, 1.f), 
            splitCamera.nearPlanePoint( 1.f, 1.f),
            splitCamera.nearPlanePoint( 1.f,-1.f), 
            splitCamera.nearPlanePoint(-1.f,-1.f),

            //-- far plane (locks from main camera)
            //-- 4 5
            //-- 7 6
            splitCamera.farPlanePoint(-1.f, 1.f),
            splitCamera.farPlanePoint( 1.f, 1.f),
            splitCamera.farPlanePoint( 1.f,-1.f),
            splitCamera.farPlanePoint(-1.f,-1.f)
        };

        Vector3 splitFrustumPointsWorld[8];
        for(size_t i = 0; i < 8; ++i) {
            splitFrustumPointsWorld[i] = splitViewInv.applyPoint(
                splitFrustumPointsView[i]);
        }

        struct Edge
        {
            Edge(const int& b, const int& e)
                : begin(b), end(e)
            { ; }

            size_t begin;
            size_t end;
        };

        //-- indices of the split frustum edges
        Edge splitFrustumEdges[] = { 
            //-- near plane, clock wise order
            Edge(0, 1), Edge(1, 2), Edge(2, 3), Edge(3, 0), 

            //-- far plane, clock wise order
            Edge(4, 5), Edge(5, 6), Edge(6, 7), Edge(7, 4), 

            //-- sides, clock wise order
            Edge(0, 4), Edge(1, 5), Edge(2, 6), Edge(3, 7), 
        };

        //-- list chunks that potentially intersect the frustum
        static VectorNoDestructor<Chunk*> chunks;
        chunks.clear();

        BoundingBox splitFrustumBBWorld;
        for(size_t i = 0; i < 8; ++i) {
            splitFrustumBBWorld.addBounds(splitFrustumPointsWorld[i]);
        }

        _findChunksInBB(chunks, splitFrustumBBWorld);

        //-- bound edges list
        Edge boundingBoxEdges[] = {
            // TODO: coincides with frustum
            //-- top plane, clock wise
            Edge(0, 1), Edge(1, 2), Edge(2, 3), Edge(3, 0), 

            //-- bottom plane, clock wise
            Edge(4, 5), Edge(5, 6), Edge(6, 7), Edge(7, 4), 

            //-- sides, clock wise
            Edge(0, 4), Edge(1, 5), Edge(2, 6), Edge(3, 7), 
        };

        BoundingBox clipBB(
            Vector3(-1.f, -1.f, 0.f),
            Vector3( 1.f,  1.f, 1.f));

        //-- for each chunk
        for(size_t i = 0; i < chunks.size(); ++i) {

            const BoundingBox& chunkBboundWorld = chunks[i]->visibilityBox();

            const Vector3& max = chunkBboundWorld.maxBounds();
            const Vector3& min = chunkBboundWorld.minBounds();

            //-- bound points world space
            Vector3 chunkBoundPointsWorld[8] = {
                //-- tom plane (locks from top to bottom)
                //-- 0 1 
                //-- 3 2
                Vector3(min.x, max.y, max.z),
                Vector3(max.x, max.y, max.z),
                Vector3(max.x, max.y, min.z),
                Vector3(min.x, max.y, min.z),

                //-- bottom plane (locks from top to bottom)
                //-- 4 5
                //-- 7 6
                Vector3(min.x, min.y, max.z),
                Vector3(max.x, min.y, max.z),
                Vector3(max.x, min.y, min.z),
                Vector3(min.x, min.y, min.z)
            };

            ////-- bound points split frustum space
            //Vector3 boundingPointsFrustum[8];
            //for(size_t i = 0; i < 8; ++i) {
            //  boundingPointsFrustum[i] = splitViewProj.applyPoint(
            //      boundPointsWorld[i]);
            //}

            ////-- find collision of bound edges with frustum in frustum space
            //for(size_t i = 0; i < 12; ++i) { 
            //  Edge edge = boundingBoxEdges[i];

            //  Vector3 begin = boundingPointsFrustum[edge.begin];
            //  Vector3 end = boundingPointsFrustum[edge.end];

            //  if(clipBB.clip(begin, end)) {
            //      Vector3 beginWorld = splitViewProjInv.applyPoint(begin);
            //      Vector3 endWorld = splitViewProjInv.applyPoint(end);

            //      points.push_back(beginWorld);
            //      points.push_back(endWorld);
            //  }
            //}

            //-- find collisions of frustum edges with chunk bounds in world space
            for(size_t i = 0; i < 12; ++i) {
                Edge edge = splitFrustumEdges[i];

                Vector3 begin = splitFrustumPointsWorld[edge.begin];
                Vector3 end = splitFrustumPointsWorld[edge.end];

                if(chunkBboundWorld.clip(begin, end)) {
                    points.push_back(begin);
                    points.push_back(end);
                }
            }
        }

        //-- return if receiver points are not exist
        if(points.size() < 2) return false;

        //-- find light view and projection matrices
        Matrix lightView;
        Matrix lightProj;
        {
            //--------------------------------------------------------------------------------------
            //--------------------------------------------------------------------------------------
            //--------------------------------------------------------------------------------------

            ////-- first light view matrix
            //Vector3 eye = Vector3(0.f, 0.f, 0.f);
            //Vector3 dir = lightDir;
            //Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
            //if((lightDir.x * lightDir.x + lightDir.z * lightDir.z) < 0.001) {
            //  up = Vector3(0.f, 0.f, 1.f);
            //}
            //lightView.lookAt(eye, dir, up);

            ////-- points bound in light direction
            //BoundingBox pointsBB;
            //for(size_t i = 0; i < points.size(); ++i) {
            //  pointsBB.addBounds(
            //      lightView.applyPoint(points[i]));
            //}

            ////-- fix light view matrix
            //lightView.postTranslateBy(pointsBB.centre() - lightDir * lightDistance);

            ////-- light projection matrix
            //lightProj.orthogonalProjection(pointsBB.width(), pointsBB.height(), 
            //  0.01f, lightDistance + pointsBB.depth() / 2.f);

            //--------------------------------------------------------------------------------------
            //--------------------------------------------------------------------------------------
            //--------------------------------------------------------------------------------------

            //-- Bound center point in world coordinates computed along the
            // ray of the sun
            Matrix tmpView;
            tmpView.lookAt(Vector3(0.f, 0.f, 0.f), lightDir, Vector3(0.f, 1.f, 0.f)); // TODO: up
            BoundingBox pointsBoundTmpView;
            for(size_t i = 0; i < points.size(); ++i) {
                Vector3 tmp = tmpView.applyPoint(points[i]);
                pointsBoundTmpView.addBounds(tmp);
            }
            tmpView.invert();
            Vector3 c = tmpView.applyPoint(pointsBoundTmpView.centre());

            //-- lightView matrix

            lightView.lookAt(c - lightDir * lightDistance, lightDir, Vector3(0.f, 1.f, 0.f));

            //-- Bound in LightSpace
            BoundingBox pointsBoundLightView;
            for(size_t i = 0; i < points.size(); ++i) {
                Vector3 tmp = lightView.applyPoint(points[i]);
                pointsBoundLightView.addBounds(tmp);
            }

            lightProj.orthogonalProjection(pointsBoundLightView.width(), 
                pointsBoundLightView.height(), 1.f, pointsBoundLightView.maxBounds().z);

            //lightView.lookAt(-lightDir * lightDistance, lightDir, Vector3(0.f, 1.f, 0.f));
            //lightProj.orthogonalProjection(100.0f, 100.0f, 1.f, 500.f);
        }

        //-- find texture matrix
        float texOffset = 0.5f; //+ (0.5f / (float)m_shadowMapSize);
        Matrix textureScaleBiasMat = Matrix(
            Vector4(0.5f,        0.0f,         0.0f,   0.0f),
            Vector4(0.0f,       -0.5f,         0.0f,   0.0f),
            Vector4(0.0f,        0.0f,         1.0f,   0.0f),
            Vector4(texOffset,   texOffset,    0.0f,   1.0f)
            );

        Matrix lightTexture(lightView);
        lightTexture.postMultiply(lightProj);
        lightTexture.postMultiply(textureScaleBiasMat);

        //-- return
        outLightView = lightView;
        outLightProj = lightProj;
        outLightTexture = lightTexture;
        return true;
    }

    //----------------------------------------------------------------------------------------------

    bool _calculateLightMatricesOld(
        Matrix& outLightView, 
        Matrix& outLightProj, 
        Matrix& outLightTexture, 
        Matrix& outCullViewProj,                        // output params
        Moo::Camera& mainCamera, Matrix& mainView,      // main camera params
        Vector3& lightDir, float lightDistance,         // light params
        float nearPlane, float farPlane)                // split params
    {
        BW_GUARD;
#if 1
        if( g_enableJitteringSuppressing ) 
        {
            Matrix tmpLightViewProj;
            calcLightViewProjectionMatrixJitteringSuppressed(
                outLightView, outLightProj, tmpLightViewProj, 
                outCullViewProj,
                mainView, 
                nearPlane, farPlane, 
                mainCamera.fov(), mainCamera.aspectRatio(), 
                lightDir, lightDistance,
                1024 
            );
        } 
        else 
#endif
        {
            Vector2 tmpTexelSizeWorld;
            float tmpMaxZ;
            float tmpSmSize = 1024.f;
            Matrix tmpLightViewProj;
            calcLightViewProjectionMatrix(
                outLightView, outLightProj, tmpLightViewProj, 
                tmpTexelSizeWorld, tmpMaxZ,
                mainView, 
                nearPlane, farPlane, 
                mainCamera.fov(), mainCamera.aspectRatio(),
                lightDir, lightDistance, tmpSmSize);

            outCullViewProj = tmpLightViewProj;
        }

        //-- find texture matrix
        float texOffsetX = 0.5f; // + (0.5f / (float) 1024);
        float texOffsetY = 0.5f; // + (0.5f / (float) 1024);

        Matrix textureScaleBiasMat = Matrix(
            Vector4(0.5f,       0.0f,       0.0f, 0.0f),
            Vector4(0.0f,       -0.5f,      0.0f, 0.0f),
            Vector4(0.0f,       0.0f,       1.0f /* range > 1 */, 0.0f),
            Vector4(texOffsetX, texOffsetY, 0.0f /* bias      */, 1.0f)
        );

        outLightTexture = outLightView;
        outLightTexture.postMultiply(outLightProj);
        outLightTexture.postMultiply(textureScaleBiasMat);

        return true;
    }

    //----------------------------------------------------------------------------------------------

    bool _calculateLightMatrices(
        Matrix& outLightView, Matrix& outLightProj, Matrix& outLightTexture, Matrix & outCullViewProj, // output params
        Moo::Camera& mainCamera, Matrix& mainView,                           // main camera params      
        Vector3& lightDir, float lightDistance,                              // light params
        float nearPlane, float farPlane)                                     // split params
    {
        if ( g_enableSimplifiedMatrices )
        {
            return _calculateLightMatricesOld(
                                    outLightView, outLightProj, outLightTexture, outCullViewProj,
                                    mainCamera, mainView,
                                    lightDir, lightDistance,
                                    nearPlane, farPlane);
        }

        bool ret = _calculateLightMatricesNew(
            outLightView, outLightProj, outLightTexture,
            mainCamera, mainView,
            lightDir, lightDistance,
            nearPlane, farPlane);

        outCullViewProj = outLightTexture;
        return ret;
    }

    //----------------------------------------------------------------------------------------------
    // ESM
    //----------------------------------------------------------------------------------------------

    static float _getGaussianDistribution(float x, float y, float rho) 
    {
        float g = 1.0f / sqrt( 2.0f * 3.141592654f * rho * rho );
        return g * exp( -(x * x + y * y) / (2 * rho * rho) );
    }

    static void _getGaussianOffsets(bool bHorizontal, D3DXVECTOR2 vViewportTexelSize,
        D3DXVECTOR2* vSampleOffsets, float* fSampleWeights) 
    {
        const float rho = g_esmGaussianRho;
        const float w = g_esmGaussianW;

        // Get the center texel offset and weight
        fSampleWeights[0] = 1.0f * _getGaussianDistribution( 0, 0, rho );
        vSampleOffsets[0] = D3DXVECTOR2( 0.0f, 0.0f );

        // Get the offsets and weights for the remaining taps
        if(bHorizontal) {
            for(int i = 1; i < 15; i += 2) {
                vSampleOffsets[i + 0] = D3DXVECTOR2( i * vViewportTexelSize.x, 0.0f);
                vSampleOffsets[i + 1] = D3DXVECTOR2(-i * vViewportTexelSize.x, 0.0f);

                fSampleWeights[i + 0] = w * _getGaussianDistribution(float(i + 0), 0.0f, rho);
                fSampleWeights[i + 1] = w * _getGaussianDistribution(float(i + 1), 0.0f, rho);
            }
        } else {
            for(int i = 1; i < 15; i += 2) {
                vSampleOffsets[i + 0] = D3DXVECTOR2(0.0f,  i * vViewportTexelSize.y);
                vSampleOffsets[i + 1] = D3DXVECTOR2(0.0f, -i * vViewportTexelSize.y);

                fSampleWeights[i + 0] = w * _getGaussianDistribution(0.0f, float(i + 0), rho);
                fSampleWeights[i + 1] = w * _getGaussianDistribution(0.0f, float(i + 1), rho);
            }
        }
    }

    // const float g_atlasWidth = 2.0f;  //-- TODO: document
    // const float g_atlasHeight = 2.0f; //-- TODO: document

    //----------------------------------------------------------------------------------------------

    static const Moo::DynamicShadow::ShadowQuality & getShadowQuality( int index )
    {
        using Moo::DynamicShadow;

        static const DynamicShadow::ShadowQuality options[BW::Moo::ShadowManager::QUALITY_TOTAL] = {
            // { NumSplit, BlurQuality,   CastMask }
            { 4, 0, DynamicShadow::QM_DINAMYC_CAST_ALL             }, //-- MAX
            { 3, 0, DynamicShadow::QM_DINAMYC_CAST_ALL             }, //-- HIGH
            { 3, 1, DynamicShadow::QM_DINAMYC_CAST_ALL             }, //-- MEDIUM
            { 3, 1, DynamicShadow::QM_DINAMYC_CAST_ATTACHMENT_ONLY }, //-- LOW
            { 3, 0, DynamicShadow::QM_DINAMYC_CAST_ALL,            }, //-- OFF
            { 4, 0, DynamicShadow::QM_DINAMYC_CAST_ALL,            }, //-- EDITORS
        };

        return options[index];
    }
}

//--------------------------------------------------------------------------------------------------
// End unnamed namespace.
//--------------------------------------------------------------------------------------------------

namespace Moo {

    DynamicShadow::DynamicShadow(
        bool  debugMode,
        uint  terrainShadowMapSize,
        uint  dynamicShadowMapSize,
        float viewDist,
        int   shadowQuality
    )
        : m_inited(false)
        , m_enabled(false)
        , m_unmanagedObjectsCreated(false)
        , m_debugMode(debugMode)
        , m_terrainShadowMapSize(terrainShadowMapSize)
        , m_dynamicShadowMapSize(dynamicShadowMapSize)
        , m_viewDist(viewDist)
        , m_shadowQuality(getShadowQuality(shadowQuality))
    {
        BW_GUARD;

        m_inited = true;

#if UMBRA_ENABLE
        ChunkManager::instance().umbra()->castShadowCullingEnabled(m_shadowQuality.m_castMask == QM_DINAMYC_CAST_ALL);
#endif // UMBRA_ENABLE

        // watchers

        MF_WATCH("Render/Shadows/Dynamic/enableReceivePass",
            g_enableReceivePass,
            Watcher::WT_READ_WRITE,
            "Disables receiving shadows.");

        MF_WATCH("Render/Shadows/Dynamic/enableCastPass",
            g_enableCastPass,
            Watcher::WT_READ_WRITE,
            "Stop updating the shadows (casting).");

        MF_WATCH("Render/Shadows/Dynamic/enableTerrainShadowCasting",
            g_enableTerrainShadowCasting,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/enableObjectsShadowCasting",
            g_enableObjectsShadowCasting,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/enableTreeShadowCasting",
            g_enableTreeShadowCasting,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/splitSchemeLambda",
            g_splitSchemeLambda,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/enableSimplifiedMatrices",
            g_enableSimplifiedMatrices,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/lightCamDistDynamic",
            g_lightCamDistDynamic,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/lightCamDistTerrain",
            g_lightCamDistTerrain,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/enableSimplifiedTree",
            g_enableSimplifiedTree,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/esm/K",
            g_esmK,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/esm/gaussianW",
            g_esmGaussianW,
            Watcher::WT_READ_WRITE,
            " ");

        MF_WATCH("Render/Shadows/Dynamic/EnableJitteringSuppressing",
            g_enableJitteringSuppressing,
            Watcher::WT_READ_WRITE,
            "Disables shadow jittering (reduced quality)");

        MF_WATCH("Render/Shadows/Dynamic/EnableChunkItemBBoxCulling",
            g_enableCullItems,
            Watcher::WT_READ_WRITE,
            "Includes clipping BoundingBox for ChunkItems. "
            "(Must be included umbra).");

        // Statistic 

        MF_WATCH(
            "Render/Shadows/Dynamic/CurrentDrawNumSplit",
            g_currentNumSplit,
            Watcher::WT_READ_ONLY,
            "");

        MF_WATCH("Render/Shadows/Dynamic/CurrentDrawNumChunkItems",
            g_currentDrawChunkItems,
            Watcher::WT_READ_ONLY,
            "");

        MF_WATCH(
            "Render/Shadows/Dynamic/NumDrawCalls",
            g_numDrawCalls,
            Watcher::WT_READ_ONLY,
            "");

        MF_WATCH("Render/Shadows/Dynamic/NumDrawPrimitives",
            g_numDrawPrimitives,
            Watcher::WT_READ_ONLY,
            "");

        MF_WATCH("Render/Shadows/Dynamic/NumSplits",
            *this,
            &DynamicShadow::getNumSplit,
            &DynamicShadow::setNumSplit,
            "");
    }

    //----------------------------------------------------------------------------------------------

    DynamicShadow::~DynamicShadow()
    {
        BW_GUARD;

#if ENABLE_WATCHERS
        if (Watcher::hasRootWatcher())
        {
            Watcher::rootWatcher().removeChild("Render/Shadows/Dynamic");
            Watcher::rootWatcher().removeChild("Render/Shadows/Dynamic/NumSplits");
        }
#endif
        enable(false);
        m_inited = false;
    }

    //----------------------------------------------------------------------------------------------

    bool DynamicShadow::enable(bool isEnabled)
    {
        BW_GUARD;

        if (!m_inited) return false;

        //-- enable
        if (isEnabled && !m_enabled) {
            m_enabled = true;

            createUnmanagedObjects();

            m_enabled &= _createEffect(m_terrainCast, "shaders/terrain/terrain_shadow_caster.fx");
            m_enabled &= _createEffect(m_fillScreenMap, "shaders/shadow/dynamic_resolve.fx");

            //-- debug mode
            if (m_debugMode) {
                m_enabled &= _createEffect(m_debugEffectMaterial, "shaders/shadow/debug_map_usage_terrain.fx");
            }
            else {
                m_debugEffectMaterial = NULL;
            }

            if (!m_enabled) {
                enable(false);
            }

            return m_enabled;
        }

        //-- disable
        if (!isEnabled && m_enabled) {
            m_terrainCast = NULL;
            m_fillScreenMap = NULL;

            m_debugEffectMaterial = NULL;

            deleteUnmanagedObjects();

            m_enabled = false;
            return true;
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------

    bool DynamicShadow::recreateForD3DExDevice() const
    {
        return true;
    }

    //----------------------------------------------------------------------------------------------

    void DynamicShadow::createUnmanagedObjects()
    {
        BW_GUARD;

        if (!m_enabled)
            return;

        m_splitPlanes.resize(m_shadowQuality.m_numSplit + 1);
        m_textureMatrices.resize(m_shadowQuality.m_numSplit);

        D3DFORMAT format = D3DFMT_R32F; //D3DFMT_A8R8G8B8;

        float wh = 1.f / m_shadowQuality.m_numSplit;
        for (uint i = 0; i < m_shadowQuality.m_numSplit; ++i)
        {
            m_atlasLayout[i] = AtlasRect(wh * i, 0.0f, wh, 1.0f); // 0
        }

        //-- resources (main)
        m_enabled &= _createRenderTarget(
            m_dynamicShadowMapAtlas,
            m_dynamicShadowMapSize * m_shadowQuality.m_numSplit,
            m_dynamicShadowMapSize,
            format,
            "DynamicShadowMapAtlas"
        );

        m_enabled &= createNoiseMap();

        //-- finish

        m_unmanagedObjectsCreated = true;
    }

    //----------------------------------------------------------------------------------------------

    void DynamicShadow::deleteUnmanagedObjects()
    {
        m_dynamicShadowMapAtlas = NULL;
        m_unmanagedObjectsCreated = false;
    }

    //----------------------------------------------------------------------------------------------

    void DynamicShadow::castShadows(Moo::DrawContext& shadowDrawContext)
    {
        BW_GUARD_PROFILER(DynamicShadow_castShadows);
        GPU_PROFILER_SCOPE(DynamicShadow_castShadows);

        BW_SCOPED_DOG_WATCHER("cast dynamic shadows");
        BW_SCOPED_RENDER_PERF_MARKER("Cast Dynamic Shadows");

        static DogWatch shadowsCastDogWatch("DynamicShadowsCast");
        ScopedDogWatch sdw(shadowsCastDogWatch);

        g_currentNumSplit = m_shadowQuality.m_numSplit;
        g_currentDrawChunkItems = 0;

        if (!m_enabled || !g_enableCastPass)
            return;

        const Moo::DrawcallProfilingData& profilingData = Moo::rc().liveProfilingData();

        uint32 nDrawCalls = profilingData.nDrawcalls_;
        uint32 nDrawPrimitives = profilingData.nPrimitives_;

        Vector3 lightDir = _retriveLightDir();

        // bug when loading hangar
        if (abs(lightDir.x) < 0.001 &&
            abs(lightDir.y) < 0.001 &&
            abs(lightDir.z) < 0.001) return;
        if (lightDir.length() < 0.001) return;

        Moo::rc().dynamicShadowsScene(true);

        Moo::Camera mainCamera = Moo::rc().camera();
        Matrix mainCameraViewMat = Moo::rc().view();
        Matrix mainCameraProjMat = Moo::rc().projection();
        float zoomFactor = Moo::rc().zoomFactor();

        // We get the distance to the different split screens.
        calcSplitDistances(&m_splitPlanes[0], m_shadowQuality.m_numSplit, mainCamera.nearPlane(), m_viewDist, g_splitSchemeLambda);

        //-- DYNAMIC
        Matrix lightViewMatrices[BW_SHADOWS_MAX_SPLIT_COUNT];
        Matrix lightProjMatrices[BW_SHADOWS_MAX_SPLIT_COUNT];

        if (m_dynamicShadowMapAtlas->push())
        {
            static DogWatch dynamicDogWatches[8] = {
                "split1",
                "split2",
                "split3",
                "split4",
                "split5",
                "split6",
                "split7",
                "split8"
            };

            shadowDrawContext.resetStatistics();
            // Produce culling objects from the camera splits.
            for (size_t i = 0; i < m_shadowQuality.m_numSplit; ++i)
            {
                ScopedDogWatch sdw(dynamicDogWatches[i]);

                Matrix view;
                Matrix proj;
                Matrix cullViewProj;

                float np = m_splitPlanes[i];
                float fp = m_splitPlanes[i + 1];

                //----------------------------------------------------------------------------------

                DX::Viewport oldViewport;
                DX::Viewport newViewport;

                Moo::rc().getViewport(&oldViewport);

                newViewport.X = DWORD(m_dynamicShadowMapSize * i);
                newViewport.Y = 0;
                newViewport.Width = m_dynamicShadowMapSize;
                newViewport.Height = m_dynamicShadowMapSize;

                newViewport.MinZ = oldViewport.MinZ; //-- reconsider
                newViewport.MaxZ = oldViewport.MaxZ; //-- reconsider

                Moo::rc().setViewport(&newViewport);

                //----------------------------------------------------------------------------------

                if (_calculateLightMatrices(view, proj, m_textureMatrices[i], cullViewProj, mainCamera,
                    mainCameraViewMat, lightDir, g_lightCamDistDynamic,
                    np, fp))
                {
                    float offsetX = m_atlasLayout[i].x;
                    float offsetY = m_atlasLayout[i].y;
                    float scaleX = m_atlasLayout[i].w;
                    float scaleY = m_atlasLayout[i].h;

                    //D3DXMATRIX rectFitMatrix = D3DXMATRIX(
                    //  scaleX,  0.0f,    0.0f, 0.0f,
                    //  0.0f,    scaleY,  0.0f, 0.0f,
                    //  0.0f,    0.0f,    1.0f, 0.0f,
                    //  offsetX, offsetY, 0.0f, 1.0f
                    //  );

                    Matrix rectFitMatrix = Matrix
                    (
                        Vector4(scaleX, 0.0f, 0.0f, 0.0f),
                        Vector4(0.0f, scaleY, 0.0f, 0.0f),
                        Vector4(0.0f, 0.0f, 1.0f, 0.0f),
                        Vector4(offsetX, offsetY, 0.0f, 1.0f)
                    );

                    m_textureMatrices[i].postMultiply(rectFitMatrix);

                    //------------------------------------------------------------------------------

                    lightViewMatrices[i] = view;
                    lightProjMatrices[i] = proj;

                    //------------------------------------------------------------------------------

                    uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
                    if (Moo::rc().stencilAvailable())
                    {
                        clearFlags |= D3DCLEAR_STENCIL;
                    }
                    Moo::rc().device()->Clear(0, NULL, clearFlags, 0xFFFFFFFF, 1.f, 0);

                    // Here draw all objects that fit us at a distance.
                    _castObjects(shadowDrawContext, mainCameraViewMat, view, proj, cullViewProj,
                        SHADOW_TYPE_DYNAMIC, ITEM_TYPE_EMBODIMENT | ITEM_TYPE_MODEL | ITEM_TYPE_TREE,
                        m_terrainCast, m_shadowQuality.m_castMask);

                    //------------------------------------------------------------------------------
                }
                else
                {
                    uint32 clearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
                    if (Moo::rc().stencilAvailable())
                    {
                        clearFlags |= D3DCLEAR_STENCIL;
                    }
                    Moo::rc().device()->Clear(0, NULL, clearFlags, 0xFF0000FF, 1.f, 0);
                }

                Moo::rc().setViewport(&oldViewport);
            }

            m_dynamicShadowMapAtlas->pop();
        }

        Moo::rc().dynamicShadowsScene(false);

        //-- Update Statistic

        g_numDrawCalls = profilingData.nDrawcalls_ - nDrawCalls;
        g_numDrawPrimitives = profilingData.nPrimitives_ - nDrawPrimitives;
    }

    //----------------------------------------------------------------------------------------------

    void DynamicShadow::receiveShadows()
    {
        BW_GUARD;
        BW_SCOPED_DOG_WATCHER("receive dynamic shadows");
        BW_SCOPED_RENDER_PERF_MARKER("Receive Dynamic Shadows");

        if (!m_enabled || !g_enableReceivePass)
            return;

        if (!m_fillScreenMap ||
            !m_fillScreenMap->pEffect() ||
            !m_fillScreenMap->pEffect()->pEffect()
            )
        {
            return;
        }

        ID3DXEffect* effect = m_fillScreenMap->pEffect()->pEffect();
        {
            effect->SetFloat("g_dynamicShadowMapResolution", (float)m_dynamicShadowMapSize);
            effect->SetMatrixArray("g_textureMatrices", &m_textureMatrices[0], m_shadowQuality.m_numSplit);
            effect->SetFloatArray("g_splitPlanesArray", &m_splitPlanes[1], m_shadowQuality.m_numSplit);
            effect->SetTexture("g_shadowMapAtlas", m_dynamicShadowMapAtlas->pTexture());
            effect->SetInt("g_shadowQuality", m_shadowQuality.m_blurQuality);
            effect->SetFloat("g_splitCount", (float)m_shadowQuality.m_numSplit);
            effect->SetTexture("g_noiseMapShadow", m_pNoiseMap.pComObject());

            effect->CommitChanges();
        }

        Moo::rc().fsQuad().draw(*m_fillScreenMap.get());
    }

    // 
    bool DynamicShadow::createNoiseMap()
    {

        ComObjectWrap< DX::Texture > pNoiseMap = Moo::rc().createTexture(
            256, 256, 1, 0, D3DFMT_G32R32F, D3DPOOL_MANAGED, "DynamicShadow/NoiseMap");

        if (!pNoiseMap.hasComObject())
            return false;

        HRESULT hr = S_OK;

        ComObjectWrap< DX::Texture > pLockTex;
        if (Moo::rc().usingD3DDeviceEx())
        {
            hr = Moo::rc().device()->CreateTexture(
                256, 256, 1, 0, D3DFMT_G32R32F, D3DPOOL_SYSTEMMEM, &pLockTex, NULL);
            if (FAILED(hr))
                return false;
        }
        else
            pLockTex = pNoiseMap;

        D3DLOCKED_RECT lockedRect;
        hr = pLockTex->LockRect(0, &lockedRect, NULL, 0);
        if (FAILED(hr))
        {
            return false;
        }
        else
        {
            Vector2* ptr = reinterpret_cast<Vector2*>(lockedRect.pBits);
            BWRandom random(0); // always use the same seed.

            //-- create bitwise mask.
            for (uint i = 0; i < 256; ++i)
            {
                for (uint j = 0; j < 256; ++j)
                {
                    float angle = random(0.f, MATH_PI);

                    ptr->x = cos(angle);
                    ptr->y = sin(angle);

                    ++ptr;
                }
            }

            pLockTex->UnlockRect(0);
        }

        if (Moo::rc().usingD3DDeviceEx())
        {
            Moo::rc().device()->UpdateTexture(pLockTex.pComObject(), pNoiseMap.pComObject());
        }

        m_pNoiseMap = pNoiseMap;
        return true;
    }

    //------------------------------------------
    void DynamicShadow::drawDebugInfo()
    {
        BW_GUARD;

        if (!m_enabled
            || !m_debugMode
            || !m_unmanagedObjectsCreated) return;

        // start position
        Vector2 topLeft(0, Moo::rc().screenHeight() - 150.f);
        Vector2 bottom = topLeft;

        bottom.y += 150.f;
        bottom.x += 150.f * m_shadowQuality.m_numSplit;

        // _drawDebugTexturedRect( topLeft, bottom, m_noiseMap->pTexture() );
        _drawDebugTexturedRect(topLeft, bottom, m_dynamicShadowMapAtlas->pTexture());
    }

    //------------------------------------------
    void DynamicShadow::setNumSplit(uint numSplit)
    {
        if (m_shadowQuality.m_numSplit == numSplit || numSplit > BW_SHADOWS_MAX_SPLIT_COUNT)
            return;

        deleteUnmanagedObjects();
        m_shadowQuality.m_numSplit = numSplit;
        createUnmanagedObjects();
    }

    //------------------------------------------
    uint DynamicShadow::getNumSplit() const
    {
        return m_shadowQuality.m_numSplit;
    }

}

BW_END_NAMESPACE
