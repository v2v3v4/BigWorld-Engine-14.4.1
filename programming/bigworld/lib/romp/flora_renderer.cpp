#include "pch.hpp"
#include "flora_renderer.hpp"
#include "flora_texture.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/bwresource.hpp"
#include "flora.hpp"
#include "time_of_day.hpp"
#include "enviro_minder.hpp"
#include "texture_feeds.hpp"
#include "math/colour.hpp"
#include "moo/renderer.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 );


BW_BEGIN_NAMESPACE

static AutoConfigString s_floraMFM( "environment/floraMaterial" );
static AutoConfigString s_falloffTexture( "environment/floraFalloff" );

FloraRenderer::FloraRenderer( Flora* flora ) :
    fvf_(),
    capacity_( 0 ),
    pLocked_( NULL ),
    pContainer_( NULL ),
    material_( NULL ),
    shadowMaterial_( NULL ),
    lastBlockID_( -1 ),
    bufferSize_( 0 ),
    lockFlags_( 0 ),
    parameters_( NULL ),
    flora_( flora ),
    pDecl_( NULL ),
    m_localAnimation( 64 ),
    m_globalAnimation( 64 )
{
    BW_GUARD;
    DataSectionPtr pSection = BWResource::openSection(s_floraMFM.value().c_str());

    // Set up the materials
    material_ = new Moo::EffectMaterial;
    if (pSection)
    {
        material_->load(pSection);
        standardParams_.effect(material_->pEffect()->pEffect());
    }

    shadowMaterial_ = new Moo::EffectMaterial;
}


FloraRenderer::~FloraRenderer()
{
    BW_GUARD;
    deleteUnmanagedObjects();
}


void FloraRenderer::deleteUnmanagedObjects( )
{
    BW_GUARD;
    this->delVB();
    deactivate();
}


void FloraRenderer::createUnmanagedObjects( )
{
    BW_GUARD;
    if (bufferSize_ != 0)
        this->createVB( bufferSize_, fvf_, lockFlags_ );
}


/** 
 * helper to create the vb and setting flags for locking it
 * 
 * @param bufferSize size of the vertex buffer
 * @param fvf the flexible vertex format to associate with the buffer
 * @param lockFlags the flags to use when locking the buffer
 * @return true if the successful
 */
bool FloraRenderer::createVB( uint32 bufferSize, DWORD fvf, DWORD lockFlags )
{
    BW_GUARD;
    if (pVB_.valid())
    {
        delVB();
    }

    HRESULT hr = pVB_.create( bufferSize, 
        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, fvf, 
        D3DPOOL_DEFAULT, "vertex buffer/flora" );
    if (FAILED(hr))
    {
        if (hr == D3DERR_OUTOFVIDEOMEMORY)
        {
            ERROR_MSG( "FloraRenderer::createVB: "
                    "unable to create vertex buffer.  Out of Video Memory\n" );
        }
        else
        {
            ERROR_MSG( "FloraRenderer::createVB: "
                    "unable to create vertex buffer.  Error code %lx\n",
                hr );
        }
        pVB_.release();
        return false;
    }

    fvf_ = fvf;
    lockFlags_ = lockFlags;
    bufferSize_ = bufferSize;
    clear();
    return true;
}

/**
 * helper to delete the vb
 */
void FloraRenderer::delVB()
{
    BW_GUARD;
    pVB_.release();
}

/**
 * helper to clear the vb or a section of it
 * @param offset the first byte in the vb to clear
 * @param size the number of bytes to clear
 */
