#include "pch.hpp"

#include "water.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"

#include "math/colour.hpp"

#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/effect_material.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/graphics_settings.hpp"
#include "moo/resource_load_context.hpp"
#include "moo/renderer.hpp"
#include "moo/render_event.hpp"
#include "moo/moo_utilities.hpp"
#if UMBRA_ENABLE
#include "terrain/base_terrain_block.hpp"
#endif //UMBRA_ENABLE

#include "effect_parameter_cache.hpp"
#include "enviro_minder.hpp"
#include "environment_cube_map.hpp"
#ifdef EDITOR_ENABLED
#include "appmgr/commentary.hpp"
#include "moo/geometrics.hpp"
#endif

#include "fog_controller.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk.hpp"
#if UMBRA_ENABLE
#include "chunk/chunk_terrain.hpp"
#endif //UMBRA_ENABLE

#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"

#include "speedtree/speedtree_renderer.hpp"

#include "cstdmf/watcher.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/concurrency.hpp"

#include "resmgr/auto_config.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/data_section_census.hpp"

#include "moo/line_helper.hpp"
#include "py_water_volume.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "water.ipp"
#endif

// ----------------------------------------------------------------------------
// Statics
// ----------------------------------------------------------------------------

#define BANK_DIST_DEFAULT_VALUE std::make_pair(Vector2::zero(), 100000000.0f)

uint32 Waters::s_nextMark_ = 1;
uint32 Waters::s_lastDrawMark_ = -1;
static AutoConfigString s_waterEffect( "environment/waterEffect" );
static AutoConfigString s_simulationEffect( "environment/waterSimEffect" );
static AutoConfigString s_screenFadeMap( "environment/waterScreenFadeMap" );
static bool s_enableDrawPrim = true;
static bool s_enableShadows = true;

// Water statics
Water::VertexBufferPtr          Water::s_quadVertexBuffer_;
Water::WaterRenderTargetMap     Water::s_renderTargetMap_;
bool                            Water::s_cullCells_ = true;
float                           Water::s_cullDistance_ = 1.2f;
                                //a bit further than the far plane to avoid popping

float                           Water::s_maxSimDistance_ = 200.0f;
float                           Water::s_sceneCullDistance_ = 300.f;
float                           Water::s_sceneCullFadeDistance_ = 30.f;

float                           Water::s_fTexScale = 0.5f;
float                           Water::s_fFreqX = 1.f;
float                           Water::s_fFreqZ = 1.f;
float                           Water::s_fBankLineInterpolationFactor = 10000.0f;

Moo::RenderTargetPtr            Water::s_rt = NULL;

// Water statics
#ifndef EDITOR_ENABLED
bool                            Water::s_backgroundLoad_    = true;
#else
bool                            Water::s_backgroundLoad_    = false;
#endif

// Waters statics
Moo::EffectMaterialPtr          Waters::s_effect_           = NULL;
Moo::EffectMaterialPtr          Waters::s_simulationEffect_ = NULL;
bool                            Waters::s_drawWaters_       = true;
bool                            Waters::s_drawReflection_   = true;
bool                            Waters::s_simulationEnabled_ = true;
bool                            Waters::s_highQuality_      = false;
bool                            Waters::s_forceUseCubeMap_	= false;
float                           Waters::s_autoImpactInterval_ = 0.4f;

#ifdef EDITOR_ENABLED
bool                            Waters::s_projectView_      = false;
#endif


#ifdef DEBUG_WATER

bool                            Water::s_debugSim_ = true;
int                             Water::s_debugCell_ = -1;
int                             Water::s_debugCell2_ = -1;

#define WATER_STAT(exp) exp

static uint                     s_waterCount=0;
static uint                     s_waterVisCount=0;
static uint                     s_activeCellCount=0;
static uint                     s_activeEdgeCellCount=0;
static uint                     s_movementCount=0;

#else
#define WATER_STAT(exp)
#endif //DEBUG_WATER

// ----------------------------------------------------------------------------
// Defines
// ----------------------------------------------------------------------------
#define MAX_SIM_TEXTURE_SIZE 256
// defines the distance from the edge of a cell that a movement will activate
// its neighbour
#define ACTIVITY_THRESHOLD 0.3f
#define RLE_KEY_VALUE 53
// ----------------------------------------------------------------------------
// Section: WaterCell
// ----------------------------------------------------------------------------

/**
 *  WaterCell contructor
 */
WaterCell::WaterCell() : 
        nIndices_( 0 ),
        water_( 0 ),
        xMax_( 0 ),
        zMax_( 0 ),
        xMin_( 0 ),
        zMin_( 0 ),
        min_(0,0),
        max_(0,0),      
        size_(0,0)
{
    BW_GUARD;
    for (int i=0; i<4; i++)
    {
        edgeCells_[i]=NULL;
    }
}

char* neighbours[4] = { "neighbourHeightMap0",
                        "neighbourHeightMap1",
                        "neighbourHeightMap2",
                        "neighbourHeightMap3" };

/**
 *  Binds the specified neighbour cell.
 */
void WaterCell::bindNeighbour( Moo::EffectMaterialPtr effect, int edge )
{
    BW_GUARD;
    MF_ASSERT(edge<4 && edge>=0);

    // only bind if active, else fall back to null simulation texture
    // if it is available, else unbind texture.
    if ( edgeCells_[edge] && edgeCells_[edge]->isActive() )
    {
        edgeCells_[edge]->bindAsNeighbour( effect, neighbours[edge] );
    }
    else if (SimulationManager::instance().nullSim())
    {
        effect->pEffect()->pEffect()->SetTexture( neighbours[edge], 
                    SimulationManager::instance().nullSim()->pTexture() );
    }
    else
    {
        effect->pEffect()->pEffect()->SetTexture( neighbours[edge], NULL );
    }
}

void WaterCell::activateEdgeCell( uint index )
{
    BW_GUARD;
    this->addEdge((index+2) % 4);
    this->resetIdleTimer();
    this->edgeActivation_ = true;
    this->perturbed( true );
}

/**
 *  Checks for movement activity within a cell and activates/deactivates 
 *  accordingly
 */
void WaterCell::checkActivity(  SimulationCellPtrList& activeList,
                                WaterCellPtrList& edgeList )
{
    BW_GUARD;
    if (hasMovements())
        perturbed(true);
    else if ( !edgeActivation_ ) //edge-activated cells shouldnt be disabled here
        perturbed(false);

    if (shouldActivate())
    {
        activate();     
        
        // activation possibly failed if no available textures. check activation state.
        if (isActive()) 
        {
            activeList.push_back( this );
        }
    }
    else 
    {
        bool outOfRange = false;
        if (isActive())
        {
            outOfRange = (cellDistance_ > Water::s_maxSimDistance_);            

            if (outOfRange)
            {
                clearMovements();
                perturbed(false);
            }
        }

        if ( outOfRange || shouldDeactivate() )
        {
            deactivate();
            activeList.remove( this );
            edgeActivation_=false;
        }
    }

    //inter-cell simulation....
    if (Waters::instance().simulationEnabled()) // only do for high detail settings
    {
        if (isActive())
        {
            if (edgeActivation_ == false)
                edge(0);

            int edges = getActiveEdge(ACTIVITY_THRESHOLD);

            if (movements().size())
            {
                this->edge(edges);
                if (edges > 0)
                {
                    WaterCellPtrList cornerCells;

                    for (int i=0; i<4; i++)
                    {
                        WaterCell* edgeCell = edgeCells_[i];
                        if (edgeCell && (edges & (1<<i)))
                        {
                            edgeCell->activateEdgeCell( i );
                            edgeList.push_back( edgeCell );

                            edgeActivation_ = true;

                            // check for a corner.
                            uint cornerIdx = (i+1) % 4;
                            if ( (edges & (1<<(cornerIdx))) )
                            {
                                WaterCell* cornerCell1 = edgeCell->edgeCell( cornerIdx );
                                WaterCell* cornerCell2 = edgeCells_[ cornerIdx ];
                                if ( cornerCell1 )
                                {
                                    // Activate the edges around the corner.
                                    edgeCell->addEdge( cornerIdx );
                                    if (cornerCell2)
                                    {
                                        cornerCell1->addEdge( (cornerIdx+1) % 4 );
                                        cornerCell2->addEdge( i );
                                    }
                                    cornerCell1->activateEdgeCell( cornerIdx );
                                    cornerCells.push_back( cornerCell1 );
                                }
                            }
                        }
                    }

                    // Cells at the front of this list have a higher activation priority, 
                    // cells on an edge have priority over cells on a corner.
                    edgeList.insert( edgeList.end(), cornerCells.begin(), cornerCells.end() );
                }
            }

            if (edgeActivation_)
            {
                edgeList.push_back( this );

                bool deactivate=!hasMovements();
                for (int i=0; i<4 && deactivate; i++)
                {
                    if ( edgeCells_[i] && 
                            edgeCells_[i]->isActive() )
                    {
                        if ( edgeCells_[i]->hasMovements() )
                            deactivate = false;
                    }
                }
                if (deactivate)
                    perturbed( false );
            }
        }
        else if (edgeActivation_)
        {
            perturbed( false );
            edgeList.push_back( this );
        }
    }
}


/**
 *  Activity check when activated by a neighbour
 */
void WaterCell::checkEdgeActivity( SimulationCellPtrList& activeList )
{
    BW_GUARD;
    if (edgeActivation_)
    {
        if (hasMovements())
            perturbed(true);            
        if (shouldActivate())
        {
            activate();
            activeList.push_back( this );
        }
        else if (shouldDeactivate())
        {
            edgeActivation_ = false;
            deactivate();
            activeList.remove( this );
        }
    }
}


/**
 * Update the cell distance.
 */
void WaterCell::updateDistance(const Vector3& camPos)
{
    BW_GUARD;
    BoundingBox box = water_->bb();
    Vector3 cellMin(min().x, 0, min().y);
    Vector3 cellMax(max().x, 0, max().y);

    //TODO: cells may need to store their BBs...
    box.setBounds(  box.minBounds() + cellMin,
                    box.minBounds() + cellMax );
    cellDistance_ = box.distance(camPos);
}


/**
 *  Initialise a water cell
 */
bool WaterCell::init( Water* water, Vector2 start, Vector2 size )
{ 
    BW_GUARD;
    if (!water)
        return false;

    water_ = water;

    min_ = start;
    max_ = (start+size);
    size_ = size;

    xMin_ = int( ceilf( min().x / water_->state_.tessellation_)  );
    zMin_ = int( ceilf( min().y / water_->state_.tessellation_)  );

    xMax_ = int( ceilf( (max().x) / water_->state_.tessellation_)  ) + 1;
    zMax_ = int( ceilf( (max().y) / water_->state_.tessellation_)  ) + 1;

    if (xMax_ > int( water_->gridSizeX_ ))
        xMax_ = int( water_->gridSizeX_ );

    if (zMax_ > int( water_->gridSizeZ_ ))
        zMax_ = int( water_->gridSizeZ_ );

    return true;
}


/**
 *  Release all managed data
 */
void WaterCell::deleteManaged()
{
    BW_GUARD;
    indexBuffer_.release();
}


bool Water::WaterState::load( DataSectionPtr pSection, 
          const Vector3 & defaultPosition, 
          const Matrix & mappingTransform )
{
    // load new settings (water created on first draw)
    DataSectionPtr pReqSec;

    pReqSec = pSection->openSection( "position" );
    if (!pReqSec)
    {
        position_ = defaultPosition;
    }
    else
    {
        position_ = mappingTransform.applyPoint( pReqSec->asVector3() );
    }

    float lori = pSection->readFloat( "orientation", 0.f );
    Vector3 oriVec( sinf( lori ), 0.f, cosf( lori ) );
    Vector3 worldVec = mappingTransform.applyVector(oriVec);
    orientation_ = atan2f( worldVec.x, worldVec.z );

    pReqSec = pSection->openSection( "size" );
    if (!pReqSec) return false;

    Vector3 sizeV3 = pReqSec->asVector3();
    size_ = Vector2( sizeV3.x, sizeV3.z );

    fresnelConstant_ = pSection->readFloat( "fresnelConstant", 0.3f );
    fresnelExponent_ = pSection->readFloat( "fresnelExponent", 5.f );

    reflectionTint_ = pSection->readVector4( "reflectionTint", Vector4(1,1,1,1) );
    reflectionScale_ = pSection->readFloat( "reflectionStrength", 0.04f );

    refractionTint_ = pSection->readVector4( "refractionTint", Vector4(1,1,1,1) );
    refractionScale_ = pSection->readFloat( "refractionStrength", 0.04f );

    tessellation_ = pSection->readFloat( "tessellation", 10.f );
    consistency_ = pSection->readFloat( "consistency", 0.95f );

    textureTessellation_ = pSection->readFloat( "textureTessellation", tessellation_);


    float oldX = pSection->readFloat( "scrollSpeedX", -1.f );   
    float oldY = pSection->readFloat( "scrollSpeedY", 1.f );

    scrollSpeed1_ = pSection->readVector2( "scrollSpeed1", Vector2(oldX,0.5f) );    
    scrollSpeed2_ = pSection->readVector2( "scrollSpeed2", Vector2(oldY,0.f) ); 
    waveScale_ = pSection->readVector2( "waveScale", Vector2(1.f,0.75f) );

    //windDirection_ = pSection->readFloat( "windDirection", 0.0f );    
    windVelocity_ = pSection->readFloat( "windVelocity", 0.02f );

    sunPower_ = pSection->readFloat( "sunPower", 32.f );
    sunScale_ = pSection->readFloat( "sunScale", 1.0f );

    simCellSize_ = pSection->readFloat( "cellsize", 100.f );
    smoothness_ = pSection->readFloat( "smoothness", 0.f );

    deepColour_ = pSection->readVector4( "deepColour", Vector4(0.f,0.20f,0.33f,1.f) );

    depth_ = pSection->readFloat( "depth", 10.f );
    fadeDepth_ = pSection->readFloat( "fadeDepth", 0.f );

    foamIntersection_ = pSection->readFloat( "foamIntersection", 0.25f );
    foamMultiplier_ = pSection->readFloat( "foamMultiplier", 0.75f );
    foamTiling_ = pSection->readFloat( "foamTiling", 1.f ); 
    foamFrequency_ = pSection->readFloat( "foamFreq", 0.3f );   
    foamAmplitude_ = pSection->readFloat( "foamAmplitude", 0.3f );  
    foamWidth_ = pSection->readFloat( "foamWidth", 10.0f ); 
    bypassDepth_ = pSection->readBool( "bypassDepth", false );

    useEdgeAlpha_ = pSection->readBool( "useEdgeAlpha", true );

    useSimulation_ = pSection->readBool( "useSimulation", true );

    visibility_ = pSection->readInt( "visibility", Water::ALWAYS_VISIBLE );
    if ( visibility_ != Water::ALWAYS_VISIBLE &&
        visibility_ != Water::INSIDE_ONLY &&
        visibility_ != Water::OUTSIDE_ONLY )
    {
        visibility_ = Water::ALWAYS_VISIBLE;
    }

    freqX = pSection->readFloat("freqX", 1.7f);
    freqZ = pSection->readFloat("freqZ", 2.2f);
    waveHeight = pSection->readFloat("waveHeight", 0.3f);
    causticsPower = pSection->readFloat("causticsPower", 0.2f);
    waveTextureNumber_ = pSection->readUInt("waveTextureNumber", 64);
    refractionPower_ = pSection->readFloat("waterContrast", 0.5f);
    animationSpeed_ = pSection->readFloat("animationSpeed", 4.0f);

    reflectBottom_ = pSection->readBool( "reflectBottom", true );
    useShadows_ = pSection->readBool( "useShadows", false );

    return true;
}


