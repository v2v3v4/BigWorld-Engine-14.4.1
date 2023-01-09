#include "pch.hpp"
#include "sprite_particle_renderer.hpp"
#include "particle/particle_serialisation.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "math/planeeq.hpp"
#include "moo/fog_helper.hpp" 
#include "moo/renderer.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "sprite_particle_renderer.ipp"
#endif



const BW::string SpriteParticleRenderer::nameID_ = "SpriteParticleRenderer";

namespace SpriteParticleRendererDefaults
{
    const AutoConfigString s_defaultEffectFileName(
        "system/spriteParticles/default" );

    // Defaults 
    const int DEFAULT_FRAME_COUNT = 0;
    const float DEFAULT_FRAME_RATE = 0.0f;
    const bool DEFAULT_ROTATED = false;

    // Default particle distance limits
    const uint16 MIN_DISTANCE = 0;
    const uint16 MAX_DISTANCE = 65535;

    // Soft particle defaults
    const float DEFAULT_SOFT_DEPTH_RANGE = 0.0f;
    const float DEFAULT_SOFT_FALLOFF_POWER = 1.0f;
    const float DEFAULT_SOFT_DEPTH_OFFSET = 0.0f;

    // Near fade defaults
    const float DEFAULT_NEAR_CUTOFF = 1.0f;
    const float DEFAULT_NEAR_FADE_START = 1.0f;
    const float DEFAULT_NEAR_FADE_FALLOFF_POWER = 1.0f;

}   // namespace SpriteParticleRendererDefaults

// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for SpriteParticleRenderer.
// -----------------------------------------------------------------------------

using namespace SpriteParticleRendererDefaults;

/**
 *  This is the constructor for SpriteParticleRenderer.
 *
 *  @param newTextureName   String containing the file name of the sprite.
 */
SpriteParticleRenderer::SpriteParticleRenderer(
        const BW::StringRef & newTextureName ) :
    materialFX_( FX_BLENDED ),
    textureName_( newTextureName.data(), newTextureName.size() ),
    materialSettingsChanged_( false ),
    frameCount_( DEFAULT_FRAME_COUNT ),
    frameRate_( DEFAULT_FRAME_RATE ),
    rotated_( DEFAULT_ROTATED ),
    explicitOrientation_(0,0,0),
    softDepthRange_( DEFAULT_SOFT_DEPTH_RANGE ),
    softFalloffPower_( DEFAULT_SOFT_FALLOFF_POWER ),
    softDepthOffset_( DEFAULT_SOFT_DEPTH_OFFSET ),
    nearFadeCutoff_( DEFAULT_NEAR_CUTOFF ),
    nearFadeStart_( DEFAULT_NEAR_FADE_START ),
    nearFadeFalloffPower_( DEFAULT_NEAR_FADE_FALLOFF_POWER )
{
    BW_GUARD;
    // Set up the texture coordinate in UV space for the sprite, horizontal
    // blur, and vertical blur meshes.
    sprite_[0].uv_.set( 0.0f, 1.0f );
    sprite_[1].uv_.set( 0.0f, 0.0f );
    sprite_[2].uv_.set( 1.0f, 0.0f );
    sprite_[3].uv_.set( 1.0f, 1.0f );

    // Set specular (actually fog values) for the vertices.
    sprite_[0].specular_ = 0xffffffff;
    sprite_[1].specular_ = 0xffffffff;
    sprite_[2].specular_ = 0xffffffff;
    sprite_[3].specular_ = 0xffffffff;

    effectName_ = s_defaultEffectFileName.value().c_str();

    // Check that reading AutoConfigString was successful
    MF_ASSERT( effectName_.length() > 0 );

    // Initialise the material for the sprite.
    if (textureName_.size() > 0)
    {
        updateMaterial();
    }
}

/**
 *  This is the destructor for SpriteParticleRenderer.
 */
SpriteParticleRenderer::~SpriteParticleRenderer()
{
}

/**
 * This method loads the texture using the stored texture name if no 
 * texture is currently loaded.
 * @param force     If force is true, always load the texture.
 */