void FloraRenderer::clear( uint32 offset, uint32 size )
{
    BW_GUARD;
    if (pVB_.valid())
    {
        if (size == 0 || (offset + size) > bufferSize_ )
            size = bufferSize_ - offset;
        Moo::SimpleVertexLock vl( pVB_, 0, 0, lockFlags_ );
        if (vl)
        {
            MF_ASSERT(size <= bufferSize_);
            MF_ASSERT(offset <= bufferSize_);
            MF_ASSERT((offset + size) <= bufferSize_);
            ZeroMemory( (char*)(void*)vl + offset, size );
        }
        else
        {
            ERROR_MSG( "FloraRenderer::clear - unable to lock vertex buffer\n" );
        }
    }
    else
    {
        ERROR_MSG( "FloraRenderer::clear - No vertex buffer to clear\n" );
    }
}

FloraVertexContainer* FloraRenderer::lock( uint32 firstVertex, uint32 nVertices )
{
    BW_GUARD;
    if (!pContainer_)
    {
        container_ = FloraVertexContainer();

        void* pData;
        if ( SUCCEEDED(pVB_.lock( sizeof( FloraVertex ) * firstVertex, sizeof( FloraVertex ) * nVertices,
            &pData, lockFlags_ )) )
        {
            FloraVertex* pVerts = (FloraVertex*)pData;

            container_.init( pVerts, nVertices );
            pContainer_ = &container_;

            return pContainer_;
        }
        else
        {
            ERROR_MSG( "FloraVertexContainer::lock - lock failed\n" );
        }
    }
    else
    {
        ERROR_MSG( "FloraVertexContainer::lock - Only allowed to lock once\n" );
    }
    return pContainer_;
}

bool FloraRenderer::unlock( FloraVertexContainer* pContainer )
{
    BW_GUARD;
    if (pContainer == pContainer_)
    {
        pContainer_ = NULL;
        pVB_.unlock();
    }
    return true;
}

uint32 FloraRenderer::capacity()
{
    return capacity_;
}

bool FloraRenderer::preDraw( EnviroMinder& enviro, float tintBegin, float tintEnd)
{
    BW_GUARD;
    //-- write pixel usage type into stencil buffer.
    rp().setupSystemStencil(IRendererPipeline::STENCIL_USAGE_FLORA);

    TimeOfDay* timeOfDay = enviro.timeOfDay();
    DX::Device* pDevice = Moo::rc().device();

    Moo::rc().setVertexDeclaration( pDecl_->declaration() );

    if (material_->checkEffectRecompiled())
    {
        standardParams_.effect( material_->pEffect()->pEffect() );
    }

    if (!material_->begin())
        return false;
    material_->beginPass(0);

    parameters_ = &standardParams_;

    if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_DEFERRED_SHADING)
    {
        IRendererPipeline* rp = Renderer::instance().pipeline();

        //-- set tint parameters.
        parameters_->setFloat  ("g_tintBegin", tintBegin);
        parameters_->setFloat  ("g_tintEnd", tintEnd);
        parameters_->setTexture("g_tintMap", rp->getGBufferTextureCopyFrom(2));
    }
        
    uint32 ambient = Colour::getUint32FromNormalised( timeOfDay->lighting().ambientColour );
    Moo::rc().setRenderState( D3DRS_TEXTUREFACTOR, ambient );   
    parameters_->setVector( "ambient", &timeOfDay->lighting().ambientColour );

    Vector4 vDirection( m_globalAnimation.direction().x, 0, m_globalAnimation.direction().y, 0 );
    parameters_->setVector( "g_windDirection", &vDirection );
    parameters_->setFloatArray( "localAnimations", &m_localAnimation.getGrid()->x, 64 * 2 );
    parameters_->setFloatArray( "globalAnimations", &m_globalAnimation.getGrid()->x, 64 * 2 );

    pVB_.set( 0, 0, sizeof( FloraVertex ) );    

    // Set up fog start and end, so that the vertex shader output will be interpreted
    // correctly.

    if (Moo::rc().mixedVertexProcessing())
            Moo::rc().device()->SetSoftwareVertexProcessing(FALSE);

    lastBlockID_ = -1;

    return true;
}

