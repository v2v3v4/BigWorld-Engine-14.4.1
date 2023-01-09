/***
 *

THE SPEEDTREE INTEGRATION MODULE
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

The files, functions and data structures listed below (*) 
implement the core functionality of the SpeedTree integration 
module. Understanding what they do will provide insight on the
design behind this module. Please refer to their inlined doxigen 
documentation for detailed information of their inner workings.

(*) If you are trying to understand the overall design of this
    module, I suggest you to reading the documentation in the order 
    listed here.

>> Files:
    >>  speedtree_renderer_common.*pp
    >>  speedtree_render.cpp
    >>  speedtree_renderer_methods.cpp
    >>  speedtree_util.hpp
    >>  speedtree_collision.cpp
    >>  billboard_optimiser.cpp

>> Functions:
    >>  getTreeTypeObject()
    >>  doSyncInit()
    >>  computeDrawData()
    >>  drawTrees()

>> Structs
    >>  TSpeedTreeType
    >>  TreeData
    >>  LodData
    >>  DrawData

>> Possible Improvements (with comments):

    >>  Reduce number of vertex data. If you look at the fvf formats
        (speedtree_util.hpp and billboard_optimiser.?pp), you will 
        notice that there is a massive amount of data being passed
        on with each vertex. Maybe we should shave some data off 
        or compress it.   

    >>  Aggregate leaf and frond textures. There can be two beneffits
        of doing this: (1) less texture swaps between tree types and 
        (2) no need to keep the very enefficient original composite 
        map around. Problems: the aggregator, as it currently stands, 
        requires the original texture to recover from a lost device. 
        Also, a more sophisticated bathing may be required if more than
        one aggregated texture is require to store all loaded trees.

    >>  Improvements to the billboarding system. Currently, the billboard
        optimiser uses one single vertex buffer for each chunk. Although 
        this is fine for our current far distances, as the far plane 
        increases, the number of visible chunks explodes. One possible
        approach is to group chunks together into clusters, maybe in a 
        3x3 grid around the player. One easy way to prototype this idea
        would be mapping the bboptimiser objects by something other than
        the chunk address, something that would make adjacent chunks all
        fall into the same bboptimiser instance (maybe chunk coordinates
        div n, where n is the size of a cluster).

    >>  Detail maps. This is the one functionality missing from SpeedTreeRT 
        4.1. Have a chat with Adam for details.
 *
 ***/
#include "pch.hpp"

// Module Interface
#include "speedtree_renderer.hpp"
#include "speedtree_config.hpp"

// BW Tech Headers
#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"
#include "cstdmf/slot_tracker.hpp"
#include "math/boundbox.hpp"
#include "physics2/bsp.hpp"
#include "moo/render_event.hpp"

// Set DONT_USE_DUPLO to 1 to prevent all calls
// to the dublo lib. This avoids having to link
// against duplo (which, in turn, depends on a
// lot of other libs). It is useful for testing.
#define DONT_USE_DUPLO 0

DECLARE_DEBUG_COMPONENT2("SpeedTree", 0)

#if SPEEDTREE_SUPPORT // -------------------------------------------------------

// Speedtree Lib Headers
#include "speedtree_collision.hpp"
#include "billboard_optimiser.hpp"
#include "speedtree_util.hpp"

// BW Tech Headers
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_memory.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/data_section_census.hpp"
#include "moo/material.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "moo/texture_streaming_manager.hpp"
#include "moo/texture_usage.hpp"
#include "moo/effect_material.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/vertex_buffer_wrapper.hpp"
#include "moo/index_buffer_wrapper.hpp"
#include "moo/effect_manager.hpp"
#include "moo/resource_load_context.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/time_of_day.hpp"
#include "romp/fog_controller.hpp"
#include "moo/geometrics.hpp"
#include "moo/line_helper.hpp"
#include "moo/renderer.hpp"

#include "speedtree_tree_type.hpp"
#include "speedtree_renderer_common.hpp"

#if ENABLE_FILE_CASE_CHECKING
#include "resmgr/filename_case_checker.hpp"
#endif

// SpeedTree API
#include <SpeedTreeRT.h>
#include <SpeedTreeAllocator.h>

// DX Headers
#include <d3dx9math.h>

// STD Headers
#include <sstream>


BW_BEGIN_NAMESPACE

// Import dx wrappers
using Moo::IndexBufferWrapper;
using Moo::VertexBufferWrapper;
using Moo::BaseTexturePtr;


BW_END_NAMESPACE

#include "speedtree_renderer_util.hpp"

BW_BEGIN_NAMESPACE

//-- start unnamed namespace.
//--------------------------------------------------------------------------------------------------
namespace
{
    using namespace speedtree;

// Named contants
    const uint  c_maxTreeMaterialTypes  = 32;
    const float c_maxDTime              = 0.05f;
    static bool s_enableDrawPrim        = true;
    const AutoConfigString s_speedTreeXML("system/speedTreeXML");

    //--
    //----------------------------------------------------------------------------------------------
    struct SemitransparentTree
    {
        SemitransparentTree() : m_type(NULL), m_data(NULL) { }
        SemitransparentTree(TSpeedTreeType* type, TSpeedTreeType::DrawData* data) : m_type(type), m_data(data) { }

        bool operator < (const SemitransparentTree& right) const
        {
            return m_data->leavesAlpha_ > right.m_data->leavesAlpha_;
        }

        TSpeedTreeType * m_type;
        TSpeedTreeType::DrawData * m_data;
    };
    typedef BW::vector<SemitransparentTree> SemitransparentTrees;


    //----------------------------------------------------------------------------------------------
    class SpeedTreeMaterialsMapConstant : public Moo::EffectConstantValue
    {
    public:
        SpeedTreeMaterialsMapConstant()
        {
            m_texture = Moo::rc().createTexture(
                2, c_maxTreeMaterialTypes * 4, 1, 0, D3DFMT_A8R8G8B8, 
                D3DPOOL_MANAGED, "Speedtree/MaterialsMap" );

            MF_ASSERT( m_texture.hasComObject() );
        }

        //------------------------------------------------------------------------------------------
        void update()
        {
            BW_GUARD;

            //-- 1. update tree materials list.
            //--    Note: Here we make assumption that 32 unique types per space is more than enough.
            uint8 materials[c_maxTreeMaterialTypes * 4][8];
            {
                //-- Note: make the type coefficient black to see errors (if we exceed available
                //--       materials count) as short as possible.
                memset(materials, 0, c_maxTreeMaterialTypes * 4 * 8 * sizeof(uint8));

                for (
                    TSpeedTreeType::TreeTypesMap::const_iterator it = TSpeedTreeType::s_typesMap_.begin();
                    it != TSpeedTreeType::s_typesMap_.end();
                    ++it
                    )
                {
                    uint                idx = it->second->materialHandle_;
                    const CSpeedTreeRT& spt = *it->second->speedTree_.get();

                    if (idx >= c_maxTreeMaterialTypes)
                    {
                        WARNING_MSG("Too many unique tree types. Current count of unique types %d",
                            TSpeedTreeType::s_typesMap_.size()
                            );
                        idx = (c_maxTreeMaterialTypes - 1);
                    }

                    const float* const sptMats[] = 
                    {
                        spt.GetBranchMaterial(),
                        spt.GetFrondMaterial(),
                        spt.GetLeafMaterial(),
                        spt.GetLeafMaterial()
                    };

                    const float lightAdj = spt.GetLeafLightingAdjustment();

                    for (uint i = 0; i < 4; ++i)
                    {
                        //-- diffuse.
                        materials[idx * 4 + i][0] = static_cast<uint8>(sptMats[i][0] * 255);
                        materials[idx * 4 + i][1] = static_cast<uint8>(sptMats[i][1] * 255);
                        materials[idx * 4 + i][2] = static_cast<uint8>(sptMats[i][2] * 255);
                        materials[idx * 4 + i][3] = (i > 1) ? static_cast<uint8>(lightAdj * 255) : 0;

                        //-- ambient.
                        materials[idx * 4 + i][4] = static_cast<uint8>(sptMats[i][3] * 255);
                        materials[idx * 4 + i][5] = static_cast<uint8>(sptMats[i][4] * 255);
                        materials[idx * 4 + i][6] = static_cast<uint8>(sptMats[i][5] * 255);
                        materials[idx * 4 + i][7] = 0;

                        //-- debug.
#if 0
                        const char* const g_treePartName[] =
                        {
                            "branch", "frond", "leaf", "billboard"
                        };

                        NOTICE_MSG("SpeedTree name <%s> part <%s> material diffuse - (%f,%f,%f), ambient - (%f,%f,%f), light Adj - (%f) \n",
                            it->first.c_str(), g_treePartName[i], sptMats[i][0], sptMats[i][1], sptMats[i][2],
                            sptMats[i][3], sptMats[i][4], sptMats[i][5], (i > 1) ? lightAdj : 0.0f
                            );
#endif

                    }
                }
            }
            
            //-- 2. upload regenerated data into texture.
            {
                ComObjectWrap< DX::Texture > pLockTex;
                if ( Moo::rc().usingD3DDeviceEx() )
                {
                    D3DSURFACE_DESC sd;
                    m_texture->GetLevelDesc( 0, &sd );

                    pLockTex = Moo::rc().createTexture( sd.Width, sd.Height, m_texture->GetLevelCount(),
                        0, sd.Format, D3DPOOL_SYSTEMMEM );
                }
                else pLockTex = m_texture;


                D3DLOCKED_RECT lockedRect;
                if (SUCCEEDED(pLockTex->LockRect(0, &lockedRect, NULL, 0)))
                {
                    uint8* ptr = reinterpret_cast<uint8*>(lockedRect.pBits);

                    for (uint i = 0; i < 128; ++i)
                    {
                        for (uint j = 0; j < 8; ++j)
                        {
                            *(ptr++) = materials[i][j];
                        }

                        ptr += lockedRect.Pitch - sizeof(uint8) * 8;
                    }
                    pLockTex->UnlockRect(0);
                }

                if ( Moo::rc().usingD3DDeviceEx() )
                    Moo::rc().device()->UpdateTexture( pLockTex.pComObject(), m_texture.pComObject() );
            }
        }

        //-- interface Moo::EffectConstantValue
        //------------------------------------------------------------------------------------------
        bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
        {
            HRESULT hr = pEffect->SetTexture(constantHandle, m_texture.pComObject());
            return SUCCEEDED(hr);
        }

    private:
        ComObjectWrap<DX::Texture> m_texture;   
    };
    SpeedTreeMaterialsMapConstant* g_speedTreeMaterialsConstant = NULL;

    //-- looks slightly ugly because we lose auto life-time control. But it's lesser of two evils.
    //-- Initialization of the smart pointer performed in SpeedTreeRenderer::init and reseting in fini().
    std::auto_ptr<speedtree::SpeedTreeRendererCommon> g_commonRenderData;
}
//--------------------------------------------------------------------------------------------------
//-- end unnamed namespace.

namespace speedtree
{

// Do we want to render depth only passes?
Moo::ERenderingPassType SpeedTreeRenderer::s_curRenderingPass_ = Moo::RENDERING_PASS_COLOR;
MatrixProviderPtr       SpeedTreeRenderer::s_pTarget;
Vector2                 SpeedTreeRenderer::s_vTargetScreenPos;

Vector3                 SpeedTreeRenderer::s_vCamPos;
Vector3                 SpeedTreeRenderer::s_vTargetPos;
float                   SpeedTreeRenderer::s_fToTargetDistance;
Vector2                 SpeedTreeRenderer::s_vCamFwd2;
#ifdef EDITOR_ENABLED
bool                    SpeedTreeRenderer::s_paused;
#endif

bool                    SpeedTreeRenderer::s_enableTransparency = true;

bool                    SpeedTreeRenderer::s_hideClosest = false;
float                   SpeedTreeRenderer::s_sqrHideRadius = 0.0f;
float                   SpeedTreeRenderer::s_hideRadius = 0.0f;
float                   SpeedTreeRenderer::s_hideRadiusAlpha = 0.0f;
float                   SpeedTreeRenderer::s_hiddenLeavesAlpha = 0.4f;
float                   SpeedTreeRenderer::s_hiddenBranchAlpha = 0.7f;

#if ENABLE_RESOURCE_COUNTERS

//--------------------------------------------------------------------------------------------------
class MFSpeedTreeAllocator: public CSpeedTreeAllocator
{
    // Store a header at the start of each allocation to report amount freed
    // and for tracking original pointer while returning aligned pointers.
    struct Header
    {
        size_t size_;   // size not including header
        void * base_;   // base pointer for freeing
    };