bool Water::WaterResources::load( DataSectionPtr pSection, 
          const BW::string & transparenctTable )
{
    waveTexture_ = pSection->readString( "waveTexture", "system/maps/water_normal_animation/normal" );

    foamTexture_ = pSection->readString( "foamTexture", "system/maps/water_foam2.dds" );    

    reflectionTexture_ = pSection->readString( "reflectionTexture", "system/maps/cloudyhillscubemap2.dds" );

    transparencyTable_ = transparenctTable;

    return true;
}

/**
 *  Take the supplied vertex index and map it to a spot in a new
 *  vertex buffer.
 */
uint32 Water::remapVertex( uint32 index )
{
    BW_GUARD;
    uint32 newIndex = index;
    if (remappedVerts_.size())
    {
        BW::map<uint32, uint32>& currentMap = remappedVerts_.back();
        BW::map<uint32,uint32>::iterator it = currentMap.find(newIndex);
        // check if it's already mapped
        if (it == currentMap.end())
        {
            // not found, remap
            newIndex = (uint32)currentMap.size();
            currentMap[index] = newIndex;
        }
        else
        {
            newIndex = it->second;
        }
    }
    return newIndex;
}

/**
 *  Remaps a list of vertex indices to ensure they are contained within a
 *  single vertex buffer.
 */
template< class T >
uint32 Water::remap( BW::vector<T>& dstIndices,
                    const BW::vector<uint32>& srcIndices )
{
    BW_GUARD;
    uint32 maxVerts = Moo::rc().maxVertexIndex();
    // Make a new buffer
    if (remappedVerts_.size() == 0)
    {
        remappedVerts_.resize(1);
    }
    // Allocate the destination index buffer.
    dstIndices.resize(srcIndices.size());

    // Transfer all the indices, remapping as necesary.
    for (uint i=0; i<srcIndices.size() ; i++)
    {
        dstIndices[i] = (T)this->remapVertex( srcIndices[i] );
    }

    // check if the current buffer has overflowed
    if ( remappedVerts_.back().size() > maxVerts)
    {
        // overflow, create new buffer + remap again
        remappedVerts_.push_back(BW::map<uint32, uint32>());    
        for (uint i=0; i<srcIndices.size() ; i++)
        {
            dstIndices[i] = (T)this->remapVertex( srcIndices[i] );
        }

        // If it's full again then it's an error (too many verts)
        if ( remappedVerts_.back().size() > maxVerts )
        {
            ERROR_MSG("Water::remap( ): Maximum vertex count "
                "exceeded, please INCREASE the \"Mesh Size\" parameter.\n" );
            remappedVerts_.pop_back();
            return 0;
        }
    }
    return static_cast<uint32>( remappedVerts_.size() );
}

/**
 *  Build the indices to match the templated value
 *  NOTE: only valid for uint32 and uint16
 */
template< class T >
void WaterCell::buildIndices( )
{
    BW_GUARD;
    #define WIDX_CHECKMAX  maxIndex = indices_32.back() > maxIndex ? \
                                    indices_32.back() : maxIndex;

    BW::vector< T >     indices;
    BW::vector< uint32 >    indices_32;

    int lastIndex = -1;
    bool instrip = false;
    uint32 gridIndex = 0;
    uint32 maxIndex=0;
    uint32 maxVerts = Moo::rc().maxVertexIndex();
    D3DFORMAT format;
    uint32 size = sizeof(T);

    if (size == 2)
        format = D3DFMT_INDEX16;
    else
        format = D3DFMT_INDEX32;

    // Build the master index list first.
    // then if any of the verts exceed the max... remap them all into a 
    // new vertex buffer.
    gridIndex = xMin_ + (water_->gridSizeX_*zMin_);
    for (uint z = uint(zMin_); z < uint(zMax_) - 1; z++)
    {
        for (uint x = uint(xMin_); x < uint(xMax_); x++)
        {
            bool lastX = (x == xMax_ - 1);

            if (!instrip && !lastX &&
                water_->wIndex_[ gridIndex ] >= 0 &&
                water_->wIndex_[ gridIndex + 1 ] >= 0 &&
                water_->wIndex_[ gridIndex + water_->gridSizeX_ ] >= 0 &&
                water_->wIndex_[ gridIndex + water_->gridSizeX_ + 1 ] >= 0
                )
            {
                if (lastIndex == -1) //first
                    lastIndex = water_->wIndex_[ gridIndex ];

                indices_32.push_back( uint32( lastIndex ) );

                indices_32.push_back( uint32( water_->wIndex_[ gridIndex ] ));
                WIDX_CHECKMAX

                indices_32.push_back( uint32( water_->wIndex_[ gridIndex ] ));
                WIDX_CHECKMAX

                indices_32.push_back( uint32( water_->wIndex_[ gridIndex + water_->gridSizeX_] ));
                WIDX_CHECKMAX

                instrip = true;
            }
            else
            {
                if (water_->wIndex_[ gridIndex ] >= 0 &&
                    water_->wIndex_[ gridIndex + water_->gridSizeX_] >= 0  &&
                    instrip)
                {
                    indices_32.push_back( uint32(water_->wIndex_[ gridIndex ] ) );
                    WIDX_CHECKMAX

                    indices_32.push_back( uint32(water_->wIndex_[ gridIndex + water_->gridSizeX_]));
                    WIDX_CHECKMAX

                    lastIndex = indices_32.back();
                }
                else
                    instrip = false;
            }

            if (lastX)
                instrip = false;

            ++gridIndex;
        }
        gridIndex += (water_->gridSizeX_ - xMax_) + xMin_;
    }

    // Process the indices
    bool remap = (maxIndex > maxVerts);
    if (remap)
    {
        vBufferIndex_ = water_->remap<T>( indices, indices_32 );
        if (vBufferIndex_ == 0) //error has occured.
        {
            nIndices_ = 0;      
            return;
        }
    }
    else
    {
        vBufferIndex_ = 0;
        indices.resize( indices_32.size() );
        for (uint i=0; i<indices_32.size() ; i++)
        {
            indices[i] = (T)indices_32[i];      
        }
    }

    // Build the D3D index buffer.
    if (indices.size() > 2)
    {
        indices.erase( indices.begin(), indices.begin() + 2 );
        nIndices_ = static_cast<uint32>( indices.size() );

        // Create the index buffer
        // The index buffer consists of one big strip covering the whole water
        // cell surface, Does not include sections of water that is under the
        // terrain. made up of individual rows of strips connected by degenerate
        // triangles.
        if( SUCCEEDED( indexBuffer_.create( nIndices_, format, D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED ) ) )
        {
            Moo::IndicesReference ir = indexBuffer_.lock( 0, nIndices_, 0 );
            if(ir.valid())
            {
                ir.fill( &indices.front(), nIndices_ );
                indexBuffer_.unlock();
            }
        }
    }
#undef WIDX_CHECKMAX
}

/**
 *  Create the water cells managed index buffer
 */
void WaterCell::createManagedIndices()
{
    BW_GUARD;
    if (!water_)
        return;
    if (Moo::rc().maxVertexIndex() <= 0xffff)
        buildIndices<uint16>();
    else
        buildIndices<uint32>();
}

// ----------------------------------------------------------------------------
// Section: Water
// ----------------------------------------------------------------------------

namespace { //anonymous

static SimpleMutex * deletionLock_ = NULL;

};


/**
 * Constructor for water.
 *
 * @param config a structure containing the configuration data for the water surface.
 * @param pCollider the collider to use to intersect the water with the scene
 */
Water::Water( const WaterState& state, const WaterResources& resources, RompColliderPtr pCollider )
:   state_( state ),
    resources_( resources ),
    gridSizeX_( int( ceilf( state.size_.x / state.tessellation_ ) + 1 ) ),
    gridSizeZ_( int( ceilf( state.size_.y / state.tessellation_ ) + 1 ) ),
    time_( 0 ),
    lastDTime_( 1.f ),
    normalTextures_( NULL ),
    screenFadeTexture_( NULL ),
    foamTexture_( NULL ),
    reflectionCubeMap_( NULL ),
    pCollider_( pCollider ),
    waterScene_( NULL),
    drawSelf_( true ),
    simulationEffect_( NULL ),
    reflectionCleared_( false ),
    paramCache_( new EffectParameterCache() ),
#ifdef EDITOR_ENABLED
    drawRed_( false ),
    highlight_( false ),
#endif
    visible_( true ),
    needSceneUpdate_( false ),
    createdCells_( false ),
    initialised_( false ),
    enableSim_( true ),
    drawMark_( 0 ),
    simpleReflection_( 0.0f ),
    outsideVisible_( false ),
    animTime_(0.0f)
{
    BW_GUARD;

    SimulationManager::init();

    if (!deletionLock_)
        deletionLock_ = new SimpleMutex;

    Waters::instance().push_back(this);

    // Resize the water buffers.
    wRigid_.resize( gridSizeX_ * gridSizeZ_, false );
    wAlpha_.resize( gridSizeX_ * gridSizeZ_, 0 );
    bankDist.resize(gridSizeX_ * gridSizeZ_, BANK_DIST_DEFAULT_VALUE);

    // Create the water transforms.
    transform_.setRotateY( state_.orientation_ );
    transform_.postTranslateBy( state_.position_ );
    invTransform_.invert( transform_ );

    if (Water::backgroundLoad())
    {
        // do heavy setup stuff in the background
        FileIOTaskManager::instance().addBackgroundTask(
            new CStyleBackgroundTask( "Water Load Task",
                &Water::doCreateTables, this,
                &Water::onCreateTablesComplete, this ) );
    }
    else
    {
        Water::doCreateTables( this );
        Water::onCreateTablesComplete( this );
    }

    static bool first = true;
    if (first)
    {
        first = false;
#ifdef DEBUG_WATER
        MF_WATCH( "Client Settings/Water/debug cell", s_debugCell_, 
                Watcher::WT_READ_WRITE,
                "simulation debugging?" );
        MF_WATCH( "Client Settings/Water/debug cell2", s_debugCell2_, 
                Watcher::WT_READ_WRITE,
                "simulation debugging?" );
        MF_WATCH( "Client Settings/Water/debug sim", s_debugSim_, 
                Watcher::WT_READ_WRITE,
                "simulation debugging?" );
#endif
        MF_WATCH( "Client Settings/Water/cell cull enable", s_cullCells_, 
                Watcher::WT_READ_WRITE,
                "enable cell culling?" );
        MF_WATCH( "Client Settings/Water/cell cull dist", s_cullDistance_, 
                Watcher::WT_READ_WRITE,
                "cell culling distance (far plane)" );

        MF_WATCH( "Render/Performance/DrawPrim WaterCell", s_enableDrawPrim,
            Watcher::WT_READ_WRITE,
            "Allow WaterCell to call drawIndexedPrimitive()." );
    }

    //Create python accessor object
    pPyVolume_ = PyObjectPtr(new PyWaterVolume( this ), PyObjectPtr::STEAL_REFERENCE);
}


/**
 * Destructor.
 */
Water::~Water()
{
    BW_GUARD;
    simulationEffect_ = NULL;
    bw_safe_delete(paramCache_);

    releaseWaterScene();

    SimulationManager::fini();

    bw_safe_delete_array( normalTextures_ );
}

/**
 *  Check the water pointer is valid for usage.
 *  Used by the background tasks to avoid bad pointers.
 *
 *  @param  water   Pointer to check.
 *
 *  @return         Validity of the pointer.
 */
bool Water::stillValid(Water* water)
{
    BW_GUARD_PROFILER( Water_stillValid );
    if (!water)
    {
        return false;
    }

    if (Water::backgroundLoad())
    {
        if (deletionLock_)
        {
            deletionLock_->grab();

            BW::vector< Water* >::iterator it =
                std::find(  Waters::instance().begin(),
                            Waters::instance().end(), water );  

            if (it == Waters::instance().end()) // deleted
            {
                deletionLock_->give();
                return false;
            }
        }
        else
            return false;
    }
    return true;
}

/**
 *  Delete a water object. Controlled destruction is required by the 
 *  background tasks.
 *
 *  The water destructor is private so all destruction should come
 *  through here.
 *
 *  @param  water   Water object to destroy.
 *
 */
void Water::deleteWater(Water* water)
{   
    BW_GUARD_PROFILER( Water_deleteWater );
    if (deletionLock_)
    {
        deletionLock_->grab();

        BW::vector< Water* >::iterator it = std::find( Waters::instance().begin( ), Waters::instance().end( ), water );
        Waters::instance().erase(it);

        delete water;
        water = NULL;


        bool deleteMutex = (Waters::instance().size() == 0);
        deletionLock_->give();
        if (deleteMutex)
        {
            bw_safe_delete(deletionLock_);
        }
    }
    else
    {
        BW::vector< Water* >::iterator it = std::find( Waters::instance().begin( ), Waters::instance().end( ), water );
        Waters::instance().erase(it);

        delete water;
        water = NULL;
    }
}


/**
 * Remove the references to the water scene
 */
void Water::releaseWaterScene()
{
    BW_GUARD;
    if (waterScene_)
    {
        waterScene_->removeOwner(this); 
        if ( s_renderTargetMap_.size() && waterScene_->refCount() == 1 )
        {
            float key = state_.position_.y;
            key = floorf(key+0.5f);
            WaterRenderTargetMap::iterator it = s_renderTargetMap_.find(key);
            WaterScene *(&a) = (it->second);
            a = NULL; //a ref
        }
        waterScene_->decRef();
        waterScene_ = NULL;
    }
}