void SpriteParticleRenderer::updateTexture( bool force )
{
    if (!this->textureName().empty() && 
        ( !diffuseMap_.get() || force ))
    {
        diffuseMap_ = Moo::TextureManager::instance()->get(
            this->textureName(), true, true, true, "texture/particle");
    }
}


// -----------------------------------------------------------------------------
// Section: Renderer Overrides for the SpriteParticleRenderer.
// -----------------------------------------------------------------------------

/**
 *  This method is called whenever a material property for the sprite has
 *  been changed. It prepares any changes for drawing.
 */
void SpriteParticleRenderer::updateMaterial()
{
    BW_GUARD;

    if (effectMaterial_)
    {
        effectMaterial_ = NULL;
    }

    Moo::EffectMaterialPtr material;

    // Load material
    material = new Moo::EffectMaterial();
    material->initFromEffect( this->effectName() );

    // Load texture
    this->updateTexture();

    this->setMaterial( material );

    materialSettingsChanged_ = false;
}

/**
 *  Set the current material and texture.
 *  Effect properties from the material are overridden by properties read
 *  from the particle's file, which were loaded in serialiseInternal().
 *  @param material new material pointer to use.
 *  @param texture new texture pointer to use.
 */
void SpriteParticleRenderer::setMaterial(
    Moo::EffectMaterialPtr& material )
{
    BW_GUARD;

    // - Take pointers
    effectMaterial_ = material;

    // - Set effect properties in material
    if ( effectMaterial_ && effectMaterial_->pEffect() )
    {
        effectMaterial_->properties().clear();
        this->setMaterialProperties( true );
    }
}

/**
 *  Set effect properties in material.
 *  @param force force all properties to be set, even if the material settings
 *  have not changed.
 */
void SpriteParticleRenderer::setMaterialProperties( bool force )
{
    BW_GUARD;

    if (effectMaterial_ == NULL)
    {
        return;
    }

    // Material has changed - reset props
    if ( materialSettingsChanged_ || force )
    {
        // setup alpha blending state
        setMaterialFX();

        // set texture
        if ( diffuseMap_ != NULL )
        {
            effectMaterial_->setProperty( "diffuseMap", diffuseMap_, true );
        }

        effectMaterial_->setProperty( "softDepthRange", softDepthRange_, true );
        effectMaterial_->setProperty( "softFalloffPower", softFalloffPower_, true );
        effectMaterial_->setProperty( "softDepthOffset", softDepthOffset_, true );
        effectMaterial_->setProperty( "nearFadeCutoff", nearFadeCutoff_, true );
        effectMaterial_->setProperty( "nearFadeStart", nearFadeStart_, true );
        effectMaterial_->setProperty( "nearFadeFalloffPower", nearFadeFalloffPower_, true );
    }
}

/**
 *  materialFX sets the srcBlend and destBlend for the material.
 *  eg. solid, alpha, additive etc.
 */