    virtual void* Alloc(size_t BlockSize, size_t Alignment = 16)
    {
            char * pAllocated = new char[ BlockSize + sizeof( Header ) + Alignment - 1 ];
            char * p = pAllocated;
            p += sizeof( Header );
    
            void * pAligned = (char*)( ( (uintptr_t)p + Alignment - 1 ) & ~(Alignment - 1) );
    
            Header * header = (Header*)pAligned - 1;
            header->size_ = BlockSize + Alignment - 1; 
            header->base_ = pAllocated;
    
            RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("Speedtree/Tree", 
                (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
                header->size_, 0 )
            return pAligned;
    }
    virtual void Free(void* pBlock)
    {
        Header * header = (Header*)pBlock - 1;
        size_t size = header->size_;
        delete[] header->base_;

        RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("Speedtree/Tree", 
            (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
            size, 0 )
    }
};

//--------------------------------------------------------------------------------------------------
class MFSpeedTreeAllocatorSetter
{
public:
    MFSpeedTreeAllocatorSetter()
    {
        static MFSpeedTreeAllocator allocator;
        CSpeedTreeRT::SetAllocator( &allocator );
    }
}
MFSpeedTreeAllocatorSetter;

#endif //-- ENABLE_RESOURCE_COUNTERS

#if ENABLE_WATCHERS
bool SpeedTreeRenderer::isVisible = true;
#endif

//--------------------------------------------------------------------------------------------------
SpeedTreeRenderer::SpeedTreeRenderer() :
    treeType_(NULL),
    bbOptimiser_(NULL),
    treeID_(-1),
    windOffset_(0.0f)
{
    BW_GUARD;
    windOffset_ = float(int(10.0f * (float(rand( )) / RAND_MAX)));
    ++TSpeedTreeType::s_totalCount_;

#if ENABLE_WATCHERS
    static bool s_isWatcherAdded = false;
    if (!s_isWatcherAdded)
    {
        s_isWatcherAdded = true;
        MF_WATCH("Visibility/Speed_Tree", isVisible, Watcher::WT_READ_WRITE, "Is speed tree visible." );
    }
#endif

}

//--------------------------------------------------------------------------------------------------
SpeedTreeRenderer::~SpeedTreeRenderer()
{
    BW_GUARD;
    --TSpeedTreeType::s_totalCount_;

#if ENABLE_BB_OPTIMISER
    // try to release bb optimizer
    if (this->bbOptimiser_.exists())
    {
        this->resetTransform( Matrix::identity, false );
        this->bbOptimiser_ = NULL;
    }
#endif
}

//-- Initializes the speedtree module. 
//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::init()
{
    CSpeedTreeRT::Authorize("D6A8D469CB84DA91");

    if ( !CSpeedTreeRT::IsAuthorized() )
    {
        ERROR_MSG("SpeedTree authorisation failed\n");
    }

    DataSectionPtr section = BWResource::instance().openSection(s_speedTreeXML);

    if ( !section.exists() )
    {
        CRITICAL_MSG("SpeedTree settings file not found");
        return;
    }

    CSpeedTreeRT::SetNumWindMatrices( 4 );
    CSpeedTreeRT::SetDropToBillboard( true );

    //-- ToDo: currently BillboardOptimiser::init depends on SpeedTreeRendererCommon. Fix this.
#if ENABLE_BB_OPTIMISER
    // init billboard optimizer.
    BillboardOptimiser::init(section);
#endif

#if ENABLE_BB_OPTIMISER
    TSpeedTreeType::s_optimiseBillboards_ = section->readBool(
        "billboardOptimiser/enabled", TSpeedTreeType::s_optimiseBillboards_ );
    MF_WATCH("SpeedTree/Optimise billboards",
        TSpeedTreeType::s_optimiseBillboards_, Watcher::WT_READ_WRITE,
        "Toggles billboards optimisations on/off (requires batching to be on)" );
#endif

    //-- initialize common rendering data.
    g_commonRenderData.reset(new SpeedTreeRendererCommon(section));

    //-- register shared auto-constant.
    {
        g_speedTreeMaterialsConstant = new SpeedTreeMaterialsMapConstant;

        Moo::rc().effectVisualContext().registerAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "SpeedTreeMaterials", g_speedTreeMaterialsConstant
            );
    }

    MF_WATCH("SpeedTree/Envirominder Lighting",
        TSpeedTreeType::s_enviroMinderLight_, Watcher::WT_READ_WRITE,
        "Toggles use of envirominder as the source for lighting "
        "information, instead of Moo's light container.");

    MF_WATCH("SpeedTree/Bounding boxes",
        TSpeedTreeType::s_drawBoundingBox_, Watcher::WT_READ_WRITE,
        "Toggles rendering of trees' bounding boxes on/off");

    MF_WATCH("SpeedTree/enable culling",
        TSpeedTreeType::s_enableCulling_, Watcher::WT_READ_WRITE,
        "Toggles culling of trees on/off");

    MF_WATCH("SpeedTree/enable instancing",
        TSpeedTreeType::s_enableInstaning_, Watcher::WT_READ_WRITE,
        "Toggles instancing of trees on/off");

    MF_WATCH("SpeedTree/enable z-pre pass",
        TSpeedTreeType::s_enableZPrePass_, Watcher::WT_READ_WRITE,
        "Toggles z-pre pass of trees on/off to minimize overdraw effect");

    MF_WATCH("SpeedTree/enable z-distance sorting",
        TSpeedTreeType::s_enableZDistSorting_, Watcher::WT_READ_WRITE,
        "Toggles z-distance sorting of trees on/off to minimize overdraw effect");

    MF_WATCH("SpeedTree/Draw trees",
        TSpeedTreeType::s_drawTrees_, Watcher::WT_READ_WRITE,
        "Toggles speedtree rendering on/off");

    MF_WATCH("SpeedTree/Draw leaves",
        TSpeedTreeType::s_drawLeaves_, Watcher::WT_READ_WRITE,
        "Toggles rendering of leaves on/off");

    MF_WATCH("SpeedTree/Draw branches",
        TSpeedTreeType::s_drawBranches_, Watcher::WT_READ_WRITE,
        "Toggles rendering of branches on/off");

    MF_WATCH("SpeedTree/Draw fronds",
        TSpeedTreeType::s_drawFronds_, Watcher::WT_READ_WRITE,
        "Toggles rendering of fronds on/off");

    MF_WATCH("SpeedTree/Draw billboards",
        TSpeedTreeType::s_drawBillboards_, Watcher::WT_READ_WRITE,
        "Toggles rendering of billboards on/off");

    MF_WATCH("SpeedTree/Play animation",
        TSpeedTreeType::s_playAnimation_, Watcher::WT_READ_WRITE,
        "Toggles wind animation on/off");

    TSpeedTreeType::s_leafRockFar_ = section->readFloat(
        "leafRockFar", TSpeedTreeType::s_leafRockFar_ );
    MF_WATCH("SpeedTree/Leaf Rock Far Plane",
        TSpeedTreeType::s_leafRockFar_, Watcher::WT_READ_WRITE,
        "Sets the maximun distance to which "
        "leaves will still rock and rustle" );

    TSpeedTreeType::s_lodMode_ = section->readFloat(
        "lodMode", TSpeedTreeType::s_lodMode_ );
    MF_WATCH("SpeedTree/LOD Mode", TSpeedTreeType::s_lodMode_, Watcher::WT_READ_WRITE,
        "LOD Mode: -1: dynamic (near and far LOD defined per tree);  "
        "-2: dynamic (use global near and far LOD values);  "
        "[0.0 - 1.0]: force static." );

    TSpeedTreeType::s_lodNear_ = section->readFloat(
        "lodNear", TSpeedTreeType::s_lodNear_ );
    MF_WATCH("SpeedTree/LOD near",
        TSpeedTreeType::s_lodNear_, Watcher::WT_READ_WRITE,
        "Global LOD near value (distance for maximum level of detail");

    TSpeedTreeType::s_lodFar_ = section->readFloat(
        "lodFar", TSpeedTreeType::s_lodFar_ );
    MF_WATCH("SpeedTree/LOD far",
        TSpeedTreeType::s_lodFar_, Watcher::WT_READ_WRITE,
        "Global LOD far value (distance for minimum level of detail)");

    TSpeedTreeType::s_lod0Yardstick_ = section->readFloat(
        "lod0Yardstick", TSpeedTreeType::s_lod0Yardstick_ );
    MF_WATCH("SpeedTree/LOD-0 Yardstick",
        TSpeedTreeType::s_lod0Yardstick_, Watcher::WT_READ_WRITE,
        "Trees taller than the yardstick will LOD to billboard farther than LodFar. "
        "Trees shorter than the yardstick will LOD to billboard closer than LodFar.");

    TSpeedTreeType::s_lod0Variance_ = section->readFloat(
        "lod0Variance", TSpeedTreeType::s_lod0Variance_ );
    MF_WATCH("SpeedTree/LOD-0 Variance",
        TSpeedTreeType::s_lod0Variance_, Watcher::WT_READ_WRITE,
        "How much will the lod level vary depending on the size of the tree" );

    MF_WATCH("SpeedTree/Counters/Unique",
        TSpeedTreeType::s_uniqueCount_, Watcher::WT_READ_ONLY,
        "Counter: number of unique tree models currently loaded" );

    MF_WATCH("SpeedTree/Counters/Species",
        TSpeedTreeType::s_speciesCount_, Watcher::WT_READ_ONLY,
        "Counter: number of speed tree files currently loaded" );

    MF_WATCH("SpeedTree/Counters/Visible",
        TSpeedTreeType::s_visibleCount_, Watcher::WT_READ_ONLY,
        "Counter: number of tree currently visible in the scene" );

    MF_WATCH("SpeedTree/Counters/Instances",
        TSpeedTreeType::s_totalCount_, Watcher::WT_READ_ONLY,
        "Counter: total number of trees instantiated" );

    TSpeedTreeType::s_speedWindFile_ = section->readString(
        "speedWindINI", TSpeedTreeType::s_speedWindFile_ );

    MF_WATCH( "Render/Performance/DrawPrim SpeedTreeRenderer", s_enableDrawPrim,
        Watcher::WT_READ_WRITE,
        "Allow SpeedTreeRenderer to call drawIndexedPrimitive()." );

    MF_WATCH("SpeedTree/BB mipmap bias", 
        BillboardOptimiser::s_mipBias_, Watcher::WT_READ_WRITE, 
        "Amount to offset billboard mip map lod level." );

    MF_WATCH( "SpeedTree/Hidden Leaves Alpha",
        s_hiddenLeavesAlpha, Watcher::WT_READ_WRITE,
        "Alpha of hidden leaves" );

    MF_WATCH( "SpeedTree/Hidden Branch Alpha",
        s_hiddenBranchAlpha, Watcher::WT_READ_WRITE,
        "Alpha of hidden branches" );

    TSpeedTreeType::loadBushNames();
}

/**
 *  Finilises the speedtree module.
 */
void SpeedTreeRenderer::fini()
{
    BW_GUARD;
    CommonTraits::s_vertexBuffer_ = NULL;
    CommonTraits::s_indexBuffer_ = NULL;
    LeafTraits::s_vertexBuffer_ = NULL;
    LeafTraits::s_indexBuffer_ = NULL;
    BillboardTraits::s_vertexBuffer_ = NULL;
    BillboardTraits::s_indexBuffer_ = NULL;

    TSpeedTreeType::fini();

#if ENABLE_BB_OPTIMISER
    BillboardOptimiser::fini();
#endif

    //-- unregister shared auto-constant.
    {
        g_speedTreeMaterialsConstant = NULL;

        Moo::rc().effectVisualContext().unregisterAutoConstant(
            Moo::EffectVisualContext::AUTO_CONSTANT_TYPE_PER_FRAME, "SpeedTreeMaterials"
            );
    }

    //-- clear common render data.
    g_commonRenderData.reset();
}

/**
 *  Loads a config for each unique space.
 */
void SpeedTreeRenderer::loadSpaceCfg(const DataSectionPtr& spaceCfg)
{
    DataSectionPtr cfg = spaceCfg;
    if (!cfg)
    {
        cfg = BWResource::instance().openSection(s_speedTreeXML);
    }

    TSpeedTreeType::s_lodNear_ = cfg->readFloat("lodNear", TSpeedTreeType::s_lodNear_ );
    TSpeedTreeType::s_lodFar_  = cfg->readFloat("lodFar", TSpeedTreeType::s_lodFar_ );
}

/**
 *  Loads a SPT file.
 *
 *  @param  filename    name of SPT file to load.
 *  @param  seed        seed number to use when generating tree geometry.
 *
 *  @note               will throw std::runtime_error if file can't be loaded.
 */
void SpeedTreeRenderer::load(const char* filename, uint seed, const Matrix& world)
{
    BW_GUARD;
    Moo::ScopedResourceLoadContext resLoadCtx( BWResource::getFilename( filename ) );

    this->treeType_ = TSpeedTreeType::getTreeTypeObject( filename, seed );
    this->resetTransform( world );
}

/**
 *  Animates trees.
 *  Should be called only once per frame.
 *
 *  @param  dTime   time elapsed since last tick.
 */
void SpeedTreeRenderer::tick( float dTime )
{
    BW_GUARD;
#ifdef EDITOR_ENABLED
    //Checking for 0.f time elapsed from previous frame. (means game is paused)
    //To generate the same animation consistently for automated testing screenshots.
    s_paused = dTime == 0.f;
#endif

#if ENABLE_WATCHERS
    if(!isVisible)
        return;
#endif

    //-- ToDo: reconsider.
    if ( s_pTarget )
    {
        Matrix mTarget;
        SpeedTreeRenderer::s_pTarget->matrix( mTarget );

        s_vCamPos = Moo::rc().invView().applyToOrigin();
        s_vTargetPos = Vector3( &mTarget.row( 3 ).x );
        s_vTargetPos.y += 1.0f;

        s_fToTargetDistance = ( s_vCamPos - s_vTargetPos ).length();

        Vector3 vCamFwd( &TSpeedTreeType::s_invView_.row( 2 ).x );
        s_vCamFwd2 = Vector2( vCamFwd.x, vCamFwd.z );

        Vector4 v = mTarget.row( 3 );
        TSpeedTreeType::s_view_.applyPoint( v, v );
        TSpeedTreeType::s_projection_.applyPoint( v, v );
        s_vTargetScreenPos = Vector2( v.x / v.w, v.y / v.w );
    }

    //--
    {
        ScopedDogWatch watcher( TSpeedTreeType::s_globalWatch_ );
        TSpeedTreeType::tick( dTime );

#if ENABLE_BB_OPTIMISER
        BillboardOptimiser::tick();
#endif
    }
    //-- ToDo: Think about moving here wind animation update.
}

/**
 *  Marks begining of new frame. No draw call can be done
 *  before this method is called.
 *  @param envMinder    pointer to the current environment minder object
 *                      (if NULL, will use constant lighting and wind).
 *
 */
void SpeedTreeRenderer::beginFrame(
        EnviroMinder* envMinder, Moo::ERenderingPassType type, const Matrix& lodCamera)
{
    BW_GUARD;

#if ENABLE_WATCHERS
    if(!isVisible)
        return;
#endif

    //-- save desired LOD camera.
    TSpeedTreeType::s_lodCamera_ = lodCamera;

    SpeedTreeRenderer::update();

    //-- check ready status. BigWorld shader system requires that even if shader compilation and
    //-- loading is performed in background final step must be done in the main thread. So before
    //-- using material we need check its status from the main thread.
    if (!g_commonRenderData->readyToUse())
        return;
    //-- prepare common render data for drawing with desired rendering pass.
    g_commonRenderData->pass(type, TSpeedTreeType::s_enableInstaning_);
    s_curRenderingPass_ = type;

    ++TSpeedTreeType::s_passNumCount_;
    if ( TSpeedTreeType::s_passNumCount_ == 1 )
    {
        TSpeedTreeType::s_visibleCount_ = 0;
    }
    TSpeedTreeType::s_lastPassCount_ = 0;
    
    static DogWatch clearWatch("Clear Deferred");
    ScopedDogWatch watcher1(TSpeedTreeType::s_globalWatch_);
    ScopedDogWatch watcher2(TSpeedTreeType::s_prepareWatch_);

    //-- store view and projection matrices for future use.
    TSpeedTreeType::s_view_       = Moo::rc().view();
    TSpeedTreeType::s_projection_ = Moo::rc().projection();
    TSpeedTreeType::s_viewProj_   = Moo::rc().viewProjection();
    TSpeedTreeType::s_invView_    = TSpeedTreeType::s_view_;
    TSpeedTreeType::s_invView_.invert();

    if (s_curRenderingPass_ == Moo::RENDERING_PASS_COLOR)
    {
        TSpeedTreeType::s_alphaPassViewMat = TSpeedTreeType::s_view_;
        TSpeedTreeType::s_alphaPassProjMat = TSpeedTreeType::s_projection_;
    }

    // update speed tree camera (in speedtree, z is up)
    const Vector3& pos = TSpeedTreeType::s_invView_.applyToOrigin();
    Vector3        one = TSpeedTreeType::s_invView_.applyPoint(Vector3(0,0,-1));
    Vector3        dir = one - pos;

    const float cam_pos[] = {pos.x, pos.y, -pos.z};
    const float cam_dir[] = {dir.x, dir.y, -dir.z};
    CSpeedTreeRT::SetCamera( cam_pos, cam_dir );

    TSpeedTreeType::saveWindInformation( envMinder );
    TSpeedTreeType::saveLightInformation( envMinder );

    TSpeedTreeType::s_frameStarted_ = true;
    TSpeedTreeType::s_deferredCount_ = 0;
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::flush()
{
    BW_GUARD;
    ScopedDogWatch watcher1(TSpeedTreeType::s_globalWatch_);

    if ( TSpeedTreeType::s_deferredCount_ )
    {
        ScopedDogWatch watcher(TSpeedTreeType::s_drawWatch_);
        TSpeedTreeType::drawTrees(s_curRenderingPass_);

        TSpeedTreeType::s_deferredCount_ = 0;
    }
}

/**
 *  Marks end of current frame.  No draw call can be done after
 *  this method (and before beginFrame) is called. It's at this point
 *  that all deferred trees will actually be sent down the rendering
 *  pipeline, if batch rendering is enabled (it is by default).
 */
void SpeedTreeRenderer::endFrame()
{
    BW_GUARD;

#if ENABLE_WATCHERS
    if(!isVisible)
        return;
#endif

    ScopedDogWatch watcher(TSpeedTreeType::s_globalWatch_);

    if (TSpeedTreeType::s_totalCount_ == 0)
    {
        return;
    }

    if ( TSpeedTreeType::s_frameStarted_ )
    {
        TSpeedTreeType::s_frameStarted_ = false;

        if ( TSpeedTreeType::s_lastPassCount_ > 0 )
        {
            ScopedDogWatch watcher(TSpeedTreeType::s_drawWatch_);

            TSpeedTreeType::drawTrees(s_curRenderingPass_);
            TSpeedTreeType::drawOptBillboards(s_curRenderingPass_);

            TSpeedTreeType::s_deferredCount_ = 0;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::drawSemitransparent(bool skipDrawing)
{
    BW_GUARD;

#if ENABLE_WATCHERS
    if(!isVisible)
        return;
#endif

    TSpeedTreeType::drawSemitransparentTrees(skipDrawing);
}

//----------------------------------------------------------------------------------------------
void TSpeedTreeType::drawSemitransparentTrees(bool skipDrawing)
{
    BW_GUARD;

    SemitransparentTrees trees;

    //-- iterate over the whole set of currently active tree types.
    for (TreeTypesMap::iterator it = s_typesMap_.begin(); it != s_typesMap_.end(); ++it)
    {
        TSpeedTreeType& rt = *it->second;
        DrawDataVector& dd = rt.drawDataCollector_.m_semitransparentTrees;

        //-- iterate over the whole set of instances of the current tree type.
        for (uint i = 0; i < dd.size(); ++i)
        {
            trees.push_back(SemitransparentTree(&rt, dd[i]));
        }

        //-- prepare instances container for the next frame.
        rt.drawDataCollector_.m_semitransparentTrees.clear();
    }

    //--
    if (skipDrawing)
        return;

    //-- sort semitransparent trees.
    std::sort(trees.begin(), trees.end());

    //-- update main drawing matrices.
    Moo::rc().view(s_alphaPassViewMat);
    Moo::rc().projection(s_alphaPassProjMat);
    Moo::rc().updateViewTransforms();

    //-- prepare shader constants for rendering.
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_VIEW);

    //-- write pixel usage type into stencil buffer.
    rp().setupSystemStencil(IRendererPipeline::STENCIL_USAGE_SPEEDTREE);

    //-- iterate over the whole set of currently sorted trees.
    for (SemitransparentTrees::iterator it = trees.begin(); it != trees.end(); ++it)
    {
        TSpeedTreeType & rt = *it->m_type;
        DrawData & dd = *it->m_data;

        for (uint i = 0; i < SPECIAL_DRAWING_STATE_COUNT; ++i)
        {
            ESpecialDrawingState state = static_cast<ESpecialDrawingState>(i);

            rt.customDrawLeaf(g_commonRenderData->specialDrawingState(state, TREE_PART_LEAF), dd);
            rt.customDrawFrond(g_commonRenderData->specialDrawingState(state, TREE_PART_FROND), dd);
            rt.customDrawBranch(g_commonRenderData->specialDrawingState(state, TREE_PART_BRANCH), dd);
            rt.customDrawBillBoard(g_commonRenderData->specialDrawingState(state, TREE_PART_BILLBOARD), dd);
        }
    }
    
    //-- restore stencil state.
    rp().restoreSystemStencil();
}

//--------------------------------------------------------------------------------------------------
bool SpeedTreeRenderer::tryToUpdateBBOptimizer(const Matrix& worldTransform)
{
    //-- optimized billboard
    if (TSpeedTreeType::s_optimiseBillboards_ && treeType_->bbTreeType_ != -1 && bbOptimiser_.exists())
    {
        BW_SCOPED_DOG_WATCHER("Billboards");

        //-- add deferred billboard.
        if (drawData_.lod_.billboardDraw_)
        {
            float alphaValue = drawData_.lod_.billboardFadeValue_;

            if (treeID_ != -1)
            {
                //-- update tree's alpha test value with the bb optimizer.
                bbOptimiser_->updateTreeAlpha(treeID_, alphaValue);
            }
            else
            {
                if (bbOptimiser_->isFull())
                {
                    bbOptimiser_ = BillboardOptimiser::retrieve(worldTransform.applyToOrigin());
                }

                //-- If tree have not yet been registered with the bb optimizer (and the 
                //-- billboard vertices have already been calculated), do it now.
                treeID_ = bbOptimiser_->addTree(treeType_->bbTreeType_, worldTransform, alphaValue);
            }

            return true;
        }
    }

    return false;
}

//-- Draw an instance of this tree at the given world transform
//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::draw(const Matrix& worldTransform)
{
    BW_GUARD;

#if ENABLE_WATCHERS
    if(!isVisible)
        return;
#endif

    //-- ToDo: Think about using for real rendering list of only visible tree types. Means tree types
    //--       for which at least one instance is visible from the current view point. For the real big
    //--       world this may be good optimization but for relatively small it may be not, because
    //--       almost every time we will see almost all the tree types.

    //-- skip tree rendering in case if we don't start speedtree frame. This may happen either
    //-- we didn't call beginFrame() method or some preparation in that method is not finished yet.
    //-- So we need waiting in that case.
    if (!TSpeedTreeType::s_frameStarted_)
        return;

    if (TSpeedTreeType::s_passNumCount_ == 1)
    {
        ++TSpeedTreeType::s_visibleCount_;
    }
    ++TSpeedTreeType::s_lastPassCount_;

    // quit if tree rendering is disabled.
    if (!TSpeedTreeType::s_drawTrees_)
    {
        return;
    }

    //-- set to 1 if you want to debug trees' rotation
#if 0
    Matrix world = worldTransform;
    Matrix rot;
    rot.setRotateY(TSpeedTreeType::s_time*0.3f);
    world.preMultiply(rot);
#else
    const Matrix& world = worldTransform;
#endif

    //-- visibility culling.
    if (TSpeedTreeType::s_enableCulling_)
    {
        Matrix worldViewProj = world;
        worldViewProj.postMultiply(TSpeedTreeType::s_viewProj_);

        const BoundingBox& bb = treeType_->treeData_.boundingBox_;
        bb.calculateOutcode(worldViewProj);

        if (bb.combinedOutcode())
            return;
    }

    //-- Render bounding box
    if (TSpeedTreeType::s_drawBoundingBox_)
    {
        Moo::rc().push();
        Moo::rc().world( world );
        Moo::Material::setVertexColour();
        Geometrics::wireBox(treeType_->treeData_.boundingBox_, Moo::Colour(1,0,0,0));
        Moo::rc().pop();
    }

    //-- collect tree's rendering data.
    {
        ScopedDogWatch watcher2(TSpeedTreeType::s_prepareWatch_);

#ifdef EDITOR_ENABLED
        //To generate the same animation consistently for automated testing screenshots.
        const float windOffset = s_paused ? 0.f : windOffset_;
        treeType_->computeDrawData( world, windOffset, drawData_ );
#else
        treeType_->computeDrawData( world, windOffset_, drawData_ );
#endif

        //--
        bool semitransparent = (s_curRenderingPass_ == Moo::RENDERING_PASS_COLOR) ? TSpeedTreeType::isTreeSemiTransparent(drawData_) : false;
        bool useBBOptimizer  = semitransparent ? false : tryToUpdateBBOptimizer(world);

        treeType_->collectTreeDrawData(drawData_, !useBBOptimizer, semitransparent);
    }
}

void SpeedTreeRenderer::generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup )
{
    if (!treeType_)
        return;

    treeType_->generateTextureUsage( usageGroup );
}

/**
 *  Notifies this tree that it's position has been updated. Calling this every
 *  time the transform of a tree changes is necessary because some optimizations
 *  assume the tree to be static most of the time. Calling this allows the tree
 *  to refresh all cached data that becomes obsolete whenever a change occours.
 *
 *  @param  updateBB    true if the tree should update the bb optimizer.
 *                      Default is true.
 */
void SpeedTreeRenderer::resetTransform( const Matrix& world, bool updateBB /*=true*/ )
{
    BW_GUARD;

#if ENABLE_BB_OPTIMISER
    if (treeID_ != -1)
    {
        bbOptimiser_->removeTree(treeID_);
        treeID_ = -1;
    }
    if (updateBB)
    {
        bbOptimiser_ = BillboardOptimiser::retrieve(world.applyToOrigin());
    }
#endif

    //-- mark this tree instance as needed of rebuilding drawing data.
    drawData_.initialised_ = false;
}

/**
 *  Returns tree bounding box.
 */
const BoundingBox & SpeedTreeRenderer::boundingBox() const
{
    BW_GUARD;
    return treeType_->treeData_.boundingBox_;
}

/**
 *  Returns tree BSP-Tree.
 */
const BSPTree* SpeedTreeRenderer::bsp() const
{
    BW_GUARD;
    return this->treeType_->bspTree_.get();
}

/**
 *  Returns name of SPT file used to generate this tree.
 */
const char * SpeedTreeRenderer::filename() const
{
    BW_GUARD;
    return treeType_->filename_.c_str();
}

/**
 *  Returns seed number used to generate this tree.
 */
uint SpeedTreeRenderer::seed() const
{
    BW_GUARD;
    return treeType_->seed_;
}

/**
 *  Sets the LOD mode. Returns flag state before call.
 */
float SpeedTreeRenderer::lodMode( float newValue )
{
    BW_GUARD;
    float oldValue = TSpeedTreeType::s_lodMode_;
    TSpeedTreeType::s_lodMode_ = newValue;
    return oldValue;
}

/**
 *  Sets the max LOD. Returns flag state before call.
 */
float SpeedTreeRenderer::maxLod( float newValue )
{
    BW_GUARD;
    float oldValue = TSpeedTreeType::s_maxLod_;
    TSpeedTreeType::s_maxLod_ = newValue;
    return oldValue;
}

/**
 *  Sets the envirominder as the source for lighting
 *  information, instead of the current light containers. 
 *
 *  @param  newValue    true if the environminder should be used as source of
 *                      lighing. false is Moo's light containers should be used.
 *
 *  @return             state before call.
 */
bool SpeedTreeRenderer::enviroMinderLighting( bool newValue )
{
    BW_GUARD;
    bool oldValue = TSpeedTreeType::s_enviroMinderLight_;
    TSpeedTreeType::s_enviroMinderLight_ = newValue;
    return oldValue;
}

//-- Enables or disables drawing of trees. 
//--------------------------------------------------------------------------------------------------
bool SpeedTreeRenderer::drawTrees( bool newValue )
{
    BW_GUARD;
    bool oldValue = TSpeedTreeType::s_drawTrees_;
    TSpeedTreeType::s_drawTrees_ = newValue;
    return oldValue;
}

//-- Enables or disables wind influence on trees. 
//--------------------------------------------------------------------------------------------------
bool SpeedTreeRenderer::useWind( bool useWind_ )
{
    BW_GUARD;
    bool oldValue = TSpeedTreeType::s_useMapWind;
    TSpeedTreeType::s_useMapWind = useWind_;
    return oldValue;
}


//-- Do house keeping chores. 
//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::update()
{
    BW_GUARD;

#if ENABLE_WATCHERS
    if(!isVisible)
        return;
#endif

    ScopedDogWatch watcher(TSpeedTreeType::s_globalWatch_);
    TSpeedTreeType::update();

#if ENABLE_BB_OPTIMISER
    BillboardOptimiser::update();
#endif
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::setCameraTarget( MatrixProvider *pTarget )
{
    s_pTarget = pTarget;
}

//--------------------------------------------------------------------------------------------------
TSpeedTreeType::TSpeedTreeType() :
    speedTree_( NULL ),
    pWind_( NULL ),
    treeData_(),
    seed_(1),
    filename_(),
    bspTree_(NULL),
    bbTreeType_(-1),
    refCounter_(0),
    materialHandle_(0)
{
    BW_GUARD;

    pWind_ = &s_defaultWind_;

#if ENABLE_RESOURCE_COUNTERS
    RESOURCE_COUNTER_ADD( 
        ResourceCounters::DescriptionPool("Speedtree/TSpeedTreeType",
            (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
            sizeof(*this), 0 )
#endif
}

/**
 *  Default constructor.
 */
TSpeedTreeType::~TSpeedTreeType()
{
    BW_GUARD;
#if ENABLE_RESOURCE_COUNTERS
    RESOURCE_COUNTER_SUB( 
        ResourceCounters::DescriptionPool("Speedtree/TSpeedTreeType",
            (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
            sizeof(*this), 0 )
#endif
    #if ENABLE_BB_OPTIMISER
        if ( this->bbTreeType_ != -1 )
        {
        BillboardOptimiser::delTreeType(bbTreeType_);
        }
#endif

    if (pWind_ != &TSpeedTreeType::s_defaultWind_)
    {
        delete pWind_; pWind_ = NULL;
    }

    TRACE_MSG( "deleting tree type %s\n", filename_.c_str() );
    treeData_.unload();
}

//-- Finalises the TSpeedTreeType class (static).
//--------------------------------------------------------------------------------------------------
void TSpeedTreeType::fini()
{
    BW_GUARD;
    recycleTreeTypeObjects();
    clearDXResources( true );

    if (!s_typesMap_.empty())
    {
        WARNING_MSG("TSpeedTreeType::fini: tree types still loaded: %d\n", s_typesMap_.size());
    }
}

//-- Animates trees (static).
//--------------------------------------------------------------------------------------------------
void TSpeedTreeType::tick( float dTime )
{
    BW_GUARD;
    s_visibleCount_ = 0;
    s_passNumCount_ = 0;
    s_time_ += std::min(dTime, c_maxDTime);
}

//-- Prepares trees to be rendered (static).
//--------------------------------------------------------------------------------------------------
void TSpeedTreeType::update()
{
    BW_GUARD;

    //-- ToDo: reconsider.
    if (Moo::rc().reflectionScene())
        return;

    bool initialisedATree = doSyncInit();
    bool deletedATree     = recycleTreeTypeObjects();
    if (deletedATree || initialisedATree)
    {
        s_uniqueCount_ = int(s_typesMap_.size());

        //-- regenerate tree's materials and update shared constant.
        g_speedTreeMaterialsConstant->update();
    }
}

/**
 *  Deletes tree type objects no longer being used and also clear
 *  all deferred trees for those that are still being used (static).
 */
bool TSpeedTreeType::recycleTreeTypeObjects()
{
    BW_GUARD;
    bool deletedAnyTreeType = false;
    typedef TSpeedTreeType::TreeTypesMap::iterator iterator;

    //--
    {
        SimpleMutexHolder mutexHolder(TSpeedTreeType::s_syncInitLock_);

        iterator renderIt = TSpeedTreeType::s_typesMap_.begin();
        while ( renderIt != TSpeedTreeType::s_typesMap_.end() )
        {
            if ( renderIt->second->refCount() == 0 )
            {
            s_materialHandles_.getBack(renderIt->second->materialHandle_);
            bw_safe_delete(renderIt->second);
            TSpeedTreeType::s_typesMap_.erase( renderIt++ );
            deletedAnyTreeType = true;
        }
            else
            {
                ++renderIt;
            }
        }
    }

    if (deletedAnyTreeType && TSpeedTreeType::s_typesMap_.empty())
    {
        TSpeedTreeType::clearDXResources( false );
    }

    return deletedAnyTreeType;
}

/**
 *  Do the pending synchronous initialisation on newly loaded tree type 
 *  objects (static).
 *
 *  When a new tree type is loaded, as much of the inialisation work as 
 *  possible is done from the loading thread, from the getTreeTypeObject() 
 *  function. There are some tasks, though, that can only be performed from 
 *  the rendering thread (e.g., registering a new tree type into the billboard 
 *  optimiser). 
 *
 *  For this reason, after loading a new tree-type, the getTreeTypeObject() 
 *  function will push it into the sync-init-list. The doSyncInit() function 
 *  iterates through all trees in this list, calling syncInit() on them and 
 *  moving them to the tree-types-map, where they will be ready to be rendered.
 *
 *  @return     true if at list one tree was initialised. false otherwise.
 */
bool TSpeedTreeType::doSyncInit()
{
    BW_GUARD;
    SimpleMutexHolder mutexHolder(TSpeedTreeType::s_syncInitLock_);
    const int listSize = int(TSpeedTreeType::s_syncInitList_.size());
    typedef TSpeedTreeType::InitVector::iterator iterator;
    iterator initIt  = TSpeedTreeType::s_syncInitList_.begin();
    iterator initEnd = TSpeedTreeType::s_syncInitList_.end();

    // only init one each time.
    if ( initIt != initEnd )
    {
        (*initIt)->syncInit();

        // now that initialization is finished, register renderer
        char treeDefName[BW_MAX_PATH];
        createTreeDefName( (*initIt)->filename_, (*initIt)->seed_, treeDefName,
            ARRAY_SIZE( treeDefName ) );
        (*initIt)->materialHandle_ = s_materialHandles_.getNew();
        TSpeedTreeType::s_typesMap_.insert(std::make_pair( treeDefName, *initIt ) );

        TSpeedTreeType::s_syncInitList_.erase( initIt );
    }
    return listSize > 0;
}

/**
 *  Releases all resources used by this tree type.
 */
void TSpeedTreeType::releaseResources()
{
    BW_GUARD;

    treeData_.unload();

    treeData_.branches_.lod_.clear();
    treeData_.branches_.diffuseMap_   = NULL;

    treeData_.fronds_.lod_.clear();
    treeData_.fronds_.diffuseMap_     = NULL;

    treeData_.leaves_.lod_.clear();
    treeData_.leaves_.diffuseMap_     = NULL;

    treeData_.billboards_.lod_.clear();
    treeData_.billboards_.diffuseMap_ = NULL;

#if SPT_ENABLE_NORMAL_MAPS
    treeData_.branches_.normalMap_    = NULL;
    treeData_.fronds_.normalMap_      = NULL;
    treeData_.leaves_.normalMap_      = NULL;
    treeData_.billboards_.normalMap_  = NULL;
#endif // SPT_ENABLE_NORMAL_MAPS
}

/**
 *  Deletes all tree type objects in the pending initialisation list (static).
 */
void TSpeedTreeType::clearDXResources(bool forceDelete)
{
    BW_GUARD;
    {
        SimpleMutexHolder mutexHolder(TSpeedTreeType::s_syncInitLock_);
        typedef TSpeedTreeType::InitVector::iterator iterator;

        iterator it = TSpeedTreeType::s_syncInitList_.begin();
        while ( it != TSpeedTreeType::s_syncInitList_.end() )
        {
            TSpeedTreeType* tree = *it;
            if ( tree->refCount() == 0 || forceDelete)
            {
            tree->releaseResources();
            bw_safe_delete( tree );
            it = TSpeedTreeType::s_syncInitList_.erase( it );
        }
            else
            {
                ++it;
            }
        }
    }

    if (forceDelete)
    {
        BW_SCOPED_DOG_WATCHER("release")
        for (TreeTypesMap::const_iterator it = s_typesMap_.begin(); it != s_typesMap_.end(); ++it)
        {
            it->second->releaseResources();
        }
    }
}

/**
 *  Computes the DrawData object required to render a tree of 
 *  this type at the given world transform.
 *
 *  The DrawData stores all the information required to draw a unique
 *  instance of a tree, in either immediate and batched modes.
 *
 *  This method generates a DrawData object based on this tree type
 *  and the world coordinate passed as argument.
 *
 *  LODing information can be generated in one of three ways: (1) based
 *  on the LOD information stored in each SpeedTree, (2) based on global
 *  LOD values set through <speedtree>.xml or the watchers and (3) from
 *  a constant value, also set through <speedtree>.xml or the watchers.
 * 
 *  @see    DrawData, drawRenderPass()
 *
 *  @param  world       transform where tree should be rendered.
 *  @param  o_drawData  (out) will store the defer data struct.
 */
void TSpeedTreeType::computeDrawData(const Matrix& world, float windOffset, DrawData& oDrawData)
{
    BW_GUARD;

    //-- recalculate draw data if needed.
    {
        BW_SCOPED_DOG_WATCHER("Matrices");
//      BW_PROFILER_NAMED("TSpeedTreeType::computeDrawData::Matrices");

#ifndef EDITOR_ENABLED
        if (!oDrawData.initialised_)
#endif
        {
            Quaternion  rot;
            Vector3     pos;
            Vector3     scale;
            decomposeMatrix(world, rot, pos, scale);

            oDrawData.m_instData.m_rotation.set(rot.x, rot.y, rot.z, rot.w);
            oDrawData.m_instData.m_position     = pos;
            oDrawData.m_instData.m_scale        = scale;
            oDrawData.m_instData.m_uniformScale = 0.333f * (scale.x + scale.y + scale.z);
            oDrawData.m_instData.m_windOffset   = windOffset;
        }
    }

    //-- make shortcuts used later.
    float fDistance     = 1e6;
    const float& scaleX = oDrawData.m_instData.m_scale.x;
    const float& scaleY = oDrawData.m_instData.m_scale.y;
    const float& scaleZ = oDrawData.m_instData.m_scale.z;

    float fTreeHeight = treeData_.boundingBox_.height();
    {
        BW_SCOPED_DOG_WATCHER("selecting LOD");
//      BW_PROFILER_NAMED("TSpeedTreeType::computeDrawData::selectLOD");

        //-- ToDo (b_sviglo): Try to optimize. Currently it takes about 1.5 ms when shadows is enabled.
        //--                  try to avoid some redundant calculations.
        //--                  Eliminate sqrtf() and divisions operations.

        const Vector3& eyePos   = s_lodCamera_.applyToOrigin();
        Vector3        treePos  = world.applyToOrigin();
        float          treeTop  = treePos.y + fTreeHeight;
        float          treeBase = treePos.y;

        treePos.y = Math::clamp(treeBase, eyePos.y, treeTop);

        speedTree_->SetTreePosition(treePos.x, treePos.z, treePos.y);

        //-- dynamic LOD calculation based on per tree lod parameters.
        if (s_lodMode_ == -1.0f)
        {
            float lodNear = 0;
            float lodFar  = 0;
            speedTree_->GetLodLimits( lodNear, lodFar );
            g_commonRenderData->applyLodBias(lodNear, lodFar);
            speedTree_->SetLodLimits( lodNear, lodFar );
            speedTree_->ComputeLodLevel();
        }
        //-- dynamic LOD calculation based on global lod parameters. Means it's the same for all trees.
        else if (s_lodMode_ == -2.0f)
        {
            //-- calculate lod distance.
            Vector3 distVector = treePos - eyePos;
            fDistance = g_commonRenderData->applyZoomFactor(distVector.length());
            float lodNear = s_lodNear_;
            float lodFar  = s_lodFar_;
            g_commonRenderData->applyLodBias(lodNear, lodFar);
            float lodLevel = 1.0f - (fDistance - lodNear) / (lodFar - lodNear);

            float lodVari  = s_lod0Variance_ * (-0.5f + (scaleZ * treeData_.boundingBox_.height() ) / s_lod0Yardstick_);
            lodLevel = Math::clamp(0.0f, lodLevel + lodVari, max(Moo::rc().lodZoomFactor(), 0.25f));

            //-- ToDo: reconsider.
            // Manually rounding very small lod values to avoid problems
            // in the speedtree lod calculation.(avoiding problem, not fixing)
            if (lodLevel < 0.0001f)
            {
                lodLevel = 0.0f;
            }

            speedTree_->SetLodLevel( lodLevel );
        }

        //-- manual lod parameters.
        else
        {
            speedTree_->SetLodLevel( s_lodMode_ );
        }
    }

    //-- set to 1 if you want to debug trees' lod
#if 0
    float lodLevel = 0.5f * (1+sin(TSpeedTreeType::s_time*0.3f));
    speedTree->SetLodLevel(lodLevel);
#endif
    //-- compute LOD data for current tree.
    {
        BW_SCOPED_DOG_WATCHER("LOD");
//      BW_PROFILER_NAMED("TSpeedTreeType::computeDrawData::computeLOD");

        float curLodLevel = speedTree_->GetLodLevel();
        if (oDrawData.lodLevel_ != curLodLevel)
        {
            oDrawData.lodLevel_ = curLodLevel;
            computeLodData(oDrawData.lod_);
        }
    }


    //-- now calculate transparency of trees but do that only for the main color pass.
    if (SpeedTreeRenderer::s_curRenderingPass_ == Moo::RENDERING_PASS_COLOR)
    {
        BW_SCOPED_DOG_WATCHER("Transparency");
//      BW_PROFILER_NAMED("TSpeedTreeType::computeDrawData::transparency");

        float alpha        = 1.0f;
        float sqrDistToEye = (world.applyToOrigin() - Moo::rc().invView().applyToOrigin()).lengthSquared();

        //-- if tree hiding mode enabled.
        if(SpeedTreeRenderer::s_hideClosest && sqrDistToEye < SpeedTreeRenderer::s_sqrHideRadius)
        {
            float        fDist           = sqrtf(sqrDistToEye);
            const float& hideRadius      = SpeedTreeRenderer::s_hideRadius;
            const float& hideAlphaRadius = SpeedTreeRenderer::s_hideRadiusAlpha;

            alpha = (fDist - hideAlphaRadius) / (hideRadius - hideAlphaRadius);
            alpha = max(alpha, SpeedTreeRenderer::s_hiddenLeavesAlpha);
        }

        oDrawData.leavesAlpha_ = alpha;
        oDrawData.branchAlpha_ = alpha;

        //-- if tree ...
        if (SpeedTreeRenderer::s_pTarget && SpeedTreeRenderer::s_enableTransparency)
        {
            const Vector3& vMin  = treeData_.boundingBox_.minBounds();
            const Vector3& vMax  = treeData_.boundingBox_.maxBounds();
            Vector3        vDiag = (vMax - vMin);
            float          fDiag = vDiag.length();

            if (
                !treeData_.isBush &&
                oDrawData.lod_.model3dDraw_ &&
                fTreeHeight * scaleY > 6.0f &&
                (fDistance - fDiag) < SpeedTreeRenderer::s_fToTargetDistance
                )
            {
                Matrix wInv = world;
                wInv.preTranslateBy(Vector3(0.0f, fTreeHeight * 0.5f, 0.0f));
                wInv.invert(wInv);

                Vector3 vCamInTree    = wInv.applyPoint(SpeedTreeRenderer::s_vCamPos);
                Vector3 vTargetInTree = wInv.applyPoint(SpeedTreeRenderer::s_vTargetPos);

                Vector3 vSize = vDiag * 0.5f;
                float tBig    = LineInEllipsoid(vSize * ((vSize.length() + 1.0f ) / vSize.length()), vCamInTree, vTargetInTree);

                if (tBig > 0)
                {
                    float tSmall = LineInEllipsoid(vSize, vCamInTree, vTargetInTree);

                    oDrawData.leavesAlpha_ = max(1.0f - tSmall / tBig, SpeedTreeRenderer::s_hiddenLeavesAlpha);
                    oDrawData.branchAlpha_ = max(1.0f - tSmall / tBig, SpeedTreeRenderer::s_hiddenBranchAlpha);
                }
            }
        }
    }

    //-- always update in editor.
#ifndef EDITOR_ENABLED
    oDrawData.initialised_ = true; 
#endif
}

//-- Upload per-instance render constants to the rendering device.
//--------------------------------------------------------------------------------------------------
void TSpeedTreeType::uploadInstanceConstants(
    Moo::EffectMaterial& effect, const ConstantsMap& cm, const DrawData::Instance& inst)
{
    BW_GUARD;

    //-- that is all what we need send to the shader. Only 4 float4 values and more over we send
    //-- them as one "value" object minimizing CPU overhead.
    //--     float4 rotation quaternion.
    //--     float3 translation.
    //--     float  uniform scale.
    //--     float3 scale.
    //--     float  alpha reference.
    //--     float  unique wind offset.
    //--     float  transparency amount.
    //--     float  material ID.
    //--     float  padding.
    effect.pEffect()->pEffect()->SetValue(
        cm.handle(ConstantsMap::CONST_INSTANCE), reinterpret_cast<const void*>(&inst), sizeof(DrawData::Instance)
        );
}

//--------------------------------------------------------------------------------------------------
uint TSpeedTreeType::uploadInstancingVB(
        Moo::VertexBuffer& vb, uint32 numInst, const DrawData::Instance* instances, uint& offsetInBytes)
{
    BW_GUARD;

    uint32 prevOffset  = 0;
    uint32 sizeInBytes = numInst * sizeof(DrawData::PackedGPU);

    //-- if current instancing buffer doesn't have enough memory to store rendering data, then
    //-- start recycling process.
    if ((offsetInBytes + sizeInBytes) > SpeedTreeRendererCommon::instance().maxInstBufferSizeInBytes())
    {
        offsetInBytes = 0;
    }

    MF_ASSERT(numInst);
    MF_ASSERT(sizeInBytes);

    //-- ToDo: reconsider. May be better to change vertex lock type to void* and use memcpy for memory
    //--       copying.

    DWORD lockFlag = D3DLOCK_NOOVERWRITE;
    if (offsetInBytes == 0)
    {
        lockFlag = D3DLOCK_DISCARD;
    }

    Moo::VertexLock<DrawData::PackedGPU> vl(vb, offsetInBytes, sizeInBytes, lockFlag);
    if (vl)
    {
        BW_SCOPED_DOG_WATCHER("fill instancing buffer")

        for (uint i = 0; i < numInst; ++i)
        {
            vl[i] = instances[i].packForInstancing();
        }


        prevOffset     = offsetInBytes;
        offsetInBytes += sizeInBytes;
    }

    return prevOffset;
}

/**
 *  Computes all LOD data needed to render this tree. Assumes the speedtree
 *  object have already been updated in regard to its current LOD level.
 *
 *  @param  oLod    (out) will store all lod data
 */
void TSpeedTreeType::computeLodData(LodData& oLod)
{
    BW_GUARD;
    CSpeedTreeRT::SLodValues sLod;
    speedTree_->GetLodValues(sLod);

    //-- branch
    oLod.branchLod_             = sLod.m_nBranchActiveLod;
    oLod.branchDraw_            = (sLod.m_nBranchActiveLod >= 0) && treeData_.branches_.checkVerts();
    oLod.branchAlpha_           = sLod.m_fBranchAlphaTestValue;

    //-- frond.
    oLod.frondLod_              = sLod.m_nFrondActiveLod;
    oLod.frondDraw_             = (sLod.m_nFrondActiveLod >= 0) && treeData_.fronds_.checkVerts();
    oLod.frondAlpha_            = sLod.m_fFrondAlphaTestValue;

    //-- two lods of leaves.
    oLod.leafLods_[0]           = sLod.m_anLeafActiveLods[0];
    oLod.leafLods_[1]           = sLod.m_anLeafActiveLods[1];
    oLod.leafAlphaValues_[0]    = sLod.m_afLeafAlphaTestValues[0];
    oLod.leafAlphaValues_[1]    = sLod.m_afLeafAlphaTestValues[1];
    oLod.leafLodCount_          = (sLod.m_anLeafActiveLods[0] >= 0 ? 1 : 0) + (sLod.m_anLeafActiveLods[1] >= 0 ? 1 : 0);
    oLod.leafDraw_              = oLod.leafLodCount_ > 0 && treeData_.leaves_.checkVerts();

    //-- billboards.
    oLod.billboardFadeValue_    = sLod.m_fBillboardFadeOut;
    oLod.billboardDraw_         = !treeData_.billboards_.lod_.empty() && (sLod.m_fBillboardFadeOut > 0);

    //-- recognize need we draw 3D model like branch, frond or leaf or not.
    oLod.model3dDraw_           = oLod.branchDraw_ || oLod.frondDraw_ || oLod.leafDraw_;
}

//-- Draws all deferred trees in sorted order, according to their required rendering states. 
//----------------------------------------------------------------------------------------------
void TSpeedTreeType::drawTrees(Moo::ERenderingPassType pass)
{
    BW_GUARD;
    BW_SCOPED_RENDER_PERF_MARKER("Speedtree")

    //-- 1. first update wind animations.
    {
//      BW_PROFILER_NAMED("SpeedTree::updateWind");

        for (TreeTypesMap::const_iterator it = s_typesMap_.begin(); it != s_typesMap_.end(); ++it)
        {
            //-- ToDo: check is this speedtree type need to be rendered. For example may be better
            //--       to fill currently active speedtree types.

            it->second->updateWind();

            //-- enable sorting by z-distance to decrease overall overdraw.
            if (s_enableZDistSorting_)
            {
                it->second->drawDataCollector_.sort();
            }
        }
    }

    //-- 2. prepare render context for drawing.
    prepareRenderContext();

    //-- 3. write pixel usage type into stencil buffer.
    rp().setupSystemStencil(IRendererPipeline::STENCIL_USAGE_SPEEDTREE);

    //-- 4. (optional) do z-pre-pass drawing.
    if (pass == Moo::RENDERING_PASS_COLOR && s_enableZPrePass_)
    {
        g_commonRenderData->pass(Moo::RENDERING_PASS_DEPTH, s_enableInstaning_);

        rp().setupColorWritingMask(false);
        drawLeaves(g_commonRenderData->drawingState(TREE_PART_LEAF), false);
        drawFronds(g_commonRenderData->drawingState(TREE_PART_FROND), false);
        drawBranches(g_commonRenderData->drawingState(TREE_PART_BRANCH), false);
        drawBillboards(g_commonRenderData->drawingState(TREE_PART_BILLBOARD), false);
        rp().restoreColorWritingMask();

        g_commonRenderData->pass(Moo::RENDERING_PASS_COLOR, s_enableInstaning_);
    }

    //-- 5. draw tree parts.
    drawLeaves(g_commonRenderData->drawingState(TREE_PART_LEAF));
    drawFronds(g_commonRenderData->drawingState(TREE_PART_FROND));
    drawBranches(g_commonRenderData->drawingState(TREE_PART_BRANCH));
    drawBillboards(g_commonRenderData->drawingState(TREE_PART_BILLBOARD));

    //-- 6. restore stencil state.
    rp().restoreSystemStencil();
}

/**
 *  Draw billboards using the billboards optimizer.
 */
void TSpeedTreeType::drawOptBillboards(Moo::ERenderingPassType pass)
{
    BW_GUARD;
    BW_SCOPED_RENDER_PERF_MARKER("Speedtree: billboard optimizer")

    // now, if optimization is enabled, do the billboards.
#if ENABLE_BB_OPTIMISER
    if (s_optimiseBillboards_ && s_drawBillboards_)
    {
        BW_SCOPED_DOG_WATCHER("Billboards");

        Matrix viewProj(s_view_);
        viewProj.postMultiply(s_projection_);

        BillboardOptimiser::FrameData frameData(
            viewProj, s_invView_, s_sunDirection_, s_sunDiffuse_, s_sunAmbient_, s_texturedTrees_
            );

        //-- do actual drawing and mark speedtree pixels in the stencil buffer.
        rp().setupSystemStencil(IRendererPipeline::STENCIL_USAGE_SPEEDTREE);
        BillboardOptimiser::drawAll(frameData, pass, s_enableZPrePass_);
        rp().restoreSystemStencil();
    }
#endif
}

/**
 *  Sets the global render states for drawing the trees.
 *
 *  @param  effect      the effect material to upload the constants to.
 */
void TSpeedTreeType::prepareRenderContext()
{
    BW_GUARD;

    //-- ToDo: reconsider. Try to eliminate that.
    Moo::SunLight sun;
    sun.m_ambient = s_sunAmbient_;
    sun.m_color   = s_sunDiffuse_;
    sun.m_dir     = s_sunDirection_;

    Moo::rc().effectVisualContext().sunLight(sun);

    // Set up the effect visual context.
    Moo::rc().effectVisualContext().worldMatrix( NULL );
    Moo::rc().effectVisualContext().isOutside(true);

    //-- update main drawing matrices.
    Moo::rc().view(s_view_);
    Moo::rc().projection(s_projection_);
    Moo::rc().updateViewTransforms();

    //-- prepare shader constants for rendering.
    Moo::rc().effectVisualContext().updateSharedConstants(Moo::CONSTANTS_PER_VIEW);
}

/**
 * This method updates the usageGroup with all the textures
 * that this part of the tree uses.
 */
namespace TextureStreamingHelper
{
    template < class RenderData >
    void generateTextureUsage( 
        Moo::ModelTextureUsageGroup & usageGroup,
        TSpeedTreeType& treeType, 
        RenderData& renderData,
        const Moo::VertexFormat& vertFormat,
        float (*calculateUVSpaceDensity_Callback)(TSpeedTreeType&, const Moo::VertexFormat&))
    {
        if (renderData.uvSpaceDensity_ < 0.0f)
        {
            renderData.uvSpaceDensity_ = 
                calculateUVSpaceDensity_Callback( treeType, vertFormat );
        }

        // Fetch all the textures:
        Moo::BaseTexture * pTexture;
        pTexture = renderData.diffuseMap_.get();
        if (pTexture)
        {
            usageGroup.setTextureUsage( pTexture, renderData.uvSpaceDensity_ );
        }
#if SPT_ENABLE_NORMAL_MAPS
        pTexture = renderData.normalMap_.get();
        if (pTexture)
        {
            usageGroup.setTextureUsage( pTexture, renderData.uvSpaceDensity_ );
        }
#endif
    }
}

/**
 * This method updates the usageGroup with all the textures
 * that this tree type uses.
 */
void TSpeedTreeType::generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup )
{
    SimpleMutexHolder mutexHolder( TSpeedTreeType::s_vertexListLock_ );

    // Generate usage for all the parts of the tree
    
    TextureStreamingHelper::generateTextureUsage( 
        usageGroup, 
        *this,
        treeData_.leaves_,
        g_commonRenderData->vertexFormat( TREE_PART_LEAF, false ),
        calculateUVSpaceDensityLeaves );

    TextureStreamingHelper::generateTextureUsage( 
        usageGroup, 
        *this,
        treeData_.fronds_,
        g_commonRenderData->vertexFormat( TREE_PART_FROND, false ),
        calculateUVSpaceDensityFronds );

    TextureStreamingHelper::generateTextureUsage( 
        usageGroup, 
        *this,
        treeData_.branches_,
        g_commonRenderData->vertexFormat( TREE_PART_BRANCH, false ),
        calculateUVSpaceDensityBranches );

    TextureStreamingHelper::generateTextureUsage( 
        usageGroup, 
        *this,
        treeData_.billboards_,
        g_commonRenderData->vertexFormat( TREE_PART_BILLBOARD, false ),
        calculateUVSpaceDensityBillboards );
}

/**
 *  Sets the render states for drawing the branches of this type of tree.
 */
void TSpeedTreeType::setBranchRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& cm) const
{
    BW_GUARD;

    ID3DXEffect*            pEffect = effect.pEffect()->pEffect();
    const BranchRenderData& rd      = treeData_.branches_;

    const float* branchMaterial = speedTree_->GetBranchMaterial();
    float material[] =
    {
        branchMaterial[0], branchMaterial[1], branchMaterial[2], 1.0f,
        branchMaterial[3], branchMaterial[4], branchMaterial[5], 1.0f,
    };

    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_Z_PRE_PASS), s_enableZPrePass_);
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_ALPHA_TEST_ENABLED), false );
    pEffect->SetVectorArray(cm.handle(ConstantsMap::CONST_MATERIAL), (const D3DXVECTOR4*)&material[0], 2 );
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_TEXTURED_TREES), TSpeedTreeType::s_texturedTrees_ );
    