/**
 * Recreate the water surface render data.
 */
void Water::rebuild( const WaterState& state, const WaterResources& resources )
{
    BW_GUARD;
    initialised_ = false;
    vertexBuffers_.clear();

    deleteManagedObjects();

    releaseWaterScene();

    state_ = state;
    gridSizeX_ = int( ceilf( state_.size_.x / state_.tessellation_ ) + 1 );
    gridSizeZ_ = int( ceilf( state_.size_.y / state_.tessellation_ ) + 1 );

    // Create the water transforms.
    transform_.setRotateY( state_.orientation_ );
    transform_.postTranslateBy( state_.position_ );
    invTransform_.invert( transform_ );

    if( wRigid_.size() != gridSizeX_ * gridSizeZ_ )
    {// don't clear it
        wRigid_.clear();
        wAlpha_.clear();
        bankDist.clear();

        wRigid_.resize( gridSizeX_ * gridSizeZ_, false );
        wAlpha_.resize( gridSizeX_ * gridSizeZ_, 0 );
        bankDist.resize(gridSizeX_ * gridSizeZ_, BANK_DIST_DEFAULT_VALUE);
    }

    if (state_.useEdgeAlpha_)
    {
        buildTransparencyTable();
        buildBankDistTable();
    }
    else
    {
        wRigid_.assign( wRigid_.size(), false );
        wAlpha_.assign( wAlpha_.size(), 0 );
        bankDist.assign(bankDist.size(), BANK_DIST_DEFAULT_VALUE);
    }

    wIndex_.clear();
    cells_.clear();
    activeSimulations_.clear();
    nVertices_ = createIndices();

    enableSim_ = Waters::simulationEnabled();
    enableSim_ = enableSim_ && Moo::rc().supportsTextureFormat( D3DFMT_A16B16G16R16F ) && state_.useSimulation_;

    createCells();

    setup2ndPhase();

    startResLoader();
}


/**
 * Builds the index table for a water surface.
 * Creates multiple vertex buffers when the vertices go over the 
 * max vertex index
 */
uint32 Water::createIndices( )
{
    BW_GUARD;
    // Create the vertex index table for the water cell.
    uint32 index = 0;
    int32 waterIndex = 0;
    for (int zIndex = 0; zIndex < int( gridSizeZ_ ); zIndex++ )
    {
        for (int xIndex = 0; xIndex < int( gridSizeX_ ); xIndex++ )
        {
            int realNeighbours = 0;
            for (int cz = zIndex - 1; cz <= zIndex + 1; cz++ )
            {
                for (int cx = xIndex - 1; cx <= xIndex + 1; cx++ )
                {
                    if (( cx >= 0 && cx < int( gridSizeX_ ) ) &&
                        ( cz >= 0 && cz < int( gridSizeZ_ ) ))
                    {
                        if (!wRigid_[ cx + ( cz * gridSizeX_ ) ])
                        {
                            realNeighbours++;
                        }
                    }
                }
            }

            if (realNeighbours > 0)
                wIndex_.push_back(waterIndex++);
            else
                wIndex_.push_back(-1);
        }
    }

#ifdef EDITOR_ENABLED
    if (waterIndex > 0xffff)
    {
        Commentary::instance().addMsg( "WARNING! Water surface contains excess"
                " vertices, please INCREASE the \"Mesh Size\" parameter", 1 );
    }
#endif //EDITOR

    return waterIndex;
}



#ifdef EDITOR_ENABLED

/**
 *  Delete the transparency/rigidity file.
 */
void Water::deleteData( )
{
    BW::wstring fileName;
    bw_utf8tow( BWResource::resolveFilename( resources_.transparencyTable_ ), fileName );
    ::DeleteFile( fileName.c_str() );
}


/**
 *  Little helper function to write out some data to a string..
 */
template<class C> void writeVector( BW::string& data, BW::vector<C>& v )
{
    int clen = sizeof( C );
    int vlen = clen * v.size();
    data.append( (char*) &v.front(), vlen );
}


/**
 *  Writes a 32 bit uint to a string (in 4 chars)
 */
void writeValue(BW::string& data, uint32 value )
{
    data.push_back( char( (value & (0xff << 0 ))    ) );
    data.push_back( char( (value & (0xff << 8 ))>>8 ) );
    data.push_back( char( (value & (0xff << 16))>>16) );
    data.push_back( char( (value & (0xff << 24))>>24) );
}


/**
 *  Writes a bool to a string
 */
void writeValue(BW::string& data, bool value )
{
    data.push_back( char( value ) );
}


/**
 *  Compresses a vector using very simple RLE
 */
template <class C>
void compressVector( BW::string& data, BW::vector<C>& v )
{
    MF_ASSERT(v.size());
    uint32 imax = static_cast<uint32>(v.size());    
    uint32 prev_i = 0;
    bool first = true;

    C cur_v = v[0];
    C prev_v = !cur_v;
    for (uint32 i=0; i<imax; i++)
    {
        cur_v = v[i];
        uint32 cur_i = i;    
        uint32 c = cur_i - prev_i;

        // test for end of run cases
        if ( (cur_v != prev_v) || (c > 254) || (i==(imax-1)) )
        {
            if (prev_v == static_cast<C>(RLE_KEY_VALUE)) // exception for keyvalue
            {
                data.push_back(static_cast<char>(RLE_KEY_VALUE));
                writeValue(data, prev_v);
                data.push_back( static_cast<char>(c) );
            }
            else
            {
                if (c > 2)
                {
                    data.push_back(static_cast<char>(RLE_KEY_VALUE));                   
                    writeValue(data, prev_v);
                    data.push_back( static_cast<char>(c) );
                }
                else
                {
                    if (first==false)
                    {
                        if ( c > 1 )
                            writeValue(data, prev_v);
                        writeValue(data, prev_v);
                    }
                }
            }
            prev_i = cur_i;
        }
        prev_v = cur_v;
        first=false;
    }
    writeValue(data, cur_v);
}


/**
 *  Save rigidity and alpha tables to a file.
 */
void Water::saveTransparencyTable( )
{
    BW::string data1;
    size_t maxSize = data1.max_size();

    // Build the required data...
    compressVector<uint32>(data1, wAlpha_);
    BW::string data2;
    compressVector<bool>(data2, wRigid_);

    BinaryPtr binaryBlockAlpha = new BinaryBlock( data1.data(), data1.size(), "BinaryBlock/Water" );
    BinaryPtr binaryBlockRigid = new BinaryBlock( data2.data(), data2.size(), "BinaryBlock/Water" );

    //Now copy it into the data file
    BW::string dataName = resources_.transparencyTable_;
    DataSectionPtr pSection = BWResource::openSection( dataName, false );
    if ( !pSection )
    {
        // create a section
        BW::string::size_type lastSep = dataName.find_last_of('/');
        BW::string parentName = dataName.substr(0, lastSep);
        DataSectionPtr parentSection = BWResource::openSection( parentName );
        MF_ASSERT(parentSection);

        BW::string tagName = dataName.substr(lastSep + 1);

        // make it
        pSection = new BinSection( tagName, new BinaryBlock( NULL, 0, "BinaryBlock/Water" ) );
        pSection->setParent( parentSection );
        pSection = DataSectionCensus::add( dataName, pSection );
    }


    MF_ASSERT( pSection );
    pSection->delChild( "alpha" );
    DataSectionPtr alphaSection = pSection->openSection( "alpha", true );
    alphaSection->setParent(pSection);
    if ( !pSection->writeBinary( "alpha", binaryBlockAlpha ) )
    {
        CRITICAL_MSG( "Water::saveTransparencyTable - error while writing BinSection in %s/alpha\n", dataName.c_str() );
        return;
    }

    pSection->delChild( "rigid" );
    DataSectionPtr rigidSection = pSection->openSection( "rigid", true );
    rigidSection->setParent(pSection);
    if ( !pSection->writeBinary( "rigid", binaryBlockRigid ) )
    {
        CRITICAL_MSG( "Water::saveTransparencyTable - error while writing BinSection in %s/rigid\n", dataName.c_str() );
        return;
    }

    pSection->delChild( "version" );
    DataSectionPtr versionSection = pSection->openSection( "version", true );
    int version = 2;
    BinaryPtr binaryBlockVer = new BinaryBlock( &version, sizeof(int), "BinaryBlock/Water" );
    versionSection->setParent(pSection);
    if ( !pSection->writeBinary( "version", binaryBlockVer ) )
    {
        CRITICAL_MSG( "Water::saveTransparencyTable - error while writing BinSection in %s/version\n", dataName.c_str() );
        return;
    }

    // Now actually save...
    versionSection->save();
    alphaSection->save();
    rigidSection->save();
    pSection->save();

    // Make sure we break any circular references
    alphaSection->setParent( NULL );
    rigidSection->setParent( NULL );
    versionSection->setParent( NULL );
}

#endif //EDITOR_ENABLED


/**
 *  Read a uint32 from a 4 chars in a block
 *  - updates a referenced index.
 */
void readValue( const char* pCompressedData, uint32& index, uint32& value )
{
    char p1 = pCompressedData[index]; index++;  
    char p2 = pCompressedData[index]; index++;
    char p3 = pCompressedData[index]; index++;
    char p4 = pCompressedData[index]; index++;

    value = p1 & 0xff;
    value |= ((p2 << 8  ) & 0xff00);
    value |= ((p3 << 16 ) & 0xff0000);
    value |= ((p4 << 24 ) & 0xff000000);
}

#pragma warning( push )
#pragma warning( disable : 4800 ) //ignore the perf warning..

/**
 *  Read a bool from a char in a block
 *  - updates a referenced index.
 */
void readValue( const char* pCompressedData, uint32& index, bool& value )
{
    value = static_cast<bool>(pCompressedData[index]); index++; 
}

#pragma warning( pop )


/**
 *  Decompress a char block to vector using the RLE above.
 */
template< class C >
void decompressVector( const char* pCompressedData, uint32 length, BW::vector<C>& v )
{
    uint32 i = 0;
    while (i < length)
    {
        // identify the RLE key
        if (pCompressedData[i] == char(RLE_KEY_VALUE))
        {
            i++;
            C val=0;
            readValue(pCompressedData, i, val);
            uint c = uint( pCompressedData[i] & 0xff ); 
            i++;

            // unfold the run..
            for (uint j=0; j < c; j++)
            {
                v.push_back(val);
            }
        }
        else
        {
            C val=0;
            readValue(pCompressedData, i, val);
            v.push_back( val );
        }
    }
}

/**
 *  Load the previously saved transparency/rigidity data.
 */
bool Water::loadTransparencyTable( )
{
    BW_GUARD;
    BW::string sectionName = resources_.transparencyTable_;
    DataSectionPtr pSection =
            BWResource::instance().rootSection()->openSection( sectionName );

    if (!pSection)
        return false;

    BinaryPtr pVersionData = pSection->readBinary( "version" );
    int fileVersion=-1;
    if (pVersionData)
    {
        const int* pVersion = reinterpret_cast<const int*>(pVersionData->data());
        fileVersion = pVersion[0];
    }

    BinaryPtr pAlphaData = pSection->readBinary( "alpha" );
    if (fileVersion < 2)
    {
        const uint32* pAlphaValues = reinterpret_cast<const uint32*>(pAlphaData->data());
        int nAlphaValues = pAlphaData->len() / sizeof(uint32);
        wAlpha_.assign( pAlphaValues, pAlphaValues + nAlphaValues );
    }
    else
    {
        wAlpha_.clear();
        const char* pCompressedValues = reinterpret_cast<const char*>(pAlphaData->data());
        decompressVector<uint32>(pCompressedValues, pAlphaData->len(), wAlpha_);
    }

    BinaryPtr pRigidData = pSection->readBinary( "rigid" );
    if (fileVersion < 2)
    {
        const bool* pRigidValues = reinterpret_cast<const bool*>(pRigidData->data());
        int nRigidValues = pRigidData->len() / sizeof(bool); //not needed.....remove..
        wRigid_.assign( pRigidValues, pRigidValues + nRigidValues );
    }
    else
    {
        wRigid_.clear();
        const char* pCompressedValues = reinterpret_cast<const char*>(pRigidData->data());
        decompressVector<bool>(pCompressedValues, pRigidData->len(), wRigid_);
    }

    return true;
}

#define MAX_DEPTH 100.f

uint32 encodeDepth( float vertexHeight, float terrainHeight )
{
//  float delta = Math::clamp<float>(0.f, vertexHeight - terrainHeight, MAX_DEPTH);
//  delta = delta / MAX_DEPTH;
//
//  return uint32( delta*255.f ) << 24;

    float h = 155.f + 100.f * min( 1.f, max( 0.f, (1.f - ( vertexHeight - terrainHeight ))));
    h = h/255.f;                    
    h = Math::clamp<float>( 0.f, (h-0.5f)*2.f, 1.f );
    h = 1.f-h;
    h = powf(h,2.f)*2.f;
    h = min( h, 1.f);
    return uint32( h*255.f ) << 24;
}

/**
 *  Create rigidity and alpha tables.
 */
void Water::buildTransparencyTable( )
{
    BW_GUARD;
    float z = -state_.size_.y * 0.5f;
    uint32 index = 0;
    bool solidEdges =   (state_.size_.x <= (state_.tessellation_*2.f)) ||
                        (state_.size_.y <= (state_.tessellation_*2.f));


    float depth = 10.f;

    for (uint32 zIndex = 0; zIndex < gridSizeZ_; zIndex++ )
    {
        if ((zIndex+1) == gridSizeZ_)
            z = state_.size_.y * 0.5f;

        float x = -state_.size_.x * 0.5f;
        for (uint32 xIndex = 0; xIndex < gridSizeX_; xIndex++ )
        {
            if ((xIndex+1) == gridSizeX_)
                x = state_.size_.x * 0.5f;

            Vector3 v = transform_.applyPoint( Vector3( x, 0.1f, z ) );

            //TODO: determine the depth automatically
            //float height = pCollider_ ? pCollider_->ground( v ) : (v.y - 100);
            //if (height > waterDepth_

            //if (xIndex == 0 ||
            //  zIndex == 0 ||
            //  xIndex == ( gridSizeX_ - 1 ) ||
            //  zIndex == ( gridSizeZ_ - 1 ))
            //{
            //  // Set all edge cases to be completely transparent and rigid.               
            //  if (solidEdges)
            //  {
            //      wRigid_[ index ] = false;
            //      wAlpha_[ index ] = ( 255UL ) << 24;
            //  }
            //  else
            //  {
            //      wRigid_[ index ] = false;               
            //      wAlpha_[ index ] = 0x00000000;
            //  }
            //}
            //else
            //{
                // Get the terrain height, and calculate the transparency of this water grid point
                // to be the height above the terrain.
                float height = pCollider_ ? pCollider_->ground( v ) : (v.y - MAX_DEPTH);

                if (height == RompCollider::NO_GROUND_COLLISION)
                {
                    wRigid_[ index ] = false;
                    wAlpha_[ index ] = ( 255UL ) << 24;
                }
                else if ( height > v.y )
                {
                    wRigid_[ index ] = true;
                    wAlpha_[ index ] = 0x00000000;
                }
                else
                {
                    float delta = v.y - height;
                    if (delta > depth)
                        depth = (v.y-height);

                    wRigid_[ index ] = false;
                    wAlpha_[ index ] = encodeDepth(v.y, height);
                }
            //}

            ++index;
            x += state_.tessellation_;
        }
        z += state_.tessellation_;
    }

    state_.depth_ = depth;
}