void SpriteParticleRenderer::setMaterialFX()
{
    BW_GUARD;

    if (effectMaterial_ == NULL)
    {
        return;
    }

    if (!materialSettingsChanged_)
    {
        return;
    }

    effectMaterial_->setProperty( "fogToFogColour", false, true );

    if ( materialFX_ == FX_ADDITIVE )
    {
        effectMaterial_->setProperty(
            "srcBlend", Moo::Material::SRC_ALPHA, true );
        effectMaterial_->setProperty(
            "destBlend", Moo::Material::ONE, true );
        effectMaterial_->setProperty( "alphaTestEnable", false, true );
    }
    else if ( materialFX_ == FX_ADDITIVE_ALPHA )
    {
        effectMaterial_->setProperty(
            "srcBlend", Moo::Material::ONE, true );
        effectMaterial_->setProperty(
            "destBlend", Moo::Material::INV_SRC_ALPHA, true );
        effectMaterial_->setProperty( "alphaTestEnable", false, true );
    }
    else if ( materialFX_ == FX_BLENDED )
    {
        effectMaterial_->setProperty(
            "srcBlend", Moo::Material::SRC_ALPHA, true );
        effectMaterial_->setProperty(
            "destBlend", Moo::Material::INV_SRC_ALPHA, true );
        effectMaterial_->setProperty( "alphaTestEnable", false, true );
    }
    else if ( materialFX_ == FX_BLENDED_COLOUR )
    {
        effectMaterial_->setProperty(
            "srcBlend", Moo::Material::SRC_COLOUR, true );
        effectMaterial_->setProperty(
            "destBlend", Moo::Material::INV_SRC_COLOUR, true );
        effectMaterial_->setProperty( "alphaTestEnable", false, true );
    }
    else if ( materialFX_ == FX_BLENDED_INVERSE_COLOUR )
    {
        effectMaterial_->setProperty(
            "srcBlend", Moo::Material::SRC_COLOUR, true );
        effectMaterial_->setProperty(
            "destBlend", Moo::Material::INV_SRC_COLOUR, true );
        effectMaterial_->setProperty( "alphaTestEnable", false, true );
    }
    else if ( materialFX_ == FX_SOLID )
    {
        effectMaterial_->setProperty(
            "srcBlend", Moo::Material::ONE, true );
        effectMaterial_->setProperty(
            "destBlend", Moo::Material::ZERO, true );
        effectMaterial_->setProperty( "alphaTestEnable", false, true );

        effectMaterial_->setProperty( "fogToFogColour", true, true );
    }
    else if ( materialFX_ == FX_SHIMMER )
    {
        effectMaterial_->setProperty( "alphaTestEnable", false, true );
    }
    else if ( materialFX_ == FX_SOURCE_ALPHA )
    {
        effectMaterial_->setProperty(
            "srcBlend", Moo::Material::ONE, true );
        effectMaterial_->setProperty(
            "destBlend", Moo::Material::ZERO, true );
        effectMaterial_->setProperty( "alphaTestEnable", true, true );
        effectMaterial_->setProperty( "alphaReference", 0x80, true );

        effectMaterial_->setProperty( "fogToFogColour", true, true );
    }
    else
    {
        ERROR_MSG( "Unknown materialFX \"%d\"\n", materialFX_ );
    }

}

/**
 *  This is the draw method that does the drawing for the frame. In theory,
 *  a sprite particle renderer could draw multiple particle systems provided
 *  they all use the same texture and effects.
 */
void SpriteParticleRenderer::draw( Moo::DrawContext& drawContext,
                                  const Matrix& worldTransform,
                                  Particles::iterator beg,
                                  Particles::iterator end,
                                  const BoundingBox &bb )
{
    BW_GUARD;

    // Make sure the texture is ready.
    if (materialSettingsChanged_)
    {
        setMaterialProperties();
    }

    if ( beg != end )
    {
        Matrix view = Moo::rc().view();
        float distance = 0.f;
        if ( local() )
        {
            view.preMultiply( worldTransform );
        }

        // destBlend() == Moo::Material::ONE
        if ( materialFX_ == FX_ADDITIVE )
        {
            BoundingBox bounds = bb;
            bounds.transformBy( view );
            distance = ( bounds.maxBounds().z + bounds.minBounds().z ) * 0.5f;
        }
        else if ( !viewDependent() )
        {
            Particles::iterator it = beg;
            const Vector3& pos = it->position();
            float maxDist = pos.x * view[0][2] +
                pos.y * view[1][2] +
                pos.z * view[2][2] +
                view[3][2];

            float minDist = maxDist;
            ++it;

            const float particleDistConvert =
                float( MAX_DISTANCE ) / Moo::rc().camera().farPlane();

            while ( it != end )
            {
                const Vector3& p = it->position();
                float dist = p.x * view[0][2] +
                    p.y * view[1][2] +
                    p.z * view[2][2] +
                    view[3][2];

                if ( dist <= 0 )
                {
                    it->distance( 0 );
                }
                else if ( dist >= Moo::rc().camera().farPlane() )
                {
                    it->distance( MAX_DISTANCE );
                }
                else
                {
                    it->distance( uint16( particleDistConvert * dist ) );
                }


                maxDist = max( maxDist, dist );
                minDist = min( minDist, dist );

                ++it;
            }
            distance = ( maxDist + minDist ) * 0.5f;
        }

        sortedDrawItem_.set( this, worldTransform, beg, end );

        if ( FX_SHIMMER != materialFX_ )
        {
            drawContext.drawUserItem( &sortedDrawItem_, Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );
        }
        else
        {
            drawContext.drawUserItem( &sortedDrawItem_, Moo::DrawContext::SHIMMER_CHANNEL_MASK, distance );
        }

    }
}