    if (rd.diffuseMap_.exists())
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_DIFFUSE_MAP), rd.diffuseMap_->pTexture() );
    }

#if SPT_ENABLE_NORMAL_MAPS
    bool bUseNormals = rd.normalMap_.exists();
    if (bUseNormals)
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_NORMAL_MAP), rd.normalMap_->pTexture());
    }
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_NORMAL_MAP), bUseNormals );
#endif // SPT_ENABLE_NORMAL_MAPS
        
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_CULL_ENABLED), true );

    //-- setup wind matrices.
    if (pWind_)
    {
        pEffect->SetMatrixArray(
            cm.handle(ConstantsMap::CONST_WIND_MATRICES), pWind_->matrices(), pWind_->matrixCount()
            );
    }
}

/**
 *  Sets the render states for drawing the fronds of this type of tree.
 */
void TSpeedTreeType::setFrondRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& cm) const
{
    BW_GUARD;

    ID3DXEffect*          pEffect = effect.pEffect()->pEffect();
    const LeafRenderData& rd      = treeData_.leaves_;

    const float* frondMaterial = speedTree_->GetFrondMaterial();
    const float material[] =
    {
        frondMaterial[0], frondMaterial[1], frondMaterial[2], 1.0f,
        frondMaterial[3], frondMaterial[4], frondMaterial[5], 1.0f,
    };

    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_Z_PRE_PASS), s_enableZPrePass_);
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_ALPHA_TEST_ENABLED), true );
    pEffect->SetVectorArray(cm.handle(ConstantsMap::CONST_MATERIAL), (D3DXVECTOR4*)material, 2 );
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_TEXTURED_TREES), TSpeedTreeType::s_texturedTrees_ );

    if (rd.diffuseMap_.exists())
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_DIFFUSE_MAP), rd.diffuseMap_->pTexture());
    }