#define IS_BANK_LINE(xIndex, zIndex, data) \
    data[ xIndex + ( zIndex * gridSizeX_ ) ] && \
(((xIndex + 1) < gridSizeX_ && !data[xIndex + 1 + ( zIndex * gridSizeX_ )])   ||\
 ((zIndex + 1) < gridSizeZ_ && !data[xIndex + ( (zIndex + 1) * gridSizeX_ )]) ||\
 ( xIndex     > 0           && !data[xIndex - 1 + ( zIndex * gridSizeX_ )])   ||\
 ( zIndex     > 0           && !data[xIndex + ( (zIndex - 1) * gridSizeX_ )]))

// create table of distances from bank. Is needed for foam movements
void Water::buildBankDistTable()
{
    BW_GUARD;

    struct BWPolygon
    {
        BW::list<Vector2> vertices;
        inline void Add(float x, float y)
        {
            vertices.push_back(Vector2(x, y));
        }
        // returns false if none of bank line vertices passed interpolation factor or result dir == 0
        inline bool DirectionAndDistance(Vector2 pos, Vector2& dir, float& minDist) 
        {
            bool result = false;
            dir.x = 0.0f;
            dir.y = 0.0f;
            minDist = 100000000.0f;
            for ( BW::list<Vector2>::iterator it  = vertices.begin(); it != vertices.end(); it++ )
            {
                Vector2 delta = (*it) - pos;
                float dist = delta.lengthSquared();
                if(dist < Water::s_fBankLineInterpolationFactor && dist > 0.0f)
                {
                    dir += delta/(dist*dist);
                    if(dist < minDist)
                        minDist = dist;
                    result = true;
                }
            }
            if(result)
            {
                dir.normalise();
                minDist = sqrt(minDist);
            }
            return result;
        }
    } poly;

    WaterRigid rigidCopy(wRigid_);

    // prune line the shore to hide artifacts
    for(uint32 zIndex = 0; zIndex < gridSizeZ_; zIndex++)
        for (uint32 xIndex = 0; xIndex < gridSizeX_; xIndex++ )
            if( IS_BANK_LINE(xIndex, zIndex, wRigid_) )
                rigidCopy[xIndex + zIndex * gridSizeX_] = false;

    float z = -state_.size_.y * 0.5f;

    // get all border verts
    for (uint32 zIndex = 0; zIndex < gridSizeZ_; zIndex++ )
    {
        if ((zIndex+1) == gridSizeZ_)
            z = state_.size_.y * 0.5f;

        float x = -state_.size_.x * 0.5f;
        for (uint32 xIndex = 0; xIndex < gridSizeX_; xIndex++ )
        {
            if ((xIndex+1) == gridSizeX_)
                x = state_.size_.x * 0.5f;

            if( IS_BANK_LINE(xIndex, zIndex, rigidCopy) )
            {
                poly.Add(x, z);
            }
            x += state_.tessellation_;
        }
        z += state_.tessellation_;
    }

    z = -state_.size_.y * 0.5f;
    for (uint32 zIndex = 0; zIndex < gridSizeZ_; zIndex++ )
    {
        if ((zIndex+1) == gridSizeZ_)
            z = state_.size_.y * 0.5f;

        float x = -state_.size_.x * 0.5f;
        for (uint32 xIndex = 0; xIndex < gridSizeX_; xIndex++ )
        {
            if ((xIndex+1) == gridSizeX_)
                x = state_.size_.x * 0.5f;

            Vector2 pos(x,z);
            Vector2 dir;
            float dist;
            size_t bankDistIdx = xIndex + ( zIndex * gridSizeX_ );
            if( poly.DirectionAndDistance(pos, dir, dist) )
            {
                bankDist[ bankDistIdx ].second = dist;
                if(!rigidCopy[bankDistIdx])
                    bankDist[bankDistIdx].first.set(dir.x, dir.y);
                else
                    bankDist[bankDistIdx].first.set(-dir.x, -dir.y);
            }
            else
            {
                bankDist[bankDistIdx] = BANK_DIST_DEFAULT_VALUE;
            }


            x += state_.tessellation_;
        }
        z += state_.tessellation_;
    }
}

/**
 *  Generate the required simulation cell divisions.
 */
void Water::createCells()
{
    BW_GUARD;
    if ( enableSim_ )
    {
        float simGridSize =  ( ceilf( state_.simCellSize_ / state_.tessellation_ ) * state_.tessellation_);

        for (float y=0.f;y<state_.size_.y; y+=simGridSize)
        {
            Vector2 actualSize(simGridSize,simGridSize);
            if ( (y+simGridSize) > state_.size_.y )
                actualSize.y = (state_.size_.y - y);

            //TODO: if the extra bit is really small... just enlarge the current cell??? hmmmm
            for (float x=0.f;x<state_.size_.x; x+=simGridSize)          
            {
                WaterCell newCell;
                if ( (x+simGridSize) > state_.size_.x )
                    actualSize.x = (state_.size_.x - x);

                newCell.init( this, Vector2(x,y), actualSize );
                cells_.push_back( newCell );
            }
        }
    }
    else
    {
        // If the simulation is disabled, dont divide the surface.. just use one cell..
        Vector2 actualSize(state_.size_.x, state_.size_.y);
        WaterCell newCell;
        newCell.init( this, Vector2(0.f,0.f), actualSize );
        cells_.push_back( newCell );
    }
}


/**
 *  Create the data tables
 */
void Water::doCreateTables( void* self )
{
    BW_GUARD;

    Water * selfWater = static_cast< Water * >( self );
    if (Water::stillValid(selfWater))
    {
        // if in the editor: create the transparency table.. else load it...
#ifdef EDITOR_ENABLED

        // Build the transparency information
        if (!selfWater->loadTransparencyTable())
        {
            selfWater->buildTransparencyTable();
        }

#else

        if (!selfWater->loadTransparencyTable())
        {
            selfWater->state_.useEdgeAlpha_ = false;
        }

#endif

        selfWater->enableSim_ = Waters::simulationEnabled();

        selfWater->buildBankDistTable();

        // Build the water cells
        selfWater->nVertices_ = selfWater->createIndices();
        selfWater->enableSim_ = selfWater->enableSim_ && Moo::rc().supportsTextureFormat( D3DFMT_A16B16G16R16F ) && selfWater->state_.useSimulation_;

        // Create the FX shaders in the background too..
        bool res=false;
        static bool first=true;

        if (Waters::s_simulationEffect_ == NULL)
            selfWater->enableSim_ = false;
        else
            selfWater->simulationEffect_ = Waters::s_simulationEffect_;

        if (Waters::s_effect_ == NULL)
            selfWater->drawSelf_ = false;
        selfWater->createCells();

        if (Water::backgroundLoad())
            deletionLock_->give();
    }
}


/**
 *  This is called on the main thread, after the rigidity and alpha tables
 *  have been computed. Starts the second phase of the water setup.
 */
void Water::onCreateTablesComplete( void * self )
{
    BW_GUARD;

    Water * selfWater = static_cast< Water * >( self );
    if (Water::stillValid(selfWater))
    {
        selfWater->setup2ndPhase();

        if (Water::backgroundLoad())
            deletionLock_->give();

        selfWater->startResLoader();
    }
}


/**
 *  Start the resource loader BG task.
 */
void Water::startResLoader( )
{
    BW_GUARD;
    if (Water::backgroundLoad())
    {
        // load resources in the background
        FileIOTaskManager::instance().addBackgroundTask(
            new CStyleBackgroundTask( "Water Res Loader Task",
                &Water::doLoadResources, this,
                &Water::onLoadResourcesComplete, this ) );
    }
    else
    {
        Water::doLoadResources( this );
        Water::onLoadResourcesComplete( this );
    }
}


/**
 *  Second phase of the water setup.
 */
void Water::setup2ndPhase( )
{
    BW_GUARD;
    static DogWatch w3( "Material" );
    w3.start();

    DEBUG_MSG( "Water::Water: using %d vertices out of %d\n",
        nVertices_, gridSizeX_ * gridSizeZ_ );

    float simGridSize =   ceilf( state_.simCellSize_ / state_.tessellation_ ) * state_.tessellation_;


    int cellX = int(ceilf(state_.size_.x / simGridSize));
    int cellY = int(ceilf(state_.size_.y / simGridSize)); //TODO: need to use the new cell size! :D
    uint cellCount = static_cast<uint>( cells_.size() );

    for (uint i=0; i<cellCount; i++)
    {
        cells_[i].initSimulation( MAX_SIM_TEXTURE_SIZE, state_.simCellSize_ );

        //TODO: fix this initialisation...
        if (enableSim_)
        {
            int col = (i/cellX)*cellX;
            int negX = i - 1;
            int posX = i + 1;
            int negY = i - cellX;
            int posY = i + cellX;

            cells_[i].initEdge( 1, (negX < col)                 ?   NULL    : &cells_[negX] );
            cells_[i].initEdge( 0, (negY < 0)                   ?   NULL    : &cells_[negY] );
            cells_[i].initEdge( 3, (posX >= (col+cellX))        ?   NULL    : &cells_[posX] );
            cells_[i].initEdge( 2, (uint(posY) >= cellCount)    ?   NULL    : &cells_[posY] );
        }
    }

    // Create the render targets
    // The WaterSceneRenderer are now grouped based on the height of the 
    //  water plane (maybe extend to using the whole plane?)
    float key = state_.position_.y;
    key = floorf(key+0.5f);
    if ( s_renderTargetMap_.find(key) == s_renderTargetMap_.end())
    {
        s_renderTargetMap_[key] = NULL;
    }

    WaterRenderTargetMap::iterator it = s_renderTargetMap_.find(key);

    WaterScene *(&a) = (it->second);
    if (a == NULL)
        a = new WaterScene( state_.position_[1], state_.waveHeight );

    waterScene_ = (it->second);
    waterScene_->incRef();
    waterScene_->addOwner(this);

    //  // Create the bounding box.
    bb_.setBounds(  
        Vector3(    -state_.size_.x * 0.5f,
                    -1.f,
                    -state_.size_.y * 0.5f ),
        Vector3(    state_.size_.x * 0.5f,
                    1.f,
                    state_.size_.y * 0.5f ) );

    // And the volumetric bounding box ... the water is 10m deep
    //TODO: modify this to use the actual depth...... 

    //TODO: determine the depth of the water and use that for the BB
    //go through all the points and find the lowest terrain point.... use that for the BB depth

    bbDeep_ = bb_;
    bbDeep_.setBounds( bbDeep_.minBounds() - Vector3(0.f,2.f,0.f), bbDeep_.maxBounds() );

    bbActual_.setBounds(    
        Vector3(    -state_.size_.x * 0.5f,
                    -state_.depth_,
                    -state_.size_.y * 0.5f ),
        Vector3(    state_.size_.x * 0.5f,
                    0.f,
                    state_.size_.y * 0.5f ) );


    w3.stop();
    static DogWatch w4( "Finalisation" );
    w4.start();

    createUnmanagedObjects();

    // Create the managed objects.
    createManagedObjects();

    w4.stop();
}




/**
 *  Loads all resources needed by the water. To avoid stalling 
 *  the main thread, this should be done in a background task.
 */
void Water::doLoadResources( void * self )
{
    BW_GUARD;
    Moo::ScopedResourceLoadContext resLoadCtx( "water" );

    Water * selfWater = static_cast< Water * >( self );
    if (Water::stillValid(selfWater))
    {
        Moo::TextureManager::instance()->getAnimationSet(selfWater->resources_.waveTexture_, &(selfWater->normalTextures_), selfWater->state_.waveTextureNumber_);


        selfWater->screenFadeTexture_ = Moo::TextureManager::instance()->get(s_screenFadeMap);
        selfWater->foamTexture_ = Moo::TextureManager::instance()->get(selfWater->resources_.foamTexture_);
        selfWater->reflectionCubeMap_ = Moo::TextureManager::instance()->get(selfWater->resources_.reflectionTexture_);
        SimulationManager::instance().loadResources();

        if (Water::backgroundLoad())
            deletionLock_->give();
    }
}


/**
 *  This is called on the main thread, after the resources 
 *  have been loaded. Sets up the texture stages.
 */
void Water::onLoadResourcesComplete( void * self )
{
    BW_GUARD;
    Water * selfWater = static_cast< Water * >( self );
    if (Water::stillValid(selfWater))
    {
        if (selfWater && Waters::s_effect_)
        {
            selfWater->paramCache_->effect(Waters::s_effect_->pEffect()->pEffect());

            // Animate normal map texture
            int timeInt = (int)floor(selfWater->animTime_ * selfWater->state_.animationSpeed_);
            int animFrameNum1 = timeInt % selfWater->state_.waveTextureNumber_;
            int animFrameNum2 = (timeInt+1) % selfWater->state_.waveTextureNumber_;
            if(selfWater->normalTextures_)
            {
                if(selfWater->normalTextures_[animFrameNum1].exists())
                    selfWater->paramCache_->setTexture("normalMap1", selfWater->normalTextures_[animFrameNum1]->pTexture());
                if(selfWater->normalTextures_[animFrameNum2].exists())
                    selfWater->paramCache_->setTexture("normalMap2", selfWater->normalTextures_[animFrameNum2]->pTexture());
                selfWater->paramCache_->setFloat("animationInterpolator", selfWater->animTime_ - animFrameNum1);
            }
                
            if (selfWater->screenFadeTexture_)
                selfWater->paramCache_->setTexture("screenFadeMap", selfWater->screenFadeTexture_->pTexture());
            if (selfWater->foamTexture_)
                selfWater->paramCache_->setTexture("foamMap", selfWater->foamTexture_->pTexture());
            if (selfWater->reflectionCubeMap_ 
                && WaterSceneRenderer::s_simpleScene_ )
                selfWater->paramCache_->setTexture("reflectionCubeMap", selfWater->reflectionCubeMap_->pTexture());
        }

        if (selfWater && Waters::s_simulationEffect_)
        {
            ComObjectWrap<ID3DXEffect> pEffect = Waters::s_simulationEffect_->pEffect()->pEffect();

            float borderSize = Waters::s_highQuality_ ? (float)SIM_BORDER_SIZE : 0.0f;
            float pixSize = 1.0f / 256.0f;
            float pix2 = 2.0f * borderSize * pixSize;

            Vector4 cellInfo( borderSize * pixSize, 1 - (borderSize*pixSize), pix2, 0.0f);
            pEffect->SetVector("g_cellInfo", &cellInfo);
        }

        if (Water::backgroundLoad())
            deletionLock_->give();
    }
}