/**
 *  Check if the particle system is valid and can be drawn.
 *  @return true if drawable.
 */
bool SpriteParticleRenderer::isValid() const
{
    BW_GUARD;

    // Not visible - return
    // No texture
    // No effect
    // Has shimmer, but not shimmering allowed
    if ( ( diffuseMap_ == NULL ) ||
         ( effectMaterial_ == NULL ) ||
         ( effectMaterial_->pEffect() == NULL ) ||
         ( ( materialFX_ == FX_SHIMMER ) && !Moo::Material::shimmerMaterials ) )
    {
        return false;
    }

    return true;
}

/**
 *  This method sets the diffuse texture name that this renderer should use.
 *
 *  @param newString    The new file name as a string.
 */
void SpriteParticleRenderer::textureName( const BW::string& value )
{
    BW_GUARD;
    if (value != textureName_)
    {
        textureName_ = value;
        materialSettingsChanged_ = true;
        this->updateTexture( true );
    }
}

/**
 *  Returns true if Soft Particles are supported by the renderer pipeline
 */
bool SpriteParticleRenderer::isSoftSupported()
{
    return Renderer::instance().pipelineType() == 
        IRendererPipeline::TYPE_DEFERRED_SHADING;
}

/**
 *  Reset Soft Particles params to values that essentially disable soft particle effect
 *
 *  @param value    The new value for the enableSoft property.
 */
void SpriteParticleRenderer::resetSoft()
{
    softDepthRange( DEFAULT_SOFT_DEPTH_RANGE );
    softDepthOffset( DEFAULT_SOFT_DEPTH_OFFSET );
    softFalloffPower( DEFAULT_SOFT_FALLOFF_POWER );
}

/**
 *  Reset Near fade params to values that essentially disable near fading
 *
 *  @param value    The new value for the enableSoft property.
 */
void SpriteParticleRenderer::resetNearFade()
{
    nearFadeCutoff( DEFAULT_NEAR_CUTOFF );
    nearFadeStart( DEFAULT_NEAR_FADE_START );
    nearFadeFalloffPower( DEFAULT_NEAR_FADE_FALLOFF_POWER );
}

/**
 *  Sets when the particles have completely faded out and are invisible.
 *  nearFadeStart() cannot be below this value, so it will be adjusted.
 *  Near cutoff cannot be below 0 and will be clamped.
 *  @param value new cutoff value.
 */
void SpriteParticleRenderer::nearFadeCutoff( float value )
{
    BW_GUARD;

    // Cannot be below 0.0f
    nearFadeCutoff_ = std::max( 0.0f, value );
    if (nearFadeCutoff_ > nearFadeStart_)
    {
        nearFadeStart(nearFadeCutoff_);
    }

    materialSettingsChanged_ = true;
}

/**
 *  Sets when the particles start to fade out.
 *  Cannot be below 0 or below nearFadeCutoff().
 *  @param value new near fade start.
 */
void SpriteParticleRenderer::nearFadeStart( float value )
{
    BW_GUARD;

    // Clamp: Cannot be below 0 or below nearFadeCutoff
    nearFadeStart_ = std::max( 0.0f, value );
    if (nearFadeCutoff_ > nearFadeStart_)
    {
        nearFadeCutoff(nearFadeStart_);
    }

    materialSettingsChanged_ = true;
}