#if SPT_ENABLE_NORMAL_MAPS
    bool bUseNormals = rd.normalMap_.exists();
    if (bUseNormals)
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_NORMAL_MAP), rd.normalMap_->pTexture());
    }
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_NORMAL_MAP), bUseNormals);
#endif //-- SPT_ENABLE_NORMAL_MAPS

    pEffect->SetBool(cm.handle(ConstantsMap::CONST_CULL_ENABLED), false);

    //-- setup wind matrices.
    if (pWind_)
    {
        pEffect->SetMatrixArray(
            cm.handle(ConstantsMap::CONST_WIND_MATRICES), pWind_->matrices(), pWind_->matrixCount()
            );
    }
}

/**
 *  Sets the render states for drawing the leaves of this type of tree.
 */
void TSpeedTreeType::setLeafRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& cm) const
{
    BW_GUARD;

    ID3DXEffect*          pEffect = effect.pEffect()->pEffect();
    const LeafRenderData& rd      = treeData_.leaves_;

    // leaf material
    const float* leafMaterial = speedTree_->GetLeafMaterial();
    const float material[] =
    {
        leafMaterial[0], leafMaterial[1], leafMaterial[2], 1.0f,    
        leafMaterial[3], leafMaterial[4], leafMaterial[5], 1.0f,    
    };

    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_Z_PRE_PASS), s_enableZPrePass_);
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_ALPHA_TEST_ENABLED), true);
    pEffect->SetVectorArray(cm.handle(ConstantsMap::CONST_MATERIAL), (D3DXVECTOR4*)material, 2);

    // light adjustment
    float adjustment = speedTree_->GetLeafLightingAdjustment();
    pEffect->SetFloat(cm.handle(ConstantsMap::CONST_LEAF_LIGHT_ADJUST), adjustment);

    // texture, draw textured flag
    
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_TEXTURED_TREES), TSpeedTreeType::s_texturedTrees_);
    if (rd.diffuseMap_.exists())
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_DIFFUSE_MAP), rd.diffuseMap_->pTexture());
    }