void FloraRenderer::postDraw()
{
    BW_GUARD;
    material_->endPass();
    material_->end();
    if (Moo::rc().mixedVertexProcessing())
            Moo::rc().device()->SetSoftwareVertexProcessing(TRUE);

    //-- restore stencil state.
    rp().restoreSystemStencil();
}

void FloraRenderer::beginAlphaTestPass( float drawDistance, float fadePercent, uint32 alphaTestRef )
{
    BW_GUARD;
    //5 - visibility constants
    float vizFar = drawDistance;
    float vizNear = vizFar * (fadePercent /100.f);
    float vizRange = vizFar - vizNear;
    Vector4 visibility( 1.f / vizRange, - vizNear / vizRange, 0, 0.f );
    parameters_->setVector( "VISIBILITY", &visibility );        
    parameters_->setFloat( "g_alphaRef", alphaTestRef / 255.0f );

    Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
    Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    Moo::rc().setRenderState( D3DRS_ALPHAREF, alphaTestRef );
    Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
    Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
    Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );       
    lastBlockID_ = -1;
}

void FloraRenderer::drawBlock( uint32 firstVertex, uint32 nVertices, 
    const FloraBlock& block )
{       
    BW_GUARD;

    parameters_->commitChanges();

    Moo::rc().drawPrimitive(D3DPT_TRIANGLELIST, firstVertex, nVertices / 3 );
}

bool FloraRenderer::init( uint32 bufferSize )
{
    BW_GUARD;
    bool result = this->createVB( bufferSize, 0, D3DLOCK_NOOVERWRITE);
    if (result)
    {
        this->clear();
        capacity_ = bufferSize / sizeof( FloraVertex );
    }
    pDecl_ = Moo::VertexDeclaration::get( "flora" );
    return result;
}

void FloraRenderer::fini( )
{
    BW_GUARD;
    this->delVB();
    capacity_ = 0;
}

void FloraRenderer::update( float dTime, EnviroMinder& enviro )
{
    BW_GUARD;
    static bool bFirstTime = true;

    static float s_fLocalPersistence = 0.3f;
    static int s_nLocalOctaves = 6;
    static float s_fLocalScale = 0.7f;
    static float s_fLocalFrequency = 0.4f;

    static float s_fGlobalPersistence = 0.4f;
    static int s_nGlobalOctaves = 6;
    static float s_fGlobalScale = 0.2f;
    static float s_fGlobalFrequency = 0.3f;

    if ( bFirstTime )
    {
        MF_WATCH( "Client Settings/Flora/Local Noise Persistence",
            s_fLocalPersistence,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );
        MF_WATCH( "Client Settings/Flora/Local Noise octaves",
            s_nLocalOctaves,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );
        MF_WATCH( "Client Settings/Flora/Local Noise Scale",
            s_fLocalScale,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );
        MF_WATCH( "Client Settings/Flora/Local Noise Frequency",
            s_fLocalFrequency,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );

        MF_WATCH( "Client Settings/Flora/Global Noise Persistence",
            s_fGlobalPersistence,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );
        MF_WATCH( "Client Settings/Flora/Global Noise octaves",
            s_nGlobalOctaves,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );
        MF_WATCH( "Client Settings/Flora/Global Noise Scale",
            s_fGlobalScale,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );
        MF_WATCH( "Client Settings/Flora/Global Noise Frequency",
            s_fGlobalFrequency,
            Watcher::WT_READ_WRITE,
            "Alpha test reference value for the alpha tested drawing pass." );

        bFirstTime = false;
    }

    MyPerlin &localNoise = m_localAnimation.getNoise();
    localNoise.persistence( s_fLocalPersistence );
    localNoise.octaves( s_nLocalOctaves );
    m_localAnimation.scale( s_fLocalScale );

    MyPerlin &globalNoise = m_globalAnimation.getNoise();
    globalNoise.persistence( s_fGlobalPersistence );
    globalNoise.octaves( s_nGlobalOctaves );
    m_globalAnimation.scale( s_fGlobalScale );

    m_localAnimation.update( dTime * s_fLocalFrequency, enviro );
    m_globalAnimation.update( dTime * s_fGlobalFrequency, enviro );
}