class SpriteParticleJob
{
    friend class SpriteParticleRenderer;

public:
    virtual void execute();

private:
    const Particle * pParticles_;
    uint numParticles_;
    const Matrix * view_;
    const Matrix * worldTransform_;
    float frameRate_;
    int frameCount_;
    Vector3 explicitOrientation_;
    SpriteParticleRenderer::MaterialFX materialFX_;
    float fogNear_;
    float fogRange_;
    bool local_;
    bool rotated_;
    Moo::VertexXYZDUV * pVerticesOut_;
    uint numTrisOut_;
};

void SpriteParticleJob::execute()
{
    Moo::VertexXYZDUV quad[4];

    Moo::VertexXYZDUV * pVertex = pVerticesOut_;
    const Particle * particles = pParticles_;
    const uint numParticles = numParticles_;

    const int frameCount = frameCount_;
    const float frameRate = frameRate_;
    const float fogNear = fogNear_;
    const float fogRange = fogRange_;
    SpriteParticleRenderer::MaterialFX materialFX = materialFX_;

    float u = 0.f;
    const float ux = (frameCount > 0) ? 1.f / frameCount : 1.f;

    Matrix transform( *view_ );
    if (local_)
    {
        transform.preMultiply( *worldTransform_ );
    }

    // set the minsize to be a quarter of a pixel in screenspace
    const float minSize = 0.25f / Moo::rc().halfScreenWidth();

    const bool reOrient = (explicitOrientation_ != Vector3(0,0,0));

    uint numTris = 0;
    Vector3 position;
    for (uint i = 0; i < numParticles; i++)
    {
        const Particle & p = particles[i];

        if (!p.isAlive())
        {
            continue;
        }

        if (frameCount > 0)
        {
            u = floorf( p.age() * frameRate ) * ux;
        }

        transform.applyPoint( position, p.position() );
        if (position.z < 0)
        {
            continue;
        }

        const float size = p.size();

        // skip this particle if it's too small.
        if (size < minSize * position.z)
        {
            continue;
        }

        // apply the rotation
        const float initialRotation = p.yaw();
        float rotationSpeed = p.pitch();
        if (rotationSpeed > MATH_PI)
        {
            rotationSpeed -= MATH_2PI;
        }
        rotationSpeed *= MATH_2PI;// convert from rev/s to angular speed

        DWORD colour;
        if (materialFX == SpriteParticleRenderer::FX_ADDITIVE_ALPHA)
        {
            const float dist = 1.f - Math::clamp( 0.f,
                ((position.z - fogNear) / fogRange), 1.f );
            colour = Colour::getUint32( Colour::getVector4( p.colour() ) * dist);
        }
        else
        {
            colour = p.colour();
        }

        const float actualRotation = initialRotation + rotationSpeed * p.age();
        const float sinx = rotated_ ? sinf( actualRotation ) : 0.f;
        const float cosx = rotated_ ? cosf( actualRotation ) : 1.f;

        if (reOrient)
        {       
            PlaneEq plane( explicitOrientation_, 0.f );
            Vector3 xdir;
            Vector3 ydir;
            plane.basis( xdir, ydir );
            xdir = view_->applyVector( xdir * size );
            ydir = view_->applyVector( ydir * size );

            quad[0].pos_ = position - ydir * cosx + xdir * sinx;
            quad[0].colour_ = colour;
            quad[0].uv_.set( u, 0.0f );
            quad[1].pos_ = position + xdir * cosx + ydir * sinx;
            quad[1].colour_ = colour;
            quad[1].uv_.set( u + ux, 0.0f );
            quad[2].pos_ = position + ydir * cosx - xdir * sinx;
            quad[2].colour_ = colour;
            quad[2].uv_.set( u + ux, 1.0f );
            quad[3].pos_ = position - xdir * cosx - ydir * sinx;
            quad[3].colour_ = colour;
            quad[3].uv_.set( u, 1.0f );
        }
        else
        {
            // set up the 4 vertices for the sprite
            quad[0].pos_.x = position.x - size * cosx - size * sinx;        
            quad[0].pos_.y = position.y - size * sinx + size * cosx;
            quad[0].pos_.z = position.z;
            quad[0].colour_ = colour;
            quad[0].uv_.set( u, 0.0f );
            quad[1].pos_.x = position.x + size * cosx - size * sinx;
            quad[1].pos_.y = position.y + size * sinx + size * cosx;
            quad[1].pos_.z = position.z;
            quad[1].colour_ = colour;
            quad[1].uv_.set( u + ux, 0.0f );
            quad[2].pos_.x = position.x + size * cosx + size * sinx;
            quad[2].pos_.y = position.y + size * sinx - size * cosx;
            quad[2].pos_.z = position.z;
            quad[2].colour_ = colour;
            quad[2].uv_.set( u + ux, 1.0f );
            quad[3].pos_.x = position.x - size * cosx + size * sinx;
            quad[3].pos_.y = position.y - size * sinx - size * cosx;
            quad[3].pos_.z = position.z;
            quad[3].colour_ = colour;
            quad[3].uv_.set( u, 1.0f );
        }

        // add the 2 triangles to the triangle-list
        *(pVertex++) = quad[0];
        *(pVertex++) = quad[1];
        *(pVertex++) = quad[2];

        *(pVertex++) = quad[0];
        *(pVertex++) = quad[2];
        *(pVertex++) = quad[3];

        numTris += 2;
    }

    numTrisOut_ = numTris;
}