#if SPT_ENABLE_NORMAL_MAPS
    bool bUseNormals = rd.normalMap_.exists();
    if (bUseNormals)
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_NORMAL_MAP), rd.normalMap_->pTexture());
    }
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_NORMAL_MAP), bUseNormals);
#endif //-- SPT_ENABLE_NORMAL_MAPS

    //-- setup wind matrices and leaf billboard tables.
    if (pWind_)
    {
        pEffect->SetMatrixArray(
            cm.handle(ConstantsMap::CONST_WIND_MATRICES), pWind_->matrices(), pWind_->matrixCount()
            );

        pEffect->SetVectorArray(
            cm.handle(ConstantsMap::CONST_LEAF_ANGLES),
            reinterpret_cast<const D3DXVECTOR4*>(pWind_->leafAnglesTable()), pWind_->leafAnglesCount()
            );      
    }

    Vector4 leafAngleScalars(treeData_.leafRockScalar_,treeData_.leafRustleScalar_, 0, 0 );
    pEffect->SetVector(cm.handle(ConstantsMap::CONST_LEAF_ANGLE_SCALARS), (D3DXVECTOR4*)&leafAngleScalars );
    pEffect->SetFloat(cm.handle(ConstantsMap::CONST_LEAF_ROCK_FAR), TSpeedTreeType::s_leafRockFar_);
}