/**
 *  
 */
void Water::deleteUnmanagedObjects( )
{
    BW_GUARD;
    if (waterScene_)
    {
        waterScene_->deleteUnmanagedObjects();
    }
    s_rt = NULL;
}


/**
 *  Create all unmanaged resources
 */
void Water::createUnmanagedObjects( )
{
    BW_GUARD;
    if (Waters::s_effect_)
    {
        SimulationCell::createUnmanaged();

        WaterCell::SimulationCellPtrList::iterator cellIt = activeSimulations_.begin();
        for (; cellIt != activeSimulations_.end(); cellIt++)
        {
            //activeList.remove( this );
            (*cellIt)->deactivate();
            (*cellIt)->edgeActivation(false);
        }
        activeSimulations_.clear();
    }

    if (waterScene_)
    {
        waterScene_->recreate();
    }

    const D3DSURFACE_DESC& bbDesc = Moo::rc().backBufferDesc();
    D3DFORMAT format = bbDesc.Format;
    if(Renderer::pInstance()->pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
        format = D3DFMT_A16B16G16R16F;
    bool inited = createRenderTarget(s_rt, bbDesc.Width, bbDesc.Height, format, "Water bbCopy", true);
}


/**
 *  Delete managed objects
 */
void Water::deleteManagedObjects( )
{
    BW_GUARD;
    for (uint i=0; i<cells_.size(); i++)
    {
        cells_[i].deleteManaged();
    }
    s_quadVertexBuffer_ = NULL;     
}


/**
 *  Create managed objects
 */
void Water::createManagedObjects( )
{
    BW_GUARD;
    if (Moo::rc().device())
    {
        for (uint i=0; i<cells_.size(); i++)
        {
            cells_[i].createManagedIndices();
        }
        createdCells_ = true;

        //Create a quad for simulation renders
        typedef VertexBufferWrapper< VERTEX_TYPE > VBufferType;
        s_quadVertexBuffer_ = new VBufferType;

        if (s_quadVertexBuffer_->reset( 4 ) && 
            s_quadVertexBuffer_->lock())
        {
            VBufferType::iterator vbIt  = s_quadVertexBuffer_->begin();
            VBufferType::iterator vbEnd = s_quadVertexBuffer_->end();

            // Load the vertex data
            vbIt->pos_.set(-1.f, 1.f, 0.f);
            vbIt->colour_ = 0xffffffff;
            vbIt->uv_.set(0,0);
            ++vbIt;

            vbIt->pos_.set(1.f, 1.f, 0.f);
            vbIt->colour_ = 0xffffffff;
            vbIt->uv_.set(1,0);
            ++vbIt;

            vbIt->pos_.set(1,-1, 0);
            vbIt->colour_ = 0xffffffff;
            vbIt->uv_.set(1,1);
            ++vbIt;

            vbIt->pos_.set(-1,-1,0);
            vbIt->colour_ = 0xffffffff;
            vbIt->uv_.set(0,1);
            ++vbIt;

            s_quadVertexBuffer_->unlock();
        }
        else
        {
            ERROR_MSG(
                "Water::createManagedObjects: "
                "Could not create/lock vertex buffer\n");
        }

        MF_ASSERT( s_quadVertexBuffer_.getObject() != 0 );
    }
    initialised_=false;
    vertexBuffers_.clear();
}


/**
 *  Render the simulations for all the active cells in this water surface.
 */
void Water::renderSimulation(float dTime)
{
    BW_GUARD;
    if (!simulationEffect_)
        return;

    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );

    static DogWatch simulationWatch( "Simulation" );
    simulationWatch.start();

    simulationEffect_->checkEffectRecompiled();

    static bool first = true;
    if (first || SimulationCell::s_hitTime==-2.0)
    {
        resetSimulation();
        first = false;
    }
    simulationEffect_->hTechnique( "water_simulation" );

    //TODO: move the rain simulation stuff out of the individual water simulation and
    // expose the resulting texture for use elsewhere....
    if (raining_)
    {
        static uint32 rainMark = 0;
        if (rainMark != Waters::s_nextMark_)
        {
            rainMark = Waters::s_nextMark_;
            SimulationManager::instance().simulateRain( dTime, Waters::instance().rainAmount(), simulationEffect_ );
        }
    }

    //TODO:
    // -perturbations should not be passed to a cell if its too far away from the camera
    // -cell with the closest perturbation to the camera should take priority (along with its neighbours)

    //TODO: only check activity when in view...however, keep simulating existing active cells
    // when the water is not visible (will automatically time out)
    //if (visible_)
    //{
        WaterCell::WaterCellVector::iterator cellIt = cells_.begin();
        WaterCell::WaterCellPtrList::iterator wCellIt;
        for (; cellIt != cells_.end(); cellIt++)
        {
            (*cellIt).checkActivity( activeSimulations_, edgeList_ );

            WATER_STAT(s_movementCount += (*cellIt).movements().size());
        }

        // this list is used to activate neighbouring cells
        for (wCellIt = edgeList_.begin(); wCellIt != edgeList_.end(); wCellIt++)
        {
            (*wCellIt)->checkEdgeActivity( activeSimulations_ );
        }
        edgeList_.clear();
    //}

    ComObjectWrap<ID3DXEffect> pEffect = simulationEffect_->pEffect()->pEffect();

    Vector4 psSimulationPositionWeighting(state_.consistency_ + 1.0f, state_.consistency_, 0.0f, 0.0f);
    pEffect->SetVector("psSimulationPositionWeighting", &psSimulationPositionWeighting);
    
    //-- set cell info every frame.
    {
        float borderSize = Waters::s_highQuality_ ? (float)SIM_BORDER_SIZE : 0.0f;
        float pixSize = 1.0f / 256.0f;
        float pix2 = 2.0f * borderSize * pixSize;
        const D3DXVECTOR4 cellInfo( borderSize * pixSize, 1 - (borderSize*pixSize), pix2, 0.0f);

        pEffect->SetVector( "g_cellInfo", &cellInfo );
    }
    

    if (activeSimulations_.size())
    {
        // render simulation to each simulation texture block
        WaterCell::SimulationCellPtrList::iterator it = activeSimulations_.begin();     
        for (; it != activeSimulations_.end(); it++)
        {
            if ((*it))
            {
                //if ((*it)->shouldDeactivate())
                //{
                //  (*it)->deactivate();
                //  (*it)->edgeActivation(false);
                //  it = activeSimulations_.erase(it);              
                //}

                WATER_STAT(s_activeEdgeCellCount = (*it)->edgeActivation() ? s_activeEdgeCellCount + 1 : s_activeEdgeCellCount);
                WATER_STAT(s_activeCellCount++);

                // only sim + tick if not already ticked
                if ((*it)->mark() < Waters::s_nextMark_)
                {
                    (*it)->simulate( simulationEffect_, dTime, Waters::instance() );
                    (*it)->tick();
                    (*it)->mark( Waters::s_nextMark_ );
                }
            }
        }

        if (Waters::s_highQuality_ &&
            Waters::instance().simulationEnabled()) // only do for high detail settings
        {
            simulationEffect_->hTechnique( "simulation_edging" );
            for (it = activeSimulations_.begin(); it != activeSimulations_.end(); it++)
            {
                if ((*it) && (*it)->mark() == Waters::s_nextMark_)
                {
                    (*it)->stitchEdges( simulationEffect_, dTime, Waters::instance() );
                }
            }
        }

    }
    simulationWatch.stop();
    Moo::rc().popRenderState();
}


/**
 *  This method resets all the simulation cells.
 */
void Water::resetSimulation()
{
    BW_GUARD;
    for (uint i=0; i<cells_.size(); i++)
        cells_[i].clear();
}

/**
 * This clear the reflection render target.
 */
void Water::clearRT()
{
    BW_GUARD;
    Moo::RenderTargetPtr rt = waterScene_->reflectionTexture();

    if (rt && rt->push())
    {
        Moo::rc().beginScene();

        Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            RGB( 255, 255, 255 ), 1, 0 );

        Moo::rc().endScene();

        rt->pop();
    }
}


/**
 * This method initialises the mesh data for the water surface.
 */
bool Water::init()
{
    BW_GUARD;
    if (initialised_)
        return true;

    if ( waterScene_ )
    {
        if ( !waterScene_->recreate() )
        {
        //  ERROR_MSG(  "Water::init()"
        //                  " couldn't setup render targets" ); 

            return false;
        }
    }
    else
        return false;



    initialised_ = true;
    vertexBuffers_.clear();
    vertexBuffers_.resize(remappedVerts_.size() + 1);
    // build multiple vertex buffers based on the remapping data.
    // if nVertices_ is greater than the max allowed, break up the buffer.


    typedef VertexBufferWrapper< VERTEX_TYPE > VBufferType;

    for (uint i=0; i<vertexBuffers_.size(); i++)
    {
        vertexBuffers_[i] = new VBufferType;
    }
    VertexBufferPtr pVerts0 = vertexBuffers_[0];
    uint32 nVert0=0;
    if (remappedVerts_.size())
        nVert0 = 0xffff + 1;
    else
        nVert0 = nVertices_;

    bool success = pVerts0->reset( nVert0 ) && pVerts0->lock();
    for ( uint i=1; i<vertexBuffers_.size(); i++ )
    {
        success = success &&
            vertexBuffers_[i]->reset((int)remappedVerts_[i-1].size()) &&
            vertexBuffers_[i]->lock();
    }

    if (success)
    {
        VBufferType::iterator vb0It  = pVerts0->begin();
        VBufferType::iterator vb01End = pVerts0->end();

        uint32 index = 0;
        WaterAlphas::iterator waterAlpha = wAlpha_.begin();
        BankDist::iterator bankDirIt = bankDist.begin();
        
        float z = -state_.size_.y * 0.5f;
        float zT = 0.f;

        for (uint32 zIndex = 0; zIndex < gridSizeZ_; zIndex++)
        {
            if ((zIndex+1) == gridSizeZ_)
            {
                z = state_.size_.y * 0.5f;
                zT = state_.size_.y;
            }

            float x = -state_.size_.x * 0.5f;
            float xT = 0.f;
            for (uint32 xIndex = 0; xIndex < gridSizeX_; xIndex++)
            {
                if ((xIndex+1) == gridSizeX_)
                {
                    x = state_.size_.x * 0.5f;
                    xT = state_.size_.x;
                }

                if (wIndex_[ index ] >= 0)
                {
                    //check if it's been re-mapped....
                    // and put it in the second buffer it it has
                    // if it's less than the current max.. add it here too.
                    for ( uint vIdx=0; vIdx<remappedVerts_.size(); vIdx++ )
                    {
                        BW::map<uint32, uint32>& mapping =
                                                remappedVerts_[vIdx];
                        VertexBufferPtr pVertsX = vertexBuffers_[vIdx+1];

                        BW::map<uint32, uint32>::iterator it = 
                                    mapping.find( wIndex_[ index ] );
                        if ( it != mapping.end() )
                        {                   
                            //copy to buffer
                            uint32 idx = it->second;
                            VERTEX_TYPE&  pVert = (*pVertsX)[idx];

                            // Set the position of the water point.
                            pVert.pos_.set( x, 0, z );
                            if (state_.useEdgeAlpha_)
                                pVert.colour_ = *(waterAlpha);
                            else
                                pVert.colour_ = uint32(0xffffffff);

                            pVert.uv_.set( xT / state_.textureTessellation_, zT / state_.textureTessellation_);
                            pVert.normal_.set(bankDirIt->first.x, bankDirIt->first.y, bankDirIt->second);
                        }
                    }
                    if ( wIndex_[ index ] <= (int32)Moo::rc().maxVertexIndex() )
                    {
                        //copy to buffer one.
                        // Set the position of the water point.
                        vb0It->pos_.set( x, 0, z );

                        if (state_.useEdgeAlpha_)
                            vb0It->colour_ = *(waterAlpha);
                        else
                            vb0It->colour_ = uint32(0xffffffff);

                        vb0It->uv_.set( xT / state_.textureTessellation_, zT / state_.textureTessellation_);
                        vb0It->normal_.set(bankDirIt->first.x, bankDirIt->first.y, bankDirIt->second);
                        ++vb0It;
                    }
                }

                ++waterAlpha;
                ++bankDirIt;
                ++index;
                x += state_.tessellation_;
                xT += state_.tessellation_;
            }
            z += state_.tessellation_;
            zT += state_.tessellation_;
        }
        for ( uint i=0; i<vertexBuffers_.size(); i++ )
        {
            vertexBuffers_[i]->unlock();
        }   
    }
    else
    {
        ERROR_MSG(
            "Water::createManagedObjects: "
            "Could not create/lock vertex buffer\n");
        return false;
    }
    MF_ASSERT( vertexBuffers_[0].getObject() != 0 );

    remappedVerts_.clear();

    return true;
}


void Waters::selectTechnique()
{
    BW_GUARD;

    if(!Waters::s_effect_.exists())
        return;

#if EDITOR_ENABLED
    if (Waters::projectView())
    {
        Waters::s_effect_->hTechnique( "water_proj" );
//      raining_ = false;
    } else
#endif
        //-- ToDo (b_sviglo): reconsider.
// TODO: uncomment+implement these fallbacks
    //if (shaderCap_ > 1)
        Waters::s_effect_->hTechnique( "water_rt" );
    //else if (shaderCap_==1)
    //  Waters::s_effect_->hTechnique("water_SM1" );
    //else
    //  Waters::s_effect_->hTechnique("water_SM0" );
}