void SpriteParticleRenderer::realDraw( const Matrix& worldTransform, Particles::iterator beg, Particles::iterator end )
{
    BW_GUARD_PROFILER( SpriteParticleRenderer_realDraw );   

    // Not visible - return
    if ( ( beg == end ) || ( !isValid() ) )
    {
        return;
    }

    // The particles may be either in view space or world space.
    const Matrix view = viewDependent() ? Matrix::identity : Moo::rc().view();


    typedef Moo::VertexXYZDUV VertexType;
    int vertexSize = sizeof(VertexType);

    Moo::DynamicVertexBufferBase2& vb = 
        Moo::DynamicVertexBufferBase2::instance( vertexSize );
    const uint numParticles = static_cast< uint >( std::distance( beg, end ) );
    const uint nLockElements = numParticles * 6;

    MF_ASSERT( nLockElements <= UINT_MAX );
    VertexType * pVertex = vb.lock2<VertexType>( 
        static_cast<uint32>(nLockElements) );

    if (!pVertex)
    {
        return;
    }

    // store colorwriteenable in case we are shimmering
    Moo::rc().pushRenderState( D3DRS_COLORWRITEENABLE );

    const Moo::FogParams & params = Moo::FogHelper::pInstance()->fogParams();
    float fogNear = params.m_start;
    float fogRange = (params.m_end - params.m_start);

    if ( materialFX_ != FX_ADDITIVE )
    {
        std::sort( beg, end, Particle::sortParticlesReverse );
    }

    // set up job
    SpriteParticleJob immediateJob;
    SpriteParticleJob * job = &immediateJob;

    job->pParticles_ =  &(*beg);
    job->numParticles_ = numParticles;
    job->pVerticesOut_ = pVertex;
    job->frameCount_ = frameCount_;
    job->view_ = &view;
    job->worldTransform_ = &worldTransform;
    job->fogNear_ = fogNear;
    job->fogRange_ = fogRange;
    job->materialFX_ = materialFX();
    job->local_ = local();
    job->explicitOrientation_ = explicitOrientation_;
    job->rotated_ = rotated_;
    job->frameRate_ = frameRate_;
    job->numTrisOut_ = 0;

    job->execute();

    // Render
    vb.unlock();    
    uint32 lockIndex = vb.lockIndex();

    if (job->numTrisOut_ != 0)
    {
        vb.set( 0 );

        // Set vertex declaration
        Moo::VertexDeclaration* pDecl =
            Moo::VertexDeclaration::get( "xyzduv" );
        MF_ASSERT( pDecl );

        Moo::rc().setVertexDeclaration( pDecl->declaration() );

        // Set effect variables

        // Render using effect
        if ( effectMaterial_->begin() )
        {
            for ( uint32 i = 0; i < effectMaterial_->numPasses(); ++i )
            {
                effectMaterial_->beginPass(i);
                Moo::rc().drawPrimitive(
                    D3DPT_TRIANGLELIST, lockIndex, job->numTrisOut_ );
                effectMaterial_->endPass();
            }

            effectMaterial_->end();
        }

    }

    // set color write enable in case we are shimmering
    Moo::rc().popRenderState();
}