/**
 *  Sets the render states for drawing the billboards of this type of tree.
 */
void TSpeedTreeType::setBillboardRenderStates(Moo::EffectMaterial& effect, const ConstantsMap& cm) const
{
    BW_GUARD;

    ID3DXEffect*          pEffect = effect.pEffect()->pEffect();
    const BillboardRenderData& rd = treeData_.billboards_;

    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_Z_PRE_PASS), s_enableZPrePass_);
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_ALPHA_TEST_ENABLED), true);
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_TEXTURED_TREES), s_texturedTrees_);
    if (rd.diffuseMap_.exists())
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_DIFFUSE_MAP), rd.diffuseMap_->pTexture());
    }

#if SPT_ENABLE_NORMAL_MAPS
    bool bUseNormals = rd.normalMap_.exists();
    if (bUseNormals)
    {
        pEffect->SetTexture(cm.handle(ConstantsMap::CONST_NORMAL_MAP), rd.normalMap_->pTexture());
    }
    pEffect->SetBool(cm.handle(ConstantsMap::CONST_USE_NORMAL_MAP), bUseNormals);
#endif // SPT_ENABLE_NORMAL_MAPS

    // light adjustment
    float adjustment = speedTree_->GetLeafLightingAdjustment();
    pEffect->SetFloat(cm.handle(ConstantsMap::CONST_LEAF_LIGHT_ADJUST), adjustment);

    // leaf material
    const float* leafMaterial = speedTree_->GetLeafMaterial();
    float material[] =
    {
        leafMaterial[0], leafMaterial[1], leafMaterial[2], 1.0f,    
        leafMaterial[3], leafMaterial[4], leafMaterial[5], 1.0f,    
    };
    pEffect->SetVectorArray(cm.handle(ConstantsMap::CONST_MATERIAL), (D3DXVECTOR4*)material, 2);
}

/**
 *  Stores the current sun light information. It will
 *  later be used to setup the lighting for drawing the trees.
 *
 *  @param  envMinder   the current EnvironMander object.
 */
void TSpeedTreeType::saveLightInformation( EnviroMinder * envMinder )
{
    if (envMinder != NULL)
    {
        TSpeedTreeType::s_sunDirection_ = Moo::rc().effectVisualContext().sunLight().m_dir;
        TSpeedTreeType::s_sunDiffuse_   = Moo::rc().effectVisualContext().sunLight().m_color;
        TSpeedTreeType::s_sunAmbient_   = Moo::rc().effectVisualContext().sunLight().m_ambient;
    }
    else
    {
        TSpeedTreeType::s_sunDirection_ = Vector3(0.0f, -1.0f, 0.0f);
        TSpeedTreeType::s_sunDiffuse_   = Moo::Colour(0.5f, 0.5f, 0.5f, 1.0f);
        TSpeedTreeType::s_sunAmbient_   = Moo::Colour(0.58f, 0.51f, 0.38f, 1.0f);
    }
}

/**
 *  Retrieves a TSpeedTreeType instance for the given filename and seed. 
 *
 *  If this is the first time this tree is being requested, it  will be 
 *  loaded, generated and cached in the tree-types-map (s_typesMap). 
 *  Otherwise, the cached tree will be retrieved from the map and 
 *  returned, instead. 
 *
 *  Note: this function will also look for cached trees in the 
 *  sync-init-list (s_syncInitList). It may be temporarily stored there if 
 *  the tree have just recently been loaded but not yet sync-initialised 
 *  (that's when it actually get's inserted into the tree-types-map).
 *
 *  After loading the tree file (".spt"), this function will then look 
 *  for a pre-computed-tree file (".ctree"). If one is found, the geometry 
 *  data will be loaded directly from this file. Otherwise,  the tree will 
 *  be computed and the geometry will be extracted from it. The geometry 
 *  will then be saved into a ".ctree" file for future use. 
 *
 *  Note that the ".spt" file is always loaded. That's because some run-time 
 *  operations, most notably LOD level calculations, are responsibility of 
 *  the original CSpeedTreeRT instance.
 *
 *  @see                doSyncInit().
 *
 *  @param  filename    name of source SPT file from where to generate tree.
 *  @param  seed        seed number to use when generation tree.
 *
 *  @return             the requested tree type object.
 *
 *  @note   will throw std::runtime_error if object can't be created.
 */