/**
 * This method sets all the required render states.
 */
void Water::setupRenderState( float dTime )
{
    BW_GUARD_PROFILER( Water_setupRenderState );

    animTime_ += dTime;

    Moo::rc().effectVisualContext().initConstants();
    Moo::rc().effectVisualContext().updateSharedConstants( Moo::CONSTANTS_PER_VIEW );

    MF_ASSERT( Waters::s_effect_.hasObject() );
    // Set our renderstates and material.
    if (!paramCache_->hasEffect() ||
        Waters::s_effect_->checkEffectRecompiled())
    {
        paramCache_->effect(Waters::s_effect_->pEffect()->pEffect());
    }

#if EDITOR_ENABLED
    if (Waters::projectView())
        raining_ = false;
#endif

    Waters::instance().selectTechnique();
    
    paramCache_->setMatrix( "world", &Moo::rc().world() );

    static bool firstTime = true;
    if (firstTime)
    {
        MF_WATCH( "Client Settings/Water/texScale", s_fTexScale,
            Watcher::WT_READ_WRITE,
            "test scaling" );
        MF_WATCH( "Client Settings/Water/BankLineInterpolationFactor", s_fBankLineInterpolationFactor,
            Watcher::WT_READ_WRITE,
            "Minimal distance to bankline vertex to count it's weight in bankDir" );
        firstTime=false;
    }
    paramCache_->setFloat("texScale",s_fTexScale);

    paramCache_->setFloat( "maxDepth", state_.depth_ );

    if (waterScene_->eyeUnderWater())
        paramCache_->setFloat( "fadeDepth", (state_.depth_ - 0.001f) );
    else
        paramCache_->setFloat( "fadeDepth", state_.fadeDepth_ );
    paramCache_->setVector( "deepColour", &state_.deepColour_ );

    float w = Moo::rc().screenWidth();
    float h = Moo::rc().screenHeight();
    float pixelX = 1.f / w;
    float pixelY = 1.f / h;
    float offsetX = pixelX*0.25f;
    float offsetY = pixelY*0.25f + 1.f;
    Vector4 screenOffset( offsetX, offsetY, offsetY, offsetX );
    paramCache_->setVector( "screenOffset", &screenOffset );

    if(Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
    {
        paramCache_->setFloat( "foamIntersectionFactor", state_.foamIntersection_ );
        paramCache_->setFloat( "foamMultiplier", state_.foamMultiplier_ );  
        paramCache_->setFloat( "foamTiling", state_.foamTiling_ );
        paramCache_->setFloat( "foamFreq", state_.foamFrequency_ );
        paramCache_->setFloat( "foamWidth", state_.foamWidth_ );
        paramCache_->setFloat( "foamAmplitude", state_.foamAmplitude_ );
        paramCache_->setBool(  "bypassDepth", state_.bypassDepth_ );
        paramCache_->setFloat( "causticsPower", state_.causticsPower );
        paramCache_->setFloat( "waterContrast", state_.refractionPower_ );
        paramCache_->setFloat( "waveHeight", state_.waveHeight );
        paramCache_->setBool(  "useCaustics", Waters::s_highQuality_ ); 

        if (foamTexture_)
            paramCache_->setTexture("foamMap", foamTexture_->pTexture());

        //-- make back-buffer copy.
        copyRT(s_rt.get(), 0);
        paramCache_->setTexture("bbCopy", s_rt->pTexture());
    }

    Vector4 x1(state_.waveScale_.x,0,0,0);
    Vector4 y1(0,state_.waveScale_.x,0,0);
    Vector4 x2(state_.waveScale_.y,0,0,0);
    Vector4 y2(0,state_.waveScale_.y,0,0);

    float texAnim = Waters::instance().time() * state_.windVelocity_;

    x1.w = texAnim*state_.scrollSpeed1_.x;
    y1.w = texAnim*state_.scrollSpeed1_.y;

    x2.w = texAnim*state_.scrollSpeed2_.x;
    y2.w = texAnim*state_.scrollSpeed2_.y;

    //TODO: use the parameter cache system.

    paramCache_->setVector( "bumpTexCoordTransformX", &x1 );
    paramCache_->setVector( "bumpTexCoordTransformY", &y1 );
    paramCache_->setVector( "bumpTexCoordTransformX2", &x2 );
    paramCache_->setVector( "bumpTexCoordTransformY2", &y2 );

    //TODO: branch off and setup the surface for simple env. map. reflection if the quality is set to low...

    Vector4 distortionScale( state_.reflectionScale_, state_.reflectionScale_,
                             state_.refractionScale_, state_.refractionScale_ );
    paramCache_->setVector( "scale", &distortionScale );
    paramCache_->setFloat( "fresnelExp", state_.fresnelExponent_ );
    paramCache_->setFloat( "fresnelConstant", state_.fresnelConstant_ );

    paramCache_->setFloat( "fresnelExp", state_.fresnelExponent_ );
    paramCache_->setFloat( "fresnelConstant", state_.fresnelConstant_ );

    paramCache_->setFloat( "freqX", state_.freqX );
    paramCache_->setFloat( "freqZ", state_.freqZ );

    
    paramCache_->setFloat( "cellSizeHalfY", state_.size_.y * 0.5f );
    paramCache_->setFloat( "cellSizeHalfX", state_.size_.x * 0.5f );
    paramCache_->setFloat( "textureTesselation", state_.textureTessellation_ );

    paramCache_->setVector( "reflectionTint", &state_.reflectionTint_ );
    paramCache_->setVector( "refractionTint", &state_.refractionTint_ );

    paramCache_->setFloat( "sunPower", state_.sunPower_ );  
    paramCache_->setFloat( "sunScale", Waters::instance().insideVolume() ? 0.f : state_.sunScale_ );
    paramCache_->setFloat( "smoothness",
        Math::lerp(Waters::instance().rainAmount(), state_.smoothness_, 1.f) );

    if (waterScene_ && waterScene_->reflectionTexture())
    {
        paramCache_->setTexture("reflectionMap", waterScene_->reflectionTexture()->pTexture() );
    }
    if (screenFadeTexture_)
        paramCache_->setTexture("screenFadeMap", screenFadeTexture_->pTexture());
    if (reflectionCubeMap_ 
        && WaterSceneRenderer::s_simpleScene_ )
        paramCache_->setTexture("reflectionCubeMap", reflectionCubeMap_->pTexture());

    paramCache_->setBool( "enableWaterShadows", state_.useShadows_ );

    //-- texture normal aniomation
    int timeInt = (int)floor(animTime_ * state_.animationSpeed_);
    int animFrameNum1 = timeInt % state_.waveTextureNumber_;
    int animFrameNum2 = (timeInt+1) % state_.waveTextureNumber_;
    if(normalTextures_)
    {
        if(normalTextures_[animFrameNum1].exists())
            paramCache_->setTexture("normalMap1", normalTextures_[animFrameNum1]->pTexture());
        if(normalTextures_[animFrameNum2].exists())
            paramCache_->setTexture("normalMap2", normalTextures_[animFrameNum2]->pTexture());
    }
    paramCache_->setFloat("animationInterpolator", animTime_ * state_.animationSpeed_ - timeInt);

    if (   reflectionCubeMap_ 
        && state_.visibility_ == Water::INSIDE_ONLY 
        && WaterSceneRenderer::s_simpleScene_ )
    {       
        simpleReflection_ = 0.0f; 
        paramCache_->setTexture("reflectionCubeMap", reflectionCubeMap_->pTexture());
    }
#if WATER_REFLECTION_FADE_ENABLED
    else
    {
        Vector4 camPos = Moo::rc().invView().row(3);
        const Vector3& waterPos = this->position();
        float camDist = (Vector3(camPos.x,camPos.y,camPos.z) - waterPos).length();
        float dist = camDist - this->size().length()*0.5f;

        // lerp distance to water between cull fade start and cull distance (fade end) as [0.0-1.0]
        dist = Math::clamp( s_sceneCullDistance_-s_sceneCullFadeDistance_, dist, s_sceneCullDistance_ );
        simpleReflection_ = Math::lerp( dist, s_sceneCullDistance_-s_sceneCullFadeDistance_, 
            s_sceneCullDistance_, 0.0f, 1.0f );
    }
    paramCache_->setFloat( "simpleReflection", simpleReflection_ ); 
#endif // WATER_REFLECTION_FADE_ENABLED

    Moo::rc().setFVF( VERTEX_TYPE::fvf() );
    vertexBuffers_[0]->activate();


}


/**
 * Update the waters visibility flag
 *
 * TODO: base activity on water visibility... (perhaps just restrict adding new movements?)
 */
void Water::updateVisibility()
{   
    BW_GUARD;
    Matrix m (Moo::rc().viewProjection());
    m.preMultiply( transform_ );

    bb_.calculateOutcode( m );

    //Test visibility
    visible_ = (!bb_.combinedOutcode());

    WATER_STAT(s_waterVisCount++);
}


bool Water::canReflect( float* retDist ) const
{
    BW_GUARD;
    if (this->shouldDraw() && 
        this->drawMark() == (Waters::nextDrawMark()) &&
        !(state_.visibility_ == Water::INSIDE_ONLY && WaterSceneRenderer::s_simpleScene_) &&
        Waters::s_drawReflection_)
    {
        Vector4 camPos = Moo::rc().invView().row(3);
        const Vector3& waterPos = this->position();

        float camDist = (Vector3(camPos.x,camPos.y,camPos.z) - waterPos).length();
        float dist = camDist - this->size().length()*0.5f;
        if (retDist)
        {
            *retDist = dist;
        }

        if (dist <= s_sceneCullDistance_)
        {
            return true; 
        }
    }

    return false;
}

/**
 * Update water simulation cells for this water surface.
 * @param dTime delta time.
 */
void Water::updateSimulation( float dTime )
{
    BW_GUARD;
    BW_SCOPED_DOG_WATCHER("Water Simulation");
    BW_SCOPED_RENDER_PERF_MARKER("Water Simulation");

    if (!cells_.size() || !createdCells_ || (nVertices_==0)) return;

    // Update the water at 30 fps.
    lastDTime_ = max( 0.001f, dTime );

    static bool changed = false;
    raining_ = false;

    //TODO: clean up the raining flags...

    if (enableSim_)
    {
        raining_ = (Waters::instance().rainAmount() || SimulationManager::instance().rainActive());
        time_ += dTime * 30.f;
        while (time_ >= 1 )
        {
            time_ -= floorf( time_ );

            renderSimulation(1.f/30.f);
        }
        changed = true;
    }
    else if (changed)
    {
        SimulationManager::instance().resetSimulations();
        changed = false;
    }
}


/**
 * This method draws the water.
 * @param group the group of waters to draw.
 * @param dTime the time in seconds since the water was last drawn.
 */
void Water::draw( Waters & group, float dTime )
{
    BW_GUARD;
    GPU_PROFILER_SCOPE(Water_draw);

    // If effect has not loaded yet, just return
    // Do not remove from the draw list as the effect may still be loading
    if (!Waters::s_effect_.hasObject() || !Waters::s_effect_->pEffect())
    {
        return;
    }

#ifdef EDITOR_ENABLED

    if (Waters::drawReflection())
    {
        reflectionCleared_ = false;
    }
    else
    {
        if (!reflectionCleared_)
        {
            clearRT();
            reflectionCleared_ = true;
        }
    }

#endif // EDITOR_ENABLED

    if (!cells_.size() || !createdCells_ || (nVertices_==0))
        return; 

    static DogWatch waterWatch( "Water" );
    static DogWatch waterDraw( "Draw" );

    waterWatch.start();

#ifdef EDITOR_ENABLED
    DWORD colourise = 0;
    if (drawRed())
    {
        // Set the fog to a constant red colour
        colourise = 0x00AA0000;
    }
#endif //EDITOR_ENABLED
    //{
    //  if (Waters::instance().insideVolume())
    //  {
    //      FogController::instance().setOverride(false);
    //      FogController::instance().updateFogParams();
    //  }
    //}

    // Set up the transforms.
    Moo::rc().push();
    Moo::rc().world( transform_ );

    bool inited = init();

    //Test visibility
    if (visible_ && inited)
    {
        waterDraw.start();

        setupRenderState(dTime);
        uint32 currentVBuffer = 0;
        Moo::rc().pushRenderState( D3DRS_FILLMODE );
        Moo::rc().setRenderState( D3DRS_FILLMODE,
            group.drawWireframe_ ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

        if (!paramCache_->hasEffect())
            paramCache_->effect(Waters::s_effect_->pEffect()->pEffect());

        bool bound=false;
        for (uint i=0; i<cells_.size(); i++)
        {
#ifdef DEBUG_WATER          
            if (s_debugCell_>=0 && (i != s_debugCell_ && i != s_debugCell2_))
                continue;
#endif
            //TODO: re-evaluate this culling....improve..
            //      should really be combining multiple cells into a single draw call..
            //TODO: better culling under terrain (linked to the vertex mesh generation)
#ifdef EDITOR_ENABLED
            if (s_cullCells_ && !Waters::projectView())
#else
            if (s_cullCells_)
#endif
            {
                if ( cells_[i].cellDistance() > s_cullDistance_*Moo::rc().camera().farPlane())
                    continue;
            }
            if (currentVBuffer != cells_[i].bufferIndex())
            {
                currentVBuffer = cells_[i].bufferIndex();
                vertexBuffers_[ currentVBuffer ]->activate();
            }

            if( Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING )
            {
                bool usingSim = cells_[i].simulationActive();
                if (raining_)
                {
                    if (!bound) //first time
                    {
                        paramCache_->setBool("useSimulation", true);
                        paramCache_->setFloat("simulationTiling", 10.f);
                    }
                    if (usingSim)
                    {
                        // Mix in with the regular sim.
                        paramCache_->setBool("combineSimulation", true);
                        paramCache_->setTexture("simulationMap", cells_[i].simTexture()->pTexture());                   
                        paramCache_->setTexture("rainSimulationMap", SimulationManager::instance().rainTexture()->pTexture());
                    }
                    else
                    {
                        usingSim=true;
                        paramCache_->setBool("combineSimulation", false);
                        paramCache_->setTexture("simulationMap", SimulationManager::instance().rainTexture()->pTexture());
                        paramCache_->setTexture("rainSimulationMap", SimulationManager::instance().rainTexture()->pTexture());
                    }
                }
                else if (usingSim)
                {
                    if (!bound) //first time
                    {
                        paramCache_->setBool("combineSimulation", false);
                        paramCache_->setFloat("simulationTiling", 1.f);
                    }
                    paramCache_->setBool("useSimulation", true);
                    paramCache_->setTexture("simulationMap", cells_[i].simTexture()->pTexture());
                }
                else
                    paramCache_->setBool("useSimulation", false);

                if (usingSim)
                {
                    //TODO: clean this up..... move things into the cell class..
                    float miny = cells_[i].min().y;
                    float maxy = cells_[i].max().y;
                    float minx = cells_[i].min().x;
                    float maxx = cells_[i].max().x;

                    float simGridSize =   ceilf( state_.simCellSize_ / state_.tessellation_ ) * state_.tessellation_;

                    float s = (state_.textureTessellation_) / (simGridSize);

                    float cx = -(minx + (simGridSize)*0.5f);
                    float cy = -(miny + (simGridSize)*0.5f);

                    Vector4 x1( s, 0.f, 0.f, cx / state_.textureTessellation_ );
                    Vector4 y1( 0.f, s, 0.f, cy / state_.textureTessellation_ );

                    //x1.set( s, 0.f, 0.f, cx / state_.textureTessellation_ );
                    //y1.set( 0.f, s, 0.f, cy / state_.textureTessellation_ );
                    paramCache_->setVector( "simulationTransformX", &x1 );
                    paramCache_->setVector( "simulationTransformY", &y1 );
                }
            }



            if (cells_[i].bind())
            {
                if (bound)
                    Waters::s_effect_->commitChanges();
                else
                {               
                    bound=true;
                    Waters::s_effect_->begin();
                }

                for (uint32 pass=0; pass<Waters::s_effect_->numPasses();pass++)
                {
                    Waters::s_effect_->beginPass(pass);
                    if ( s_enableDrawPrim )
                        cells_[i].draw( vertexBuffers_[ currentVBuffer ]->size() );
                    Waters::s_effect_->endPass();
                }
            }
        }

        if (bound)
            Waters::s_effect_->end();

        // Reset some states to their defaults.
        Moo::rc().popRenderState();
        Moo::rc().setTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );

        vertexBuffers_[ 0 ]->deactivate();

        waterDraw.stop();

#ifdef EDITOR_ENABLED
        // draw some lines for better selection feedback in the tool
        if (highlight())
        {
            float xoff = state_.size_.x * 0.5f;
            float yoff = state_.size_.y * 0.5f;

            for (uint i = 0; i < cells_.size(); i++)
            {
                Vector3 cX0Y0(
                   cells_[i].min().x - xoff, -0.5f, cells_[i].min().y - yoff );
                Vector3 cX1Y1(
                   cells_[i].max().x - xoff,  0.5f, cells_[i].max().y - yoff );

                BoundingBox bb( cX0Y0, cX1Y1 );

                Geometrics::wireBox( bb, 0x00C0FFC0 );
            }
        }
#endif // EDITOR_ENABLED

    }

#ifdef DEBUG_WATER
    //draw debug lines for cell connections
    if (s_debugSim_)
    {
        for (uint i=0; i<cells_.size(); i++)
        {
            if (!cells_[i].isActive())
                continue;

            float xoff = state_.size_.x*0.5f;
            float yoff = state_.size_.y*0.5f;

            Vector3 cX0Y0(cells_[i].min().x - xoff, 1.f, cells_[i].min().y - yoff);
            Vector3 cX0Y1(cells_[i].min().x - xoff, 1.f, cells_[i].max().y - yoff);
            Vector3 cX1Y0(cells_[i].max().x - xoff, 1.f, cells_[i].min().y - yoff);
            Vector3 cX1Y1(cells_[i].max().x - xoff, 1.f, cells_[i].max().y - yoff);

            cX0Y0 = transform_.applyPoint(cX0Y0);
            cX0Y1 = transform_.applyPoint(cX0Y1);
            cX1Y0 = transform_.applyPoint(cX1Y0);
            cX1Y1 = transform_.applyPoint(cX1Y1);

            Moo::PackedColour col = 0xffffffff;

            if (cells_[i].edgeActivation())
                col = 0xff0000ff;


            LineHelper::instance().drawLine( cX0Y0, cX0Y1, col );
            LineHelper::instance().drawLine( cX0Y0, cX1Y0, col );
            LineHelper::instance().drawLine( cX1Y0, cX1Y1, col );
            LineHelper::instance().drawLine( cX0Y1, cX1Y1, col );
        }
    }
#endif //DEBUG_WATER
    waterWatch.stop();
    Moo::rc().pop();

#ifdef EDITOR_ENABLED
    if( colourise != 0 )
    {
        FogController::instance().updateFogParams();
    }
    // Highlighting is done once. The flag will be set for the next frame if
    // it's needed.
    highlight( false );
#endif //EDITOR_ENABLED
    //{
    //  if (Waters::instance().insideVolume())
    //  {
    //      FogController::instance().setOverride(true);
    //      FogController::instance().updateFogParams();
    //  }
    //}
}

bool Water::needSceneUpdate() const 
{ 
    BW_GUARD;
    return needSceneUpdate_ && drawMark_ == Waters::nextDrawMark();
}

void Water::needSceneUpdate( bool value ) 
{ 
    needSceneUpdate_ = value; 
}


/**
 *  Is water loading done in the background?
 */
/*static*/ bool Water::backgroundLoad()
{
    return s_backgroundLoad_;
}

/**
 *  Set whether loading is done in the background.  You should do this at the
 *  start of the app; doing it while the app is running will lead to problems.
 */
/*static*/ void Water::backgroundLoad(bool background)
{
    s_backgroundLoad_ = background;
}


/**
 *  Adds a movement to this water body, if passes through it
 */
bool Water::addMovement( const Vector3& from, const Vector3& to, const float diameter )
{
    BW_GUARD;

    if (!initialised_)
        return false;

    //TODO: wheres the best place to put this movements reset?
    WaterCell::WaterCellVector::iterator cellIt = cells_.begin();
    for (; cellIt != cells_.end(); cellIt++)
    {
        (*cellIt).updateMovements();
    }

    if (!visible())
        return false;

    Vector4 worldCamPos = Moo::rc().invView().row(3);
    Vector3 worldCamPos3(worldCamPos.x, worldCamPos.y, worldCamPos.z);
    float dist = (to - worldCamPos3).length();
    if (dist > Water::s_maxSimDistance_)
        return false;

    // Only add it if it's in our bounding box
    Vector3 cto = invTransform_.applyPoint( to );
    //check distance from water surface.
    if (cto.y > 0.0f || fabs(cto.y) > 10.0f)
        return false;
    Vector3 cfrom = invTransform_.applyPoint( from );

    float displacement = (to - from).length();
    bool stationary = displacement < 0.001f;

    bool inVolume = bbActual_.clip( cfrom, cto );
    if (!stationary && inVolume)    
    {
        //add the movement to the right cell... (using "to" for now)
        Vector3 halfsize( state_.size_.x*0.5f, 0, state_.size_.y*0.5f );
        cto += halfsize;
        cfrom += halfsize;

        float simGridSize = ceilf( state_.simCellSize_ / state_.tessellation_ ) * state_.tessellation_;
        float invSimSize=1.f/simGridSize;

        int xcell = int( cto.x * invSimSize);
        int ycell = int( cto.z * invSimSize);
        uint cell = xcell + int((ceilf(state_.size_.x * invSimSize)))*ycell;
        if (cell < cells_.size())
        {   

//          float invSimSizeX=1.f/cells_[cell].size().x;
//          float invSimSizeY=1.f/cells_[cell].size().y;


            //Calculate the position inside this cell
            cto.x = (cto.x*invSimSize - xcell);
            cto.z = (cto.z*invSimSize - ycell);
            cfrom.x = (cfrom.x*invSimSize - xcell);
            cfrom.z = (cfrom.z*invSimSize - ycell);

            cto.y = displacement;

            cells_[cell].addMovement( cfrom, cto, diameter );
        }
        return true;
    }
    return inVolume;
}


// -----------------------------------------------------------------------------
// Section: Waters
// -----------------------------------------------------------------------------

bool Waters::QualityOption::isSet( EFlag option ) const
{
    return 0 != (m_flagsSet & option);
}

/**
 *  A callback method for the graphics setting quality option
 */
void Waters::setQualityOption(int optionIndex)
{
    BW_GUARD;
    MF_ASSERT(optionIndex >= 0 && optionIndex <= (int)m_qualityOptions.size());

    const QualityOption& qo = m_qualityOptions[optionIndex];

    WaterSceneRenderer::s_textureSize_  = (qo.m_textureSize);
    WaterSceneRenderer::s_drawTrees_    = qo.isSet( QualityOption::DRAW_TREES );
    WaterSceneRenderer::s_drawDynamics_ = qo.isSet( QualityOption::DRAW_DYNAMICS );
    WaterSceneRenderer::s_drawPlayer_   = qo.isSet( QualityOption::DRAW_PLAYER );
    WaterSceneRenderer::s_simpleScene_  = qo.isSet( QualityOption::USE_SIMPLE_SCENE );
    Waters::s_highQuality_              = qo.isSet( QualityOption::USE_HIGH_QUALITY );
    Waters::s_forceUseCubeMap_       	= qo.isSet( QualityOption::USE_CUBE_MAP );
    Waters::s_simulationEnabled_        = qo.isSet( QualityOption::USE_SIMULATION );

    //-- ToDo (b_sviglo): reconsider.
    //-- switch off water simulation in case if it's already enabled.
    if (!Waters::s_simulationEnabled_)
    {
        if(SimulationManager::exists())
            SimulationManager::instance().resetSimulations();
    }

    //-- ToDo (b_sviglo): reconsider.
    for (Waters::iterator it = begin(); it != end(); ++it)
    {
        (*it)->deleteUnmanagedObjects();
        (*it)->createUnmanagedObjects();
    }
}

/**
 *  Retrieves the Waters object instance
 */
inline Waters& Waters::instance()
{
    BW_GUARD;
    REGISTER_SINGLETON( Waters )
    SINGLETON_MANAGER_WRAPPER( Waters )
    static Waters inst;
    return inst;
}


/**
 *  Cleanup
 */
void Waters::fini( )
{
    BW_GUARD;
    s_simulationEffect_ = NULL;
    s_effect_ = NULL;

    listeners_.clear();

    SimulationManager::fini();
#ifdef SPLASH_ENABLED
    splashManager_.fini();
#endif

    Water::s_quadVertexBuffer_ = NULL;
}


/**
 *  Load the required resources
 */
void Waters::loadResources( void * self )
{
    BW_GUARD;

    bool res=false;

    MF_ASSERT( !s_simulationEffect_.hasObject() );
    Moo::EffectMaterialPtr pSimulationEffect = new Moo::EffectMaterial;
    res = pSimulationEffect->initFromEffect( s_simulationEffect );
    if (res)
    {
        s_simulationEffect_ = pSimulationEffect;
    }

    MF_ASSERT( !s_effect_.hasObject() );

    // Keep a local smart pointer to the effect incase fini
    // is called while we're executing (will leak, but 
    // leak already existed, and is an exiting case)
    Moo::EffectMaterialPtr pEffect = new Moo::EffectMaterial;
    res = pEffect->initFromEffect( s_waterEffect );
    
    if (!res)
    {
        CRITICAL_MSG( "Water::loadResources()"
        " couldn't load effect file "
        "%s\n", s_waterEffect.value().c_str() );
    }
    else
    {
        s_effect_ = pEffect;
    }
}


/**
 *  Called upon finishing the resource loading
 */
void Waters::loadResourcesComplete( void * self )
{
}


/**
 *  Initialise the water settings
 */
void Waters::init( )
{
    BW_GUARD;
#if ENABLE_WATCHERS
    isVisible = true;
#endif
    MF_WATCH("Visibility/water", isVisible, Watcher::WT_READ_WRITE, "Is water and water reflections visible." );

    SimulationManager::init();
    SimulationManager::instance().resetSimulations();

    //
    // Register graphics settings
    //      

    bool supported = Moo::rc().vsVersion() >= 0x300 && Moo::rc().psVersion() >= 0x300;

    //--
    qualitySettings_ = Moo::makeCallbackGraphicsSetting(
        "WATER_QUALITY", "Water Quality", *this, &Waters::setQualityOption, -1, false, false);

    qualitySettings_->addOption("MAX",      "Max",      supported, true);
    qualitySettings_->addOption("HIGH",     "High",     supported, true);
    qualitySettings_->addOption("MEDIUM",   "Medium",   supported, true);
    qualitySettings_->addOption("LOW",      "Low",      true);

    //-- ToDo (b_sviglo): reconsider initial state values. Try to avoid this set of static constant
    //--                  and use QualityOption structure instead.
    WaterSceneRenderer::s_textureSize_  = 1.f;
    WaterSceneRenderer::s_drawTrees_    = true;
    WaterSceneRenderer::s_drawDynamics_ = true;
    WaterSceneRenderer::s_drawPlayer_   = true;
    WaterSceneRenderer::s_simpleScene_  = false;
    Waters::s_highQuality_              = true;
    Waters::s_simulationEnabled_        = true;
    Waters::s_forceUseCubeMap_       	= false;

    //--
    m_qualityOptions.push_back(QualityOption(
        1.0f,
        QualityOption::DRAW_TREES |
        QualityOption::DRAW_DYNAMICS |
        QualityOption::DRAW_PLAYER |
        QualityOption::USE_HIGH_QUALITY |
        QualityOption::USE_SIMULATION
        )); //-- MAX

    m_qualityOptions.push_back(QualityOption(
        1.0f,
        QualityOption::DRAW_TREES |
        QualityOption::DRAW_PLAYER |
        QualityOption::USE_HIGH_QUALITY
        )); //-- HIGH

    m_qualityOptions.push_back(QualityOption(
        0.5f,
        QualityOption::DRAW_TREES |
        QualityOption::DRAW_PLAYER
        )); //-- MEDIUM

    m_qualityOptions.push_back(QualityOption(
        0.25f,
        QualityOption::USE_SIMPLE_SCENE |
        QualityOption::USE_CUBE_MAP
        )); //-- LOW

    //--
    Moo::GraphicsSetting::add(qualitySettings_);

    //--
    if (Water::backgroundLoad())
    {
        // do heavy setup stuff in the background
        FileIOTaskManager::instance().addBackgroundTask(
            new CStyleBackgroundTask( "Water Res Loader Task",
                &Waters::loadResources, this,
                &Waters::loadResourcesComplete, this ) );
    }
    else
    {
        loadResources(this);
        loadResourcesComplete(this);
    }
}


/**
 *  Waters constructor
 */
Waters::Waters() :
    drawWireframe_( false ),
    movementImpact_( 20.0f ),
    rainAmount_( 0.f ),
    impactTimer_( 0.f ),
    time_( 0.f )
{
    BW_GUARD;
    static bool firstTime = true;
    if (firstTime)
    {
        MF_WATCH( "Client Settings/Water/draw", s_drawWaters_, 
                    Watcher::WT_READ_WRITE,
                    "Draw water?" );
        MF_WATCH( "Client Settings/Water/wireframe", drawWireframe_,
                    Watcher::WT_READ_WRITE,
                    "Draw water in wire frame mode?" );
        MF_WATCH( "Client Settings/Water/character impact", movementImpact_,
                    Watcher::WT_READ_WRITE,
                    "Character simulation-impact scale" );
        MF_WATCH( "Client Settings/Water/drawShadows", s_enableShadows,
                    Watcher::WT_READ_WRITE, 
                    "Draw shadows?" );

#ifdef DEBUG_WATER
        MF_WATCH( "Client Settings/Water/Stats/water count", s_waterCount,
            Watcher::WT_READ_ONLY, "Water Stats");
        MF_WATCH( "Client Settings/Water/Stats/visible count", s_waterVisCount,
            Watcher::WT_READ_ONLY, "Water Stats");
        MF_WATCH( "Client Settings/Water/Stats/cell count", s_activeCellCount,
            Watcher::WT_READ_ONLY, "Water Stats");
        MF_WATCH( "Client Settings/Water/Stats/edge cell count", s_activeEdgeCellCount,
            Watcher::WT_READ_ONLY, "Water Stats");
        MF_WATCH( "Client Settings/Water/Stats/impact movement count", s_movementCount,
            Watcher::WT_READ_ONLY, "Water Stats");
#endif //DEBUG_WATER

        firstTime = false;
    }
}


/**
 *  Waters destructor
 */
Waters::~Waters()
{
    BW_GUARD;
    s_simulationEffect_ = NULL;
    s_effect_ = NULL;
}


/**
 * Adds a movement to all the waters.
 *
 * @param from  World space position, defining the start of the line moving through the water.
 * @param to    World space position, defining the end of the line moving through the water.
 * @param diameter  The diameter of the object moving through the water.
 */
void Waters::addMovement( const Vector3& from, const Vector3& to, const float diameter )
{
    BW_GUARD;
    Waters::iterator it = this->begin();
    Waters::iterator end = this->end();
    while (it!=end)
    {
        (*it++)->addMovement( from, to, diameter );
    }
}


//ever-increasing id used to identify volume listeners.
uint32 Waters::VolumeListener::s_id = 0;


/**
 * Checks whether the position provided by a matrix provider is inside
 * this water volume or not.
 * @param MatrixProvider, providing the world space position.
 * @return True if the position represented by the matrix provider is inside.
 */
bool Water::isInside( MatrixProviderPtr pMat )
{
    BW_GUARD;
    Matrix m;
    pMat->matrix( m );
    Vector3 v = invTransform_.applyPoint( m.applyToOrigin() );
    return bbActual_.intersects(v);
}


/**
 * Checks whether the position provided by a matrix provider is inside the
 * boundary of the water or not.
 * @param Matrix, providing the world space position.
 * @return True if the position represented by the matrix provider is inside 
 * the boundary.
 */
bool Water::isInsideBoundary( Matrix m )
{
    BW_GUARD;
    Vector3 v = invTransform_.applyPoint( m.applyToOrigin() );

    BoundingBox checkBox( v, v );
    checkBox.addYBounds( FLT_MAX );
    checkBox.addYBounds( -FLT_MAX );

    return bbActual_.intersects( checkBox );
}


/**
 * This method checks all water volume listeners against all current water
 * volumes, and fires off callbacks as necessary.
 */
void Waters::checkAllListeners()
{
    BW_GUARD;
    struct CallbackArguments
    {
    public:
        CallbackArguments( 
            PyObjectPtr pCallback,
            bool entering,
            PyObjectPtr pWater
            ):
            pCallback_( pCallback ),
            entering_( entering ),
            pWater_( pWater )
        {
        }

        PyObjectPtr pCallback_;
        bool entering_;
        PyObjectPtr pWater_;
    };

    BW::vector<CallbackArguments> firedCallbacks;

    VolumeListeners::iterator it = listeners_.begin();
    VolumeListeners::iterator end = listeners_.end();
    while (it != end)
    {
        VolumeListener& l= *it;

        if (!l.inside())
        {
            //If not in any water at all, check all waters.
            for (uint i=0; i < this->size(); i++)
            {
                Water* water = (*this)[i];
                bool inside = water->isInside(l.source());
                if (inside)
                {
                    firedCallbacks.push_back( CallbackArguments(l.callback(),true,water->pPyVolume()) );
                    l.water( water );
                    break;
                }
            }
        }
        else
        {
            //If already in water, only check if you are getting out of that water.
            bool foundWater = false;
            for (uint i=0; i < this->size(); i++)
            {
                Water* water = (*this)[i];
                if (l.water() == water)
                {
                    foundWater = true;
                    bool inside = water->isInside(l.source());
                    if (!inside)
                    {
                        firedCallbacks.push_back( CallbackArguments(l.callback(),false,water->pPyVolume()) );
                        l.water( NULL  );
                    }
                    break;                  
                }
            }

            //If the water is not in our Waters list, then the water has been
            //deleted.  We notify the callback that it has left the water.          
            if (!foundWater)
            {
                firedCallbacks.push_back( CallbackArguments(l.callback(),false,NULL) );
                l.water(NULL);
            }
        }           
        it++;
    }   

    for (size_t i=0; i<firedCallbacks.size(); i++)
    {
        CallbackArguments& cbData = firedCallbacks[i];
        PyObject * callback = cbData.pCallback_.getObject();
        Py_INCREF( callback );
        PyObject * args = PyTuple_New(2);
        PyTuple_SET_ITEM( args, 0, Script::getData(cbData.entering_) );
        PyTuple_SET_ITEM( args, 1, Script::getData(cbData.pWater_) );
        Script::call( callback, args, "Water listener callback: " );    
    }

    firedCallbacks.clear();
}


/*~ function BigWorld addWaterVolumeListener
 *  @components{ client }
 *  This function registers a listener on the water.  A MatrixProvider
 *  provides the source location, and the callback function is called when
 *  the source location enters or leaves a water volume.
 *  The source may only be in one water volume at any one time, if you
 *  overlapping waters, the results will be undefined.
 *  The callback function takes two arguments: a boolean describing whether
 *  the source location has just entered the water or not , and a PyWaterVolume
 *  which is a python wrapper of the water object the source location has just
 *  entered or left from.
 *  @param  dynamicSource   A MatrixProvider providing the listener's location.
 *  @param  callback        A callback fn that takes one boolean argument and one PyWaterVolume argument.
 */
/**
 *  This method adds a water volume listener.
 *  @param  dynamicSource   A MatrixProvider providing the listener's location.
 *  @param  callback        A callback fn that takes one boolean argument and one PyWaterVolume argument.
 *  @return id              ID used to represent and later delete the listener.
 */
uint32 Waters::addWaterVolumeListener( MatrixProviderPtr dynamicSource, PyObjectPtr callback )
{   
    BW_GUARD;
    Waters::instance().listeners_.push_back( VolumeListener(dynamicSource, callback) );
    return Waters::instance().listeners_.back().id();
}

PY_MODULE_STATIC_METHOD( Waters, addWaterVolumeListener, BigWorld )


/*~ function BigWorld delWaterVolumeListener
 *  @components{ client }
 *  This function removes a listener from the water.  The id
 *  received when registering the listener should be passed in.
 *  @param  id              id formerly returned by addWaterVolumeListener 
 */
/**
 *  This method adds a water volume listener.
 *  @param  id              id formerly returned by addWaterVolumeListener 
 */
void Waters::delWaterVolumeListener( uint32 id )
{
    BW_GUARD;
    VolumeListeners::iterator it = Waters::instance().listeners_.begin();
    VolumeListeners::iterator end = Waters::instance().listeners_.end();
    while (it != end)
    {
        VolumeListener& l= *it;
        if (l.id() == id)       
        {
            Waters::instance().listeners_.erase(it);
            return;
        }
        ++it;
    }   
}

PY_MODULE_STATIC_METHOD( Waters, delWaterVolumeListener, BigWorld )


// Static list used to cache up all the water surfaces that need to be drawn
static VectorNoDestructor< Water * > s_waterDrawList;


/**
 * Simulate the water interactions.
 * @param dTime delta time.
 */
void Waters::tick( float dTime )
{
    BW_GUARD;
#if ENABLE_WATCHERS
    if(!isVisible) 
        return;
#endif

    WATER_STAT(s_waterVisCount=0);
    WATER_STAT(s_activeCellCount=0);
    WATER_STAT(s_activeEdgeCellCount=0);
    WATER_STAT(s_movementCount=0);

    //set the current scene as NULL,if there is any water drawn in this tick,
    //then the current scene will be set as that water scene
    WaterSceneRenderer::currentScene( NULL );

    if (s_drawWaters_)
    {
        time_ += dTime;

        for (uint i=0; i < this->size(); i++)
        {
            (*this)[i]->visible(false);
        }
        for (uint i=0; i < s_waterDrawList.size(); i++)
        {
            s_waterDrawList[i]->updateVisibility();
        }
    }

    this->checkAllListeners();
}


/**
 * Simulate the water interactions.
 * @param dTime delta time.
 */
void Waters::updateSimulations( float dTime )
{   
    BW_GUARD;
#if ENABLE_WATCHERS
    if(!isVisible) 
        return;
#endif
    if (s_drawWaters_)
    {
        for( uint32 i = 4; i < Moo::rc().maxSimTextures(); i++ )
        {
            Moo::rc().setTexture( i, NULL );
        }
        if (simulationEnabled())
        {
            for (uint i=0; i < this->size(); i++)
            {
                (*this)[i]->updateSimulation( dTime );
            }
        }
    }
}

uint Waters::drawCount() const
{
    return static_cast<uint>( s_waterDrawList.size() );
}


/**
 *  This static method adds a water object to the list of waters to draw
 */
void Waters::addToDrawList( Water * pWater )
{
    BW_GUARD;
    if ( pWater->drawSelf_ && !(s_nextMark_ == pWater->drawMark()) )
    {
        s_waterDrawList.push_back( pWater );
        pWater->drawMark( s_nextMark_ );
        pWater->clearVisibleChunks();
    }
}


/**
 *  This method draws all the stored water objects under this object's
 *  auspices.
 */
void Waters::drawDrawList( float dTime )
{
    BW_GUARD;
    BW_SCOPED_RENDER_PERF_MARKER("Water")

#if ENABLE_WATCHERS
    if(!isVisible) 
        return;
#endif
    if (s_drawWaters_)
    {
        //-- update per-view and per-screen constants.
        Moo::rc().effectVisualContext().updateSharedConstants(  Moo::CONSTANTS_PER_SCREEN | Moo::CONSTANTS_PER_VIEW );

        for (uint i = 0; i < s_waterDrawList.size(); i++)
        {
            s_waterDrawList[i]->draw( *this, dTime );
        }
    }
    WATER_STAT(s_waterCount=this->size());

    s_lastDrawMark_ = s_nextMark_;
    s_nextMark_++;
    s_waterDrawList.clear();

    impactTimer_ += dTime;
}


/**
 * Draw the refraction mask for this water surface.
 *
 */
void Water::drawMask()
{
    BW_GUARD;
    Moo::rc().push();
    const Vector2 wSize = size();
    const Vector3 wPos = position();
    float wOrient = orientation();
    float xscale = wSize.x * 0.5f, zscale = wSize.y * 0.5f;

    Matrix trans(Matrix::identity);
    trans.setScale(xscale, 1.f, zscale);
    trans.preRotateX( DEG_TO_RAD(90) );
    trans.postRotateY( wOrient );
    trans.postTranslateBy( wPos );
    Moo::rc().world( trans );

    Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
    Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
    Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

    Moo::rc().drawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
    Moo::rc().pop();

}


/**
 * Draw the refraction masking for all the water surfaces.
 *
 */
void Waters::drawMasks()
{
    BW_GUARD;
#if ENABLE_WATCHERS
    if(!isVisible) 
        return;
#endif
    if (s_drawWaters_ && Water::s_quadVertexBuffer_ &&
         s_waterDrawList.size())
    {
        Moo::rc().setFVF(VERTEX_TYPE::fvf());
        // Render the vertex buffer
        Water::s_quadVertexBuffer_->activate();

        Moo::Material::setVertexColour();

        // Mask needs the alpha to be written out.
        Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
        Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

        Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );
        Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
        Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );

        for (uint i = 0; i < s_waterDrawList.size(); i++)
        {
            s_waterDrawList[i]->drawMask( );
        }
        Water::s_quadVertexBuffer_->deactivate();
    }
}