// -----------------------------------------------------------------------------
// Section: Auxiliary Methods for SpriteParticleRenderer.
// -----------------------------------------------------------------------------


template < typename Serialiser >
void SpriteParticleRenderer::serialise( const Serialiser & s ) const
{
    BW_PARITCLE_SERIALISE_ENUM( s, SpriteParticleRenderer, materialFX );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, frameCount );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, frameRate );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, explicitOrientation );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, textureName );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, softDepthRange );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, softFalloffPower );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, softDepthOffset );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, nearFadeCutoff );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, nearFadeStart );
    BW_PARITCLE_SERIALISE( s, SpriteParticleRenderer, nearFadeFalloffPower );
}


void SpriteParticleRenderer::loadInternal( DataSectionPtr pSect )
{
    BW_GUARD;
    this->serialise( BW::LoadFromDataSection< SpriteParticleRenderer >( pSect,
        this ) );
    this->updateMaterial();
}


void SpriteParticleRenderer::saveInternal( DataSectionPtr pSect ) const
{
    BW_GUARD;
    this->serialise( BW::SaveToDataSection< SpriteParticleRenderer >( pSect,
        this ) );
}


SpriteParticleRenderer * SpriteParticleRenderer::clone() const
{
    BW_GUARD;
    SpriteParticleRenderer * psr = new SpriteParticleRenderer(
        BW::StringRef() );
    ParticleSystemRenderer::clone( psr );
    this->serialise( BW::CloneObject< SpriteParticleRenderer >( this, psr ) );
    psr->updateMaterial();
    // rotation is based on the the SourcePSA having non zero values rotation
    // values. Assume the whole particle system is being cloned so we should
    // copy this value even though it's not serialised.
    psr->rotated_ = rotated_;
    return psr;
}