TSpeedTreeType::TreeTypePtr TSpeedTreeType::getTreeTypeObject(const char * filename, uint seed)
{
    BW_GUARD;
    START_TIMER(loadGlobal);

    TreeTypePtr result;

    // first, look at list of
    // existing renderer objects
    START_TIMER(searchExisting);
    {
        char treeDefName[BW_MAX_PATH];
        createTreeDefName( filename, seed, treeDefName,
            ARRAY_SIZE( treeDefName ) );

        SimpleMutexHolder mutexHolder(TSpeedTreeType::s_syncInitLock_);
        const TreeTypesMap & typesMap = TSpeedTreeType::s_typesMap_;
        TreeTypesMap::const_iterator rendererIt = typesMap.find( treeDefName );
        if ( rendererIt != typesMap.end() )
        {
            result = rendererIt->second;
        }
        // it may still be in the sync init
        // list. Look for it there, then.
        else
        {
            // list should be small. A linear search
            // here is actually not at all that bad.
            typedef TSpeedTreeType::InitVector::const_iterator citerator;
            citerator initIt  = TSpeedTreeType::s_syncInitList_.begin();
            citerator initEnd = TSpeedTreeType::s_syncInitList_.end();
            while ( initIt != initEnd )
            {
                if ( (*initIt)->seed_ == seed &&
                     (*initIt)->filename_ == filename )
                {
                    result = *initIt;
                    break;
                }
                ++initIt;
            }
        }
    }
    STOP_TIMER(searchExisting, "");

    // if still not found,
    // create a new one
    if ( !result.exists() )
    {
        START_TIMER(actualLoading);
        typedef std::auto_ptr< TSpeedTreeType > RendererAutoPtr;
        RendererAutoPtr renderer(new TSpeedTreeType);

        // not using speedtree's clone function here because
        // it prevents us from saving from memory by calling
        // Delete??Geometry after generating all geometry.

        bool success = false;
        const char * errorMsg = "Unknown error";
        CSpeedTreeRTPtr speedTree;
        try
        {
            DataSectionPtr rootSection = BWResource::instance().rootSection();
            BinaryPtr sptData = rootSection->readBinary( filename );
            if ( sptData.exists() )
            {
                speedTree.reset( new CSpeedTreeRT );
                speedTree->SetTextureFlip( true );
                success = speedTree->LoadTree(
                    (uchar*)sptData->data(),
                    sptData->len() );
                if (!success)
                {
                    errorMsg = CSpeedTreeRT::GetCurrentError();
                }
            }
            else
            {
                errorMsg = "Resource not found";
            }
        }
        catch (...)
        {
            errorMsg = CSpeedTreeRT::GetCurrentError();
        }

        if ( !success )
        {
            BW::string runtimeError = BW::string( filename ) + ": " + errorMsg;
            throw std::runtime_error( runtimeError.c_str() );
        }
        STOP_TIMER(actualLoading, "");

        renderer->seed_     = seed;
        renderer->filename_ = filename;
        speedTree->SetBranchLightingMethod( CSpeedTreeRT::LIGHT_DYNAMIC );
        speedTree->SetLeafLightingMethod( CSpeedTreeRT::LIGHT_DYNAMIC );
        speedTree->SetFrondLightingMethod( CSpeedTreeRT::LIGHT_DYNAMIC );

        speedTree->SetBranchWindMethod( CSpeedTreeRT::WIND_GPU );
        speedTree->SetLeafWindMethod( CSpeedTreeRT::WIND_GPU );
        speedTree->SetFrondWindMethod( CSpeedTreeRT::WIND_GPU );

        speedTree->SetNumLeafRockingGroups(4);

        char treeDataName[BW_MAX_PATH];
        bw_str_copy( treeDataName, ARRAY_SIZE( treeDataName ),
            BWResource::removeExtension( filename ) );
        bw_str_append( treeDataName, ARRAY_SIZE( treeDataName ), ".ctree" );

        if ( !speedTree->Compute( NULL, seed ) )
        {
            BW::string runtimeError = BW::string(filename) + ": Could not compute tree";
            throw std::runtime_error( runtimeError.c_str() );
        }

        // speed wind
        BW::string datapath = BWResource::getFilePath( filename );
        renderer->pWind_ = setupSpeedWind( *speedTree, datapath,
                                            TSpeedTreeType::s_speedWindFile_ );

        if ( BWResource::isFileOlder( treeDataName, filename ) ||
            !renderer->treeData_.load( treeDataName ) )
        {
            int windAnglesCount = 
                renderer->pWind_->matrixCount();

            renderer->treeData_ = TSpeedTreeType::generateTreeData(
                                    *speedTree, *renderer->pWind_->speedWind(),
                                    filename, seed, windAnglesCount );

#ifndef CONSUMER_CLIENT
            renderer->treeData_.save( treeDataName );
#endif
        }

        if ( renderer->pWind_ != &TSpeedTreeType::s_defaultWind_ )
        {
            renderer->pWind_->hasLeaves( renderer->pWind_->hasLeaves() ||
                                    renderer->treeData_.leaves_.checkVerts() );
        }

        renderer->speedTree_ = speedTree;

        // bsp tree
        START_TIMER(bspTree);
        renderer->bspTree_ = createBSPTree(
            renderer->speedTree_.get(),
            renderer->filename_.c_str(),
            renderer->seed_,
            renderer->treeData_.boundingBox_ );
        STOP_TIMER(bspTree, "");

        // release geometry data
        // held internally by speedtree
        renderer->speedTree_->DeleteBranchGeometry();
        renderer->speedTree_->DeleteFrondGeometry();
        renderer->speedTree_->DeleteLeafGeometry();
        renderer->speedTree_->DeleteTransientData();

        result = renderer.release();

        // Because it involves a lot of D3D work, the last bit
        // of initialisation must be done in the main thread.
        // Put render in deferred initialisation list so that
        // all pending initialisation tasks will be carried
        // next beginFrame call.
        START_TIMER(syncInitList);
        {
            SimpleMutexHolder mutexHolder(TSpeedTreeType::s_syncInitLock_);
            TSpeedTreeType::s_syncInitList_.push_back(result.getObject());
        }
        STOP_TIMER(syncInitList, "");
    }

    STOP_TIMER(loadGlobal, filename);

    return result;
}

/**
 *  Generates all rendering data for the tree (index and vertex buffers
 *  plus auxiliary information).
 *
 *  @param  speedTree       speedtree object from which to extract the data.
 *  @param  filename        name of the spt file from where the tree was loaded.
 *  @param  seed            seed number used to generate tree.
 *  @param  windAnglesCount number of wind angles used to animate trees.
 *
 *  @return                 a filled up TreeData structure.
 */
TSpeedTreeType::TreeData TSpeedTreeType::generateTreeData(
    CSpeedTreeRT& speedTree, CSpeedWind& speedWind, const BW::string& filename, 
    int seed, int windAnglesCount)
{
    BW_GUARD;
    START_TIMER(generateTreeData);

    SimpleMutexHolder mutexHolder(TSpeedTreeType::s_vertexListLock_);

    TreeData result;

    // get boundingbox
    float bounds[6];
    speedTree.GetBoundingBox( bounds );
    result.boundingBox_.setBounds( 
        Vector3(bounds[0], bounds[1], -bounds[5]),
        Vector3(bounds[3], bounds[4], -bounds[2] ) );

    BW::string strName = BWResource::getFilename( filename ).to_string();
    strlwr( &strName[0] );
    result.isBush = TSpeedTreeType::isBushName( strName );

    // create the geomtry buffers
    //
    BW::string datapath = BWResource::getFilePath( filename );

    TRACE_MSG( "generateTreeData for tree type %s\n", filename.c_str() );

    START_TIMER(localBranches);
    result.branches_ = createPartRenderData< BranchTraits >(
        speedTree, speedWind, datapath );
    STOP_TIMER(localBranches, "");

    START_TIMER(localFronds);
    result.fronds_ = createPartRenderData< FrondTraits >(
        speedTree, speedWind, datapath );
    STOP_TIMER(localFronds, "");

    START_TIMER(localLeaves);
    result.leaves_ = createLeafRenderData(
        speedTree, speedWind, filename, datapath,
        windAnglesCount, result.leafRockScalar_, 
        result.leafRustleScalar_ );
    STOP_TIMER(localLeaves, "");

    START_TIMER(localBillboards);
    result.billboards_ = createBillboardRenderData(
        speedTree, datapath );
    STOP_TIMER(localBillboards, "");

    STOP_TIMER(generateTreeData, "");

    return result;
}

/**
 *  Performs asynchronous initialisation. Any initialisation
 *  that can be done in the loading thread should be done here.
 */
void TSpeedTreeType::asyncInit()
{
    BW_GUARD;   
}

/**
 *  Performs synchronous initialisation. Any initialisation that
 *  has got to be carried in the rendering thread can be done here.
 */
void TSpeedTreeType::syncInit()
{
    BW_GUARD;
    START_TIMER(addTreeType);

#if ENABLE_BB_OPTIMISER
    if (TSpeedTreeType::s_optimiseBillboards_ &&
        !treeData_.billboards_.lod_.empty() &&
        treeData_.billboards_.diffuseMap_)
        {
            bbTreeType_ = BillboardOptimiser::addTreeType(
                treeData_.billboards_.diffuseMap_,              
#if SPT_ENABLE_NORMAL_MAPS
                treeData_.billboards_.normalMap_,
#endif // SPT_ENABLE_NORMAL_MAPS
                *treeData_.billboards_.lod_.back().index_.get(),
                filename_.c_str(),
                speedTree_->GetLeafMaterial(),
                speedTree_->GetLeafLightingAdjustment(),
                materialHandle_ * 4 + 3);
        }
    #endif
    STOP_TIMER(addTreeType, "");
}

/**
 *  Increments the reference counter.
 */
void TSpeedTreeType::incRef() const
{
    ++this->refCounter_;
}

/**
 *  Increments the reference count. DOES NOT delete the object if
 *  the reference counter reaches zero. The recycleTreeTypeObjects
 *  method will take care of that (for multithreading sanity).
 */
void TSpeedTreeType::decRef() const
{
    --this->refCounter_;
    MF_ASSERT(this->refCounter_>=0);
}

/**
 *  Returns the current reference counter value.
 */
int  TSpeedTreeType::refCount() const
{
    return this->refCounter_;
}

//--------------------------------------------------------------------------------------------------
TSpeedTreeType::TreeData::TreeData() :
    boundingBox_( BoundingBox::s_insideOut_ ),
    branches_(),
    fronds_(),
    leaves_(),
    billboards_(),
    leafRockScalar_( 0.0f ),
    leafRustleScalar_( 0.0f )
{
    BW_GUARD;
}

/**
 *  Saves tree render data to a binary cache file.
 *
 *  @param  filename    path to file to be saved.
 *
 *  @return             true if successfull, false on error.
 */
bool TSpeedTreeType::TreeData::save( const BW::string & filename )
{
    BW_GUARD;
    START_TIMER(saveTreeData);

    int totalSize = 0;

    // version info
    totalSize += sizeof(int);

    // bounding box + leaf scalars
    totalSize += 2*sizeof(Vector3) + 2*sizeof(float);

    //-- render part data
    totalSize += this->branches_.size();
    totalSize += this->fronds_.size();
    totalSize += this->leaves_.size();
    totalSize += this->billboards_.size();

    char * pdata = new char[totalSize];
    char * data = pdata;

    int version = SPT_CTREE_VERSION;
    memcpy( data, &version, sizeof(int) );
    data += sizeof(int);

    memcpy( data, &this->boundingBox_.minBounds(), sizeof(Vector3) );
    data += sizeof(Vector3);

    memcpy( data, &this->boundingBox_.maxBounds(), sizeof(Vector3) );
    data += sizeof(Vector3);

    *(float*)data = this->leafRockScalar_;
    data += sizeof(float);

    *(float*)data = this->leafRustleScalar_;
    data += sizeof(float);

    START_TIMER(saveRenderData);

#if ENABLE_FILE_CASE_CHECKING
    FilenameCaseChecker caseChecker;
    const BW::string & diff1 =  this->branches_.diffuseMap_.exists() ?  
        this->branches_.diffuseMap_->resourceID() : "";
    caseChecker.check( diff1 );

    const BW::string & diff2 =  this->fronds_.diffuseMap_.exists() ?    
        this->fronds_.diffuseMap_->resourceID() : "";
    caseChecker.check( diff2 );

    const BW::string & diff3 =  this->leaves_.diffuseMap_.exists() ?    
        this->leaves_.diffuseMap_->resourceID() : "";
    caseChecker.check( diff3 );

    const BW::string & diff4 =  this->billboards_.diffuseMap_.exists() ?    
        this->billboards_.diffuseMap_->resourceID() : "";
    caseChecker.check( diff4 );
#endif // ENABLE_FILE_CASE_CHECKING

    {
        SimpleMutexHolder mutexHolder(TSpeedTreeType::s_vertexListLock_);
        data = this->branches_.write( data, CommonTraits::s_vertexList_, CommonTraits::s_indexList_ );
        data = this->fronds_.write( data, CommonTraits::s_vertexList_, CommonTraits::s_indexList_  );
        data = this->leaves_.write( data, LeafTraits::s_vertexList_, LeafTraits::s_indexList_  );
        data = this->billboards_.write( data, BillboardTraits::s_vertexList_, BillboardTraits::s_indexList_ );
    }

    STOP_TIMER(saveRenderData, "");

    // create a section
    START_TIMER(saveSection);
    BW::string::size_type lastSep = filename.find_last_of('/');
    BW::string parentName = filename.substr( 0, lastSep );
    DataSectionPtr parentSection = BWResource::openSection( parentName );
    MF_ASSERT(parentSection);

    BW::StringRef nameonly = BWResource::getFilename( filename );

    DataSectionPtr file = new BinSection( 
        nameonly, new BinaryBlock( pdata, totalSize, "BinaryBlock/SpeedTree" ) );

    delete [] pdata;

    DataSectionPtr pExisting = DataSectionCensus::find(filename);
    if (pExisting != NULL)
    {
        // Remove this from the census to ensure we can write a fresh copy
        DataSectionCensus::del(pExisting.getObject());
    }

    file->setParent( parentSection );
    file = DataSectionCensus::add( filename, file );
    bool result = file->save(); 
    STOP_TIMER(saveSection, "");

    STOP_TIMER(saveTreeData, "");
    return result;  
}