/**
 * Update variables.
 */
void Water::tick()
{
    BW_GUARD;
    // transform the cam position into the waters local space.
    camPos_ = invTransform_.applyPoint( Moo::rc().invView().applyToOrigin() );

    if (initialised_)
    {
        for (uint i=0; i<cells_.size(); i++)
        {
            cells_[i].updateDistance(camPos_);
        }
    }
}


/**
 * Test to see if the camera position is inside this water volume.
 *
 */
bool Water::checkVolume()
{
    BW_GUARD;
    bool insideVol = bbActual_.distance( camPos_ ) == 0.f;
    return insideVol;
}

/**
 * Check the volume of all the water positions.
 */
void Waters::checkVolumes()
{
    BW_GUARD;
#if ENABLE_WATCHERS
    if(!isVisible) 
        return;
#endif

    Vector4 fogColour(1,1,1,1);
    uint i = 0;
    insideVolume(false);
    for (; i < this->size(); i++)
    {
        (*this)[i]->tick();

        if (!Waters::instance().insideVolume())
            Waters::instance().insideVolume( (*this)[i]->checkVolume() );
    }


//TODO: fog stuff
//  if (s_change && insideVolume())
//  {
//      fogColour = (*this)[i-1]->getFogColour();
//
//      //FogController::instance().overrideFog(s_near,s_far, Colour::getUint32(s_col) );
//      FogController::instance().overrideFog(s_near,s_far, 0xffffffff );
//      //FogController::instance().overrideFog(s_near,s_far, Colour::getUint32FromNormalised(fogColour) );
//  }
//  else
//      FogController::instance().setOverride(false);
}

BW_END_NAMESPACE

// water.cpp