/*static*/ void SpriteParticleRenderer::prerequisites(
        DataSectionPtr pSection,
        BW::set<BW::string>& output )
{
    const BW::string& textureName = pSection->readString( "textureName_" );
    if (textureName.length())
        output.insert(textureName);
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to PySpriteParticleRenderer.
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PySpriteParticleRenderer::
PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( MaterialFX, materialFX,
    materialFX )

PY_TYPEOBJECT( PySpriteParticleRenderer )

/*~ function Pixie.SpriteRenderer
 *  Factory function to create and return a new PySpriteParticleRenderer object. A
 *  SpriteParticleRenderer is a ParticleSystemRenderer which renders each
 *  particle as a facing polygon.
 *  @param spritename Name of sprite to use for particles.
 *  @return A new PySpriteParticleRenderer object.
 */
PY_FACTORY_NAMED( PySpriteParticleRenderer, "SpriteRenderer", Pixie )

PY_BEGIN_METHODS( PySpriteParticleRenderer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PySpriteParticleRenderer )
    /*~ attribute PySpriteParticleRenderer.textureName
     *  Texture file name of the sprite.
     *  @type String.
     */
    PY_ATTRIBUTE( textureName )
    /*~ attribute PySpriteParticleRenderer.materialFX
     *  Specifies the MaterialFX to use to draw the sprites into the scene.
     *  Each value specifies either a special effect, or a source and destination
     *  alpha-blend setting for the material. If no value is specified, materialFX
     *  defaults to 'BLENDED'. The possible values are listed below:
     *
     *  ADDITIVE: alpha blended. source = SRC_ALPHA, destination = ONE
     *
     *  ADDITIVE_ALPHA: alpha blended. source = ONE, destination = INV_SRC_ALPHA
     *
     *  BLENDED: alpha blended. source = SRC_ALPHA, destination = INV_SRC_ALPHA
     *
     *  BLENDED_COLOUR: alpha blended. source = SRC_COLOUR, destination = INV_SRC_COLOUR
     *
     *  BLENDED_INVERSE_COLOUR: alpha blended. source = INV_SRC_COLOUR, destination = SRC_COLOUR
     *
     *  SOLID: alpha blended. source = ONE, destination = ZERO
     *
     *  SHIMMER: special effect not implemented on PC
     *
     *  Refer to the DirectX documentation for a description of how material
     *  settings affect alpha blending.
     *
     *  @type String.
     */
    PY_ATTRIBUTE( materialFX )
    /*~ attribute PySpriteParticleRenderer.frameCount
     *  Specifies the number of frames of animation in the texture. Default is 0.
     *  @type Integer.
     */
    PY_ATTRIBUTE( frameCount )
    /*~ attribute PySpriteParticleRenderer.frameRate
     *  Specifies the number of frames of animation per second of particle age.
     *  Default is 0.
     *  @type Float.
     */
    PY_ATTRIBUTE( frameRate )

    /*~ attribute PySpriteParticleRenderer.softDepthRange
     *  Modifies range of depth difference to which softness is applied..
     *  [0.0,n] default=0.0
     *  @type float.
     */
    PY_ATTRIBUTE( softDepthRange )

    /*~ attribute PySpriteParticleRenderer.softFalloffPower
     *  Falloff curve power. 1 is linear. Higher values increase fade rate..
     *  [1.0,10.0] default=1
     *  @type float.
     */
    PY_ATTRIBUTE( softFalloffPower )

    /*~ attribute PySpriteParticleRenderer.softDepthOffset
     *  Shifts the softening by relative depth difference, relative to extent..
     *  [-1.0,1.0] default=0.0
     *  @type float.
     */
    PY_ATTRIBUTE( softDepthOffset )

    /*~ attribute PySpriteParticleRenderer.nearFadeCutoff
     *  Distance at which particles become completely invisible..
     *  [0.0,nearFadeStart] default=1.0
     *  @type float.
     */
    PY_ATTRIBUTE( nearFadeCutoff )

    /*~ attribute PySpriteParticleRenderer.nearFadeStart
     *  Distance at which fading is full opacity and starts fading towards near cutoff.
     *  [nearFadeCutoff,n] default=1.5
     *  @type float.
     */
    PY_ATTRIBUTE( nearFadeStart )

    /*~ attribute PySpriteParticleRenderer.nearFadeFalloffPower
     *  Falloff curve power, 1 is linear. Higher values increase fade rate..
     *  [1.0,10.0] default=1.0
     *  @type float.
     */
    PY_ATTRIBUTE( nearFadeFalloffPower )


PY_END_ATTRIBUTES()

PY_ENUM_MAP( PySpriteParticleRenderer::MaterialFX )
PY_ENUM_CONVERTERS_CONTIGUOUS( PySpriteParticleRenderer::MaterialFX )


/**
 *  Constructor.
 */
PySpriteParticleRenderer::PySpriteParticleRenderer( SpriteParticleRendererPtr pR, PyTypeObject *pType ):
    PyParticleSystemRenderer( pR, pType ),
    pR_( pR )
{
    BW_GUARD;
    IF_NOT_MF_ASSERT_DEV( pR_.hasObject() )
    {
        MF_EXIT( "NULL renderer" );
    }
}


/**
 *  Static Python factory method. This is declared through the factory
 *  declaration in the class definition.
 *
 *  @param  args    The list of parameters passed from Python. This should
 *                  just be a string (textureName.)
 */

PyObject *PySpriteParticleRenderer::pyNew( PyObject *args )
{
    BW_GUARD;
    char *nameFromArgs = "None";
    if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
    {
        PyErr_SetString( PyExc_TypeError, "SpriteRenderer() expects "
            "a optional texture name string" );
        return NULL;
    }

    SpriteParticleRenderer* spr = new SpriteParticleRenderer( nameFromArgs );
    return new PySpriteParticleRenderer( spr );
}

BW_END_NAMESPACE

// sprite_particle_renderer.cpp