/**
 *  Loads tree render data from a binary cache file.
 *
 *  @param  filename    path to file to be saved.
 *
 *  @return             true if successfull, false on error. Will fail
 *                      if it can't other the specified file.
 */
bool TSpeedTreeType::TreeData::load( const BW::string & filename )
{
    BW_GUARD;
    START_TIMER(loadTreeData);

    START_TIMER(openSection);
    DataSectionPtr file =
        BWResource::openSection( filename, false, BinSection::creator() );
    STOP_TIMER(openSection, "");

    if ( !file )
        return false;

    const char * data = file->asBinary()->cdata();

    // read version data
    int version;
    memcpy( &version, data, sizeof(int) );
    data += sizeof(int);
    if (version != SPT_CTREE_VERSION)
        return false;

    Vector3 minBounds;
    memcpy( &minBounds, data, sizeof(Vector3) );
    data += sizeof(Vector3);

    Vector3 maxBounds;
    memcpy( &maxBounds, data, sizeof(Vector3) );
    data += sizeof(Vector3);

    this->boundingBox_.setBounds( minBounds, maxBounds );

    BW::string strName = BWResource::getFilename( filename ).to_string();
    strlwr( &strName[0] );
    this->isBush = TSpeedTreeType::isBushName( strName );

    this->leafRockScalar_ = *(float*)data;
    data += sizeof(float);

    this->leafRustleScalar_ = *(float*)data;
    data += sizeof(float);

    START_TIMER(loadRenderData);
    {
        SimpleMutexHolder mutexHolder(TSpeedTreeType::s_vertexListLock_);
        data = this->branches_.read( data, CommonTraits::s_vertexList_, CommonTraits::s_indexList_ );
        data = this->fronds_.read( data, CommonTraits::s_vertexList_, CommonTraits::s_indexList_ );
        data = this->leaves_.read( data, LeafTraits::s_vertexList_, LeafTraits::s_indexList_ );
        data = this->billboards_.read( data, BillboardTraits::s_vertexList_, BillboardTraits::s_indexList_ );
    }

    STOP_TIMER(loadRenderData, "");

    STOP_TIMER(loadTreeData, "");

    return true;
}

//--------------------------------------------------------------------------------------------------
template< typename VertexType >
void TreePartRenderData<VertexType>::unload(STVector<VertexType>& verts, STVector<uint32>& indices)
{
    // remove the vertices from the list.
    if ( verts_ )
    {
        verts.eraseSlot( verts_ );
        verts_ = NULL;
    }

    // remove the indices.
    typename LodDataVector::iterator it=lod_.begin();
    while ( it!=lod_.end() )
    {
        if ((*it).index_)
        {
            indices.eraseSlot( (*it).index_ );
            (*it).index_ = NULL;
        }
        it++;
    }
}

//--------------------------------------------------------------------------------------------------
void TSpeedTreeType::TreeData::unload()
{
    SimpleMutexHolder mutexHolder(TSpeedTreeType::s_vertexListLock_);
    branches_.unload( CommonTraits::s_vertexList_, CommonTraits::s_indexList_ );
    fronds_.unload( CommonTraits::s_vertexList_, CommonTraits::s_indexList_ );
    leaves_.unload( LeafTraits::s_vertexList_, LeafTraits::s_indexList_ );
    billboards_.unload( BillboardTraits::s_vertexList_, BillboardTraits::s_indexList_ );
}


/**
 *  This returns the number of triangles in the tree.
 */
int SpeedTreeRenderer::numTris() const
{
    BW_GUARD;
    int tris = 0;

    if (this->treeType_)
    {
        TSpeedTreeType::TreeData& treeData = treeType_->treeData_;

        if (!treeData.branches_.lod_.empty() && treeData.branches_.lod_[ 0 ].index_)
        {
            tris += treeData.branches_.lod_[ 0 ].index_->count() - 2;
        }

        if (!treeData.fronds_.lod_.empty() && treeData.fronds_.lod_[ 0 ].index_)
        {
            tris += treeData.fronds_.lod_[ 0 ].index_->count() - 2;
        }

        if (!treeData.leaves_.lod_.empty() && treeData.leaves_.lod_[ 0 ].index_)
        {
            tris += treeData.leaves_.lod_[ 0 ].index_->count() - 2;
        }

        if (!treeData.billboards_.lod_.empty() && treeData.billboards_.lod_[ 0 ].index_)
        {
            tris += treeData.billboards_.lod_[ 0 ].index_->count() - 2;
        }
    }

    return tris;
}

/**
*   This returns the amount of the texture memory used by the tree.
*/
int SpeedTreeRenderer::getTextureMemory() const
{
    BW_GUARD;
    int memory = 0;
    BW::map<void*, int> textures;

    if (this->treeType_)
    {
        TSpeedTreeType::TreeData & treeData = this->treeType_->treeData_;
        if (treeData.branches_.diffuseMap_.exists())
            textures.insert(std::make_pair( treeData.branches_.diffuseMap_.get(), 
                                            treeData.branches_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.branches_.normalMap_.exists())
            textures.insert(std::make_pair( treeData.branches_.normalMap_.get(), 
                                            treeData.branches_.normalMap_->textureMemoryUsed()));
#endif

        if (treeData.fronds_.diffuseMap_.exists())
            textures.insert(std::make_pair( treeData.fronds_.diffuseMap_.get(), 
                                            treeData.fronds_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.fronds_.normalMap_.exists())
            textures.insert(std::make_pair( treeData.fronds_.normalMap_.get(), 
                                            treeData.fronds_.normalMap_->textureMemoryUsed()));
#endif

        if (treeData.leaves_.diffuseMap_.exists())
            textures.insert(std::make_pair( treeData.leaves_.diffuseMap_.get(), 
                                            treeData.leaves_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.leaves_.normalMap_.exists())
            textures.insert(std::make_pair( treeData.leaves_.normalMap_.get(), 
                                            treeData.leaves_.normalMap_->textureMemoryUsed()));
#endif

        if (treeData.billboards_.diffuseMap_.exists())
            textures.insert(std::make_pair( treeData.billboards_.diffuseMap_.get(), 
            treeData.billboards_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.billboards_.normalMap_.exists())
            textures.insert(std::make_pair( treeData.billboards_.normalMap_.get(), 
            treeData.billboards_.normalMap_->textureMemoryUsed()));
#endif

        for(BW::map<void*, int>::const_iterator it = textures.begin(); it != textures.end(); it++)
            memory += it->second;
    }

    return memory;
}

/**
*   This returns the amount of the vertex memory used by the tree.
*/
int SpeedTreeRenderer::getVertexBufferMemory() const
{
    BW_GUARD;
    int vertMemory = 0;

    if (this->treeType_)
    {
        TSpeedTreeType::TreeData & treeData = this->treeType_->treeData_;
        vertMemory += treeData.branches_.size();
        vertMemory += treeData.fronds_.size();
        vertMemory += treeData.leaves_.size();
        vertMemory += treeData.billboards_.size();
    }

    return vertMemory;
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::getTexturesAndVertices( BW::map<void*, int>& textures, BW::map<void*, int>& vertices )
{
    BW_GUARD;

    if (this->treeType_)
    {
        TSpeedTreeType::TreeData & treeData = this->treeType_->treeData_;
        if (treeData.branches_.diffuseMap_.exists())
            textures.insert(std::make_pair(treeData.branches_.diffuseMap_.get(), treeData.branches_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.branches_.normalMap_.exists())
            textures.insert(std::make_pair(treeData.branches_.normalMap_.get(), treeData.branches_.normalMap_->textureMemoryUsed()));
#endif

        if (treeData.fronds_.diffuseMap_.exists())
            textures.insert(std::make_pair(treeData.fronds_.diffuseMap_.get(), treeData.fronds_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.fronds_.normalMap_.exists())
            textures.insert(std::make_pair(treeData.fronds_.normalMap_.get(), treeData.fronds_.normalMap_->textureMemoryUsed()));
#endif

        if (treeData.leaves_.diffuseMap_.exists())
            textures.insert(std::make_pair(treeData.leaves_.diffuseMap_.get(), treeData.leaves_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.leaves_.normalMap_.exists())
            textures.insert(std::make_pair(treeData.leaves_.normalMap_.get(), treeData.leaves_.normalMap_->textureMemoryUsed()));
#endif

        if (treeData.billboards_.diffuseMap_.exists())
            textures.insert(std::make_pair(treeData.billboards_.diffuseMap_.get(), treeData.billboards_.diffuseMap_->textureMemoryUsed()));
#ifdef SPT_ENABLE_NORMAL_MAPS
        if (treeData.billboards_.normalMap_.exists())
            textures.insert(std::make_pair(treeData.billboards_.normalMap_.get(), treeData.billboards_.normalMap_->textureMemoryUsed()));
#endif

        char uid[BW_MAX_PATH];
        this->getUID( uid, ARRAY_SIZE( uid ) );
        TSpeedTreeType::TreeTypesMap::const_iterator it = TSpeedTreeType::s_typesMap_.find( uid );
        if(it != TSpeedTreeType::s_typesMap_.end())
        {
            int vertMemory = 0;
            vertMemory += treeData.branches_.size();
            vertMemory += treeData.fronds_.size();
            vertMemory += treeData.leaves_.size();
            vertMemory += treeData.billboards_.size();
            vertices.insert(std::make_pair((void*)it->first.data(), vertMemory));
        }
    }
}

//--------------------------------------------------------------------------------------------------
void SpeedTreeRenderer::getUID( char * dst, size_t dstSize ) const
{
    createTreeDefName( filename(), seed(), dst, dstSize );  
}

/**
 *  This returns the number of primitives in the tree.
 */
int SpeedTreeRenderer::numPrims() const
{
    // One for the branches, one for the trunk and one for the leaves.
    return 3;
}


// ---------------------------------------------------------- Static Definitions
TSpeedTreeType::InitVector TSpeedTreeType::s_syncInitList_;
SimpleMutex TSpeedTreeType::s_syncInitLock_;
SimpleMutex TSpeedTreeType::s_vertexListLock_;

TSpeedTreeType::TreeTypesMap TSpeedTreeType::s_typesMap_;

bool  TSpeedTreeType::s_frameStarted_ = false;
float TSpeedTreeType::s_time_         = 0.f;

Matrix      TSpeedTreeType::s_invView_;
Matrix      TSpeedTreeType::s_view_;
Matrix      TSpeedTreeType::s_lodCamera_;
Matrix      TSpeedTreeType::s_projection_;
Matrix      TSpeedTreeType::s_viewProj_;
Moo::Colour TSpeedTreeType::s_sunAmbient_;
Moo::Colour TSpeedTreeType::s_sunDiffuse_;
Vector3     TSpeedTreeType::s_sunDirection_;

Matrix      TSpeedTreeType::s_alphaPassViewMat;
Matrix      TSpeedTreeType::s_alphaPassProjMat;

bool  TSpeedTreeType::s_enviroMinderLight_      = false;
bool  TSpeedTreeType::s_enableCulling_          = true;
bool  TSpeedTreeType::s_drawBoundingBox_        = false;
bool  TSpeedTreeType::s_enableInstaning_        = true;
bool  TSpeedTreeType::s_enableZPrePass_         = false;
bool  TSpeedTreeType::s_enableZDistSorting_     = false;
bool  TSpeedTreeType::s_drawTrees_              = true;
bool  TSpeedTreeType::s_drawLeaves_             = true;
bool  TSpeedTreeType::s_drawBranches_           = true;
bool  TSpeedTreeType::s_drawFronds_             = true;
bool  TSpeedTreeType::s_drawBillboards_         = true;
bool  TSpeedTreeType::s_texturedTrees_          = true;
bool  TSpeedTreeType::s_playAnimation_          = true;
float TSpeedTreeType::s_leafRockFar_            = 400.f;
float TSpeedTreeType::s_maxLod_                 = 1.f;
float TSpeedTreeType::s_lodMode_                = -2;
float TSpeedTreeType::s_lodNear_                = 50.f;
float TSpeedTreeType::s_lodFar_                 = 300.f;
float TSpeedTreeType::s_lod0Yardstick_          = 130.0f;
float TSpeedTreeType::s_lod0Variance_           = 1.0f;

#if ENABLE_BB_OPTIMISER
    bool  TSpeedTreeType::s_optimiseBillboards_ = true;
#endif

int TSpeedTreeType::s_speciesCount_  = 0;
int TSpeedTreeType::s_uniqueCount_   = 0;
int TSpeedTreeType::s_visibleCount_  = 0;
int TSpeedTreeType::s_lastPassCount_ = 0;
int TSpeedTreeType::s_deferredCount_ = 0;
int TSpeedTreeType::s_passNumCount_  = 0;
int TSpeedTreeType::s_totalCount_    = 0;

BW::string TSpeedTreeType::s_speedWindFile_ = "speedtree/SpeedWind.ini";

DogWatch TSpeedTreeType::s_globalWatch_("SpeedTree");
DogWatch TSpeedTreeType::s_prepareWatch_("Peparation");
DogWatch TSpeedTreeType::s_drawWatch_("Draw");
DogWatch TSpeedTreeType::s_primitivesWatch_("Primitives");

} // namespace speedtree

#ifdef EDITOR_ENABLED
#ifndef PYMODULE
    // forces linkage of
    // thumbnail provider
    extern int SpeedTreeThumbProv_token;
    int total = SpeedTreeThumbProv_token;
#endif
#endif // EDITOR_ENABLED

BW_END_NAMESPACE

#endif //-- SPEEDTREE_SUPPORT