void FloraRenderer::deactivate()
{
    BW_GUARD;
    if(material_.exists())
    {
        material_->pEffect()->pEffect()->SetTexture("blendMap", NULL);
        standardParams_.commitChanges();
    }
    //-- ToDo (b_sviglo): reconsider shadow material in flora. It seems useless.
    if(shadowMaterial_->pEffect())
    {
        shadowMaterial_->pEffect()->pEffect()->SetTexture("blendMap", NULL);
        shadowParams_.commitChanges();
    }
}

void FloraRenderer::activate()
{
}

static bool canRender()
{
    BW_GUARD;
    return Moo::rc().vsVersion() >= 0x100 && 
        Moo::rc().deviceInfo( Moo::rc().deviceIndex() ).caps_.MaxSimultaneousTextures > 1;
}


/**
 *  Constructor
 */
FloraVertexContainer::FloraVertexContainer(
    FloraVertex* pVerts, uint32 nVerts ) :
    pVertsBase_(),
    pVerts_(),
    nVerts_(),
    blockNum_(),
    uOffset_(),
    vOffset_()
#ifdef EDITOR_ENABLED
    , blocks_(),
    pCurBlock_(),
    pCurEcotype_()
#endif

{
    BW_GUARD;
    nVerts_ = 0;
    init( pVerts, nVerts );
}


/**
* This method inits the vertex container
*/
void FloraVertexContainer::init( FloraVertex* pVerts, uint32 nVerts )
{
    BW_GUARD;
    pVertsBase_ = pVerts;
    pVerts_ = pVerts;
    nVerts_ = nVerts;
}


/**
* This method adds a list of vertices to the vertex container
*/
void FloraVertexContainer::addVertices( const FloraVertex * pVertIn,
                                       uint32 count, const Matrix * pTransform )
{
    BW_GUARD;
    MF_ASSERT( uint32(pVerts_ - pVertsBase_) <= (nVerts_ - count) );
    FloraVertex * pVertOut = pVerts_;
    for (const FloraVertex * pVertex = pVertIn, * end = pVertIn + count; 
        pVertex != end; ++pVertex, ++pVertOut)
    {
        const Vector3 tintPos( pVertex->tint_.x, pVertex->tint_.y, pVertex->tint_.z );
        pVertOut->pos_   = pTransform->applyPoint( pVertex->pos_ );
        pVertOut->uv_[0] = pVertex->uv_[0] + uOffset_;
        pVertOut->uv_[1] = pVertex->uv_[1] + vOffset_;
        pVertOut->tint_  = Vector4( pTransform->applyPoint( tintPos ),
            pVertex->tint_.w );
        pVertOut->animation_[0] = pVertex->animation_[0];
        const float fRand = 1.4142136f * pVertOut->pos_.x + 1.7320508f * pVertOut->pos_.z;
        const int iRand = int(64 * (fRand - floorf(fRand)));
        pVertOut->animation_[1] = (ANIM_TYPE)( blockNum_ + iRand );
    }
    pVerts_ = pVertOut;
}


void FloraVertexContainer::clear( uint32 nVertices )
{
    BW_GUARD;
    MF_ASSERT( nVertices <= (nVerts_ - (pVerts_ - pVertsBase_)) );
    ZeroMemory( pVerts_, nVertices * sizeof( FloraVertex ) );
    pVerts_ += nVertices;
}



#ifdef EDITOR_ENABLED
void FloraVertexContainer::offsetUV( FloraVertex*& pVerts )
{
    BW_GUARD;
    pVerts->uv_[0] += uOffset_;
    pVerts->uv_[1] += vOffset_;
}
#endif

BW_END_NAMESPACE

// flora_renderer.cpp
