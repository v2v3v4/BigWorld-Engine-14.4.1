
#include "pch.hpp"

#include "chunk_light.hpp"
#include "geometry_mapping.hpp"

#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/animation_manager.hpp"

#include "chunk.hpp"
#include "moo/renderer.hpp"
#include "moo/deferred_lights_manager.hpp"

#ifdef EDITOR_ENABLED
#include "appmgr/options.hpp"
#endif

DECLARE_DEBUG_COMPONENT2( "Chunk", 1 );


BW_BEGIN_NAMESPACE

int ChunkLight_token;


// ----------------------------------------------------------------------------
// Section: ChunkLight
// ----------------------------------------------------------------------------

/**
 *  Add ourselves to or remove ourselves from the given chunk
 */
void ChunkLight::toss( Chunk * pChunk )
{
    BW_GUARD;
    if (pChunk_ != NULL)
    {
        ChunkLightCache & clc = ChunkLightCache::instance( *pChunk_ );
        this->delFromContainer( clc.pOwnLights() );
        clc.dirtySeep();
    }

    this->ChunkItem::toss( pChunk );

    if (pChunk_ != NULL)
    {
        ChunkLightCache & clc = ChunkLightCache::instance( *pChunk_ );

        updateLight( pChunk_->transform() );

        addToCache( clc );

        clc.dirtySeep();
    }
}

// ----------------------------------------------------------------------------
// Section: ChunkMooLight
// ----------------------------------------------------------------------------

ChunkMooLight::ChunkMooLight( WantFlags wantFlags ) :
    ChunkLight( wantFlags )
{
}

void ChunkMooLight::addToCache( ChunkLightCache& cache ) const
{
    BW_GUARD;
    addToContainer( cache.pOwnLights() );
}

// ----------------------------------------------------------------------------
// Section: ChunkDirectionalLight
// ----------------------------------------------------------------------------

IMPLEMENT_CHUNK_ITEM( ChunkDirectionalLight, directionalLight, 0 )

/**
 *  Constructor
 */
ChunkDirectionalLight::ChunkDirectionalLight() :
    pLight_( new Moo::DirectionalLight(
        Moo::Colour( 0, 0, 0, 1 ), Vector3( 0, -1, 0 ) ) )
{
}


/**
 *  Destructor
 */
ChunkDirectionalLight::~ChunkDirectionalLight()
{
    // needn't clear pointer 'coz when it goes out of scope its ref
    // count will decrease for us (disposing us if necessary)
}


void ChunkDirectionalLight::updateLight( const Matrix& world ) const
{
    pLight_->worldTransform( world );
}


/**
 *  This method adds this light to the input container
 */
void ChunkDirectionalLight::addToContainer( Moo::LightContainerPtr pLC ) const
{
    pLC->addDirectional( pLight_ );
}

/**
 *  This method removes this light from the input container
 */
void ChunkDirectionalLight::delFromContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    DirectionalLightVector & lv = pLC->directionals();
    for (uint i = 0; i < lv.size(); i++)
    {
        if (lv[i] == pLight_)
        {
            lv.erase( lv.begin() + i );
            break;
        }
    }
}


/**
 *  This method loads the light from the section.
 */
bool ChunkDirectionalLight::load( DataSectionPtr pSection )
{
    BW_GUARD;   
#ifdef EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    pLight_->colour( Moo::Colour( colour[0], colour[1], colour[2], 1 ) );
#else//EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    float multiplier = pSection->readFloat( "multiplier" );
    pLight_->colour( Moo::Colour( colour[0], colour[1], colour[2], 1 ) * multiplier );
#endif//EDITOR_ENABLED


    Vector3 direction( pSection->readVector3( "direction" ) );
    pLight_->direction( direction );

    return true;
}


// ----------------------------------------------------------------------------
// Section: ChunkOmniLight
// ----------------------------------------------------------------------------

IMPLEMENT_CHUNK_ITEM( ChunkOmniLight, omniLight, 0 )

/**
 *  Constructor
 */
ChunkOmniLight::ChunkOmniLight( WantFlags wantFlags ) :
    ChunkMooLight( wantFlags ),
    pLight_( new Moo::OmniLight(
        Moo::Colour( 0, 0, 0, 1 ), Vector3( 0, 0, 0 ), 0, 0 ) )
{
}


/**
 *  Destructor
 */
ChunkOmniLight::~ChunkOmniLight()
{
}


void ChunkOmniLight::updateLight( const Matrix& world ) const
{
    BW_GUARD;
    pLight_->worldTransform( world );
}

/**
 *  This method adds this light to the input container
 */
void ChunkOmniLight::addToContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    pLC->addOmni( pLight_ );
}

/**
 *  This method removes this light from the input container
 */
void ChunkOmniLight::delFromContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    OmniLightVector & lv = pLC->omnis();
    for (uint i = 0; i < lv.size(); i++)
    {
        if (lv[i] == pLight_)
        {
            lv.erase( lv.begin() + i );
            break;
        }
    }
}

//static
void ChunkOmniLight::load( Moo::OmniLight & light,
                          const DataSectionPtr & pSection )
{
#ifdef EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    light.colour( Moo::Colour( colour[0], colour[1], colour[2], 1 ) );
#else//EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    float multiplier = pSection->readFloat( "multiplier" );
    light.colour( Moo::Colour( colour[0], colour[1], colour[2], 1 ) * multiplier );
#endif//EDITOR_ENABLED
    light.position( Vector3( pSection->readVector3( "position" ) ) );
    light.innerRadius( pSection->readFloat( "innerRadius" ) );
    light.outerRadius( pSection->readFloat( "outerRadius" ) );
    light.priority( pSection->readInt( "priority" ) );
}

/**
 *  This method loads the light from the section
 */
bool ChunkOmniLight::load( DataSectionPtr pSection )
{
    BW_GUARD;   
    load( *pLight_, pSection );

    return true;
}


// ----------------------------------------------------------------------------
// Section: ChunkSpotLight
// ----------------------------------------------------------------------------

IMPLEMENT_CHUNK_ITEM( ChunkSpotLight, spotLight, 0 )

/**
 *  Constructor
 */
ChunkSpotLight::ChunkSpotLight(WantFlags wantFlags) :
    ChunkMooLight( wantFlags ),
    pLight_( new Moo::SpotLight(
        Moo::Colour( 0, 0, 0, 1 ),
        Vector3( 0, 0, 0 ),
        Vector3( 0, -1, 0 ),
        0, 0, 0 ) )
{
}


/**
 *  Destructor
 */
ChunkSpotLight::~ChunkSpotLight()
{
}


void ChunkSpotLight::updateLight( const Matrix& world ) const
{
    BW_GUARD;
    pLight_->worldTransform( world );
}

/**
 *  This method adds this light to the input container
 */
void ChunkSpotLight::addToContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    pLC->addSpot( pLight_ );
}

/**
 *  This method removes this light from the input container
 */
void ChunkSpotLight::delFromContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    SpotLightVector & lv = pLC->spots();
    for (uint i = 0; i < lv.size(); i++)
    {
        if (lv[i] == pLight_)
        {
            lv.erase( lv.begin() + i );
            break;
        }
    }
}

//static
void ChunkSpotLight::load( Moo::SpotLight & light,
                          const DataSectionPtr & pSection )
{
#ifdef EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    light.colour( Moo::Colour( colour[0], colour[1], colour[2], 1 ) );
#else//EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    float multiplier = pSection->readFloat( "multiplier" );
    light.colour( Moo::Colour( colour[0], colour[1], colour[2], 1 ) * multiplier );
#endif//EDITOR_ENABLED

    light.position( Vector3( pSection->readVector3( "position" ) ) );
    light.direction( Vector3( pSection->readVector3( "direction" ) ) );
    light.innerRadius( pSection->readFloat( "innerRadius" ) );
    light.outerRadius( pSection->readFloat( "outerRadius" ) );
    light.cosConeAngle( pSection->readFloat( "cosConeAngle" ) );
    light.priority( pSection->readInt( "priority" ) );
}


/**
 *  This method loads the light from the section
 */
bool ChunkSpotLight::load( DataSectionPtr pSection )
{
    BW_GUARD;   
    load( *pLight_, pSection );

    return true;
}


// ----------------------------------------------------------------------------
// Section: ChunkPulseLight
// ----------------------------------------------------------------------------

IMPLEMENT_CHUNK_ITEM( ChunkPulseLight, pulseLight, 0 )

/**
 *  Constructor
 */
ChunkPulseLight::ChunkPulseLight( WantFlags wantFlags ) :
    ChunkMooLight( wantFlags ),
    pLight_( new Moo::OmniLight(
        Moo::Colour( 0, 0, 0, 1 ), Vector3( 0, 0, 0 ), 0, 0 ) ),
    positionAnimFrame_( 0 ),
    colourAnimFrame_( 0 ),
    position_(0,0,0),
    animPosition_(0,0,0),
    colour_( 0, 0, 0, 1 )
{
    BW_GUARD;
}


/**
 *  Destructor
 */
ChunkPulseLight::~ChunkPulseLight()
{
}


void ChunkPulseLight::updateLight( const Matrix& world ) const
{
    BW_GUARD;
    pLight_->worldTransform( world );
}


void ChunkPulseLight::tick( float dTime )
{
    BW_GUARD_PROFILER( ChunkPulseLight_tick );

    if (pAnimation_)
    {
        positionAnimFrame_ += dTime * 30.f;
        if (positionAnimFrame_ >= pAnimation_->totalTime())
        {
            positionAnimFrame_ = fmodf(positionAnimFrame_, pAnimation_->totalTime());
        }
        Matrix m;
        Matrix res = Matrix::identity;

        for (uint32 i = 0; i < pAnimation_->nChannelBinders(); i++)
        {
            Moo::AnimationChannelPtr pChannel = pAnimation_->channelBinder(i).channel();
            if (pChannel)
            {
                pChannel->result( positionAnimFrame_, m );
                res.preMultiply( m );
            }
        }
        animPosition_ = res.applyToOrigin();
    }

    colourAnimFrame_ += dTime;
    colourAnimFrame_ = fmodf(colourAnimFrame_, colourAnimation_.getTotalTime());
    float colourMod = colourAnimation_.animate( colourAnimFrame_ );

    pLight_->colour( Moo::Colour( colour_.r * colourMod, colour_.g * colourMod, 
        colour_.b * colourMod, 1.f ) );
    pLight_->position( position_ + animPosition_ );
    pLight_->worldTransform( (pChunk_ != NULL) ?
        pChunk_->transform() : Matrix::identity );
}

/**
 *  This method adds this light to the input container
 */
void ChunkPulseLight::addToContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    pLC->addOmni( pLight_ );
}

/**
 *  This method removes this light from the input container
 */
void ChunkPulseLight::delFromContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    OmniLightVector & lv = pLC->omnis();
    for (uint i = 0; i < lv.size(); i++)
    {
        if (lv[i] == pLight_)
        {
            lv.erase( lv.begin() + i );
            break;
        }
    }
}

const StringRef ChunkPulseLight::defaultAnimation("system/animation/lightnoise.animation");
const StringRef ChunkPulseLight::defaultFrameSection("system/data/pulse_light.xml");

/**
 *  This method loads the light from the section
 */
bool ChunkPulseLight::load( DataSectionPtr pSection )
{
    BW_GUARD;   
    ChunkOmniLight::load( *pLight_, pSection );

    position_ = pLight_->position();
    colour_ = pLight_->colour();

    BW::string animation = pSection->readString( "animation",
        defaultAnimation );
    Moo::NodePtr pNode = new Moo::Node;

    pAnimation_ = Moo::AnimationManager::instance().get( animation, pNode );

    if (!pSection->openSection( "timeScale"))
    {// load the legacy animations
        pSection = BWResource::openSection( defaultFrameSection );
    }

    colourAnimation_.reset();

    if (pSection)
    {
        float timeScale = pSection->readFloat( "timeScale", 1.f );
        float duration = pSection->readFloat( "duration", 0 );
        BW::vector< Vector2 > animFrames;
        pSection->readVector2s( "frame", animFrames );
        for (uint32 i = 0; i <animFrames.size(); i ++)
        {
            colourAnimation_.addKey( animFrames[i].x * timeScale, animFrames[i].y );
        }
        if (colourAnimation_.getTotalTime() != 0.f)
        {
            colourAnimation_.loop( true, duration > 0.f ? duration * timeScale : colourAnimation_.getTotalTime() );
        }
    }
    if (colourAnimation_.getTotalTime() == 0.f)
    {
        colourAnimation_.addKey( 0.f, 1.f );
        colourAnimation_.addKey( 1.f, 1.f );
    }

    return true;
}

// ----------------------------------------------------------------------------
// Section: ChunkAmbientLight
// ----------------------------------------------------------------------------

IMPLEMENT_CHUNK_ITEM( ChunkAmbientLight, ambientLight, 0 )

/**
 *  Constructor
 */
ChunkAmbientLight::ChunkAmbientLight() :
    colour_( 0, 0, 0, 1 )
#ifdef EDITOR_ENABLED
    , multiplier_()
#endif
{
}


/**
 *  Destructor
 */
ChunkAmbientLight::~ChunkAmbientLight()
{
}

void ChunkAmbientLight::addToCache( ChunkLightCache& cache ) const
{
    BW_GUARD;
    addToContainer( cache.pOwnLights() );
}

/**
 *  This method adds this light to the input container
 */
void ChunkAmbientLight::addToContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    pLC->ambientColour( colour_ * multiplier() );
}

/**
 *  This method removes this light from the input container
 */
void ChunkAmbientLight::delFromContainer( Moo::LightContainerPtr pLC ) const
{
    BW_GUARD;
    pLC->ambientColour( Moo::Colour( 0.f, 0.f, 0.f, 1.f ) );
}


/**
 *  This method loads the light from the section
 */
bool ChunkAmbientLight::load( DataSectionPtr pSection )
{
    BW_GUARD;   
#ifdef EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    colour_ = Moo::Colour( colour[0], colour[1], colour[2], 1 );
#else//EDITOR_ENABLED
    Vector3 colour = pSection->readVector3( "colour" ) / 255.f;
    float multiplier = pSection->readFloat( "multiplier", 1.f );
    colour = colour * multiplier;
    colour_ = Moo::Colour( colour[0], colour[1], colour[2], 1 );
#endif//EDITOR_ENABLED

    return true;
}


// ----------------------------------------------------------------------------
// Section: ChunkLightCache
// ----------------------------------------------------------------------------

// TODO: Make it so chunks know about their own space...

BW_END_NAMESPACE

#include "chunk_manager.hpp"

BW_BEGIN_NAMESPACE

namespace {

struct PortalTraverseInfo
{
    PortalTraverseInfo( Chunk& chunk, ChunkBoundary::Portal& portal )
        :   chunk_(&chunk),
            portal_(&portal)
    {
        MF_ASSERT( portal.hasChunk() );
        MF_ASSERT( portal.pChunk->isBound() );

        chunkToChunk_.multiply( portal.pChunk->transform(), chunk.transformInverse() );
    }

    Chunk* chunk_; // the chunk we traversed from
    ChunkBoundary::Portal* portal_; // the portal we traversed through
    Matrix chunkToChunk_; // transform to move a point from target chunk back to this chunk
};

typedef BW::vector<PortalTraverseInfo> PTIVector;

/**
 *  Helper function for checking a container of lights against 
 *  a stack set of portals.
 */
void pullIntersectingLightsLC( PTIVector& tstack, 
                     const Moo::LightContainer& srcLights, 
                     Moo::LightContainer& outLights )
{
    // Omni lights.
    const OmniLightVector& omnis = srcLights.omnis();
    for( size_t i = 0; i < omnis.size(); i++ )
    {
        const Moo::OmniLightPtr & omni = omnis[i];

        // If we already have this light, skip.
        if ( std::find( outLights.omnis().begin(), 
                        outLights.omnis().end(), omni ) != outLights.omnis().end() )
        {
            continue;
        }

        Vector3 localLightPos = omni->position();
        float   lightRadius   = omni->outerRadius();

        bool seepLight = true;
        for( PTIVector::reverse_iterator it = tstack.rbegin();
            it != tstack.rend(); it++ )
        {
            // Move the light position across to this chunk.
            localLightPos = it->chunkToChunk_.applyPoint( localLightPos );

            // Test it against this portal
            if ( !it->portal_->intersectSphere( localLightPos, lightRadius ) )
            {
                seepLight = false;
                break;
            }
        }

        if (seepLight)
        {
            outLights.addOmni( omni );
        }
    }

    // Spot lights
    const SpotLightVector& spots = srcLights.spots();
    for( size_t i = 0; i < spots.size(); i++ )
    {
        const Moo::SpotLightPtr & spot = spots[i];

        // If we already have this light, skip.
        if ( std::find( outLights.spots().begin(), 
                        outLights.spots().end(), spot ) != outLights.spots().end() )
        {
            continue;
        }

        Vector3 localLightPos = spot->position();
        float   lightRadius   = spot->outerRadius();

        bool seepLight = true;
        for( PTIVector::reverse_iterator it = tstack.rbegin();
            it != tstack.rend(); it++ )
        {
            // Move the light position across to this chunk.
            localLightPos = it->chunkToChunk_.applyPoint( localLightPos );

            // Test it against this portal
            if ( !it->portal_->intersectSphere( localLightPos, lightRadius ) )
            {
                seepLight = false;
                break;
            }
        }

        if (seepLight)
        {
            outLights.addSpot( spot );
        }
    }

    // Directionals
    for ( uint32 i = 0; i < srcLights.nDirectionals(); i++ )
    {
        Moo::DirectionalLightPtr pDir = srcLights.directional( i );
        if ( std::find( outLights.directionals().begin(), 
                        outLights.directionals().end(), pDir ) == outLights.directionals().end() )
        {
            outLights.addDirectional( pDir );
        }
    }
}

/**
 *  Helper function to recursively check lights from connected chunks to see if they
 *  intersect with portals back to the original chunk (inserting into the given 
 *  diffuse and specular light containers).
 */
void pullIntersectingLights( PTIVector& tstack, 
                       Moo::LightContainer& outLightsDiffuse )
{
    MF_ASSERT( !tstack.empty() );

    Chunk& lastChunk = *(tstack.back().chunk_);
    Chunk& topChunk  = *(tstack.back().portal_->pChunk);

    // See if the lights in this chunk intersect with each portal in order, right back up to the root.
    Moo::LightContainerPtr diffLC = ChunkLightCache::instance(topChunk).pOwnLights();

    pullIntersectingLightsLC( tstack, *diffLC, outLightsDiffuse );

    // Go through other connected portals to discover more lights.
    if (tstack.size() <= ChunkLightCache::MAX_LIGHT_SEEP_DEPTH)
    {
        for ( Chunk::piterator pit = topChunk.pbegin(); pit != topChunk.pend(); pit++ )
        {
            if ( !pit->hasChunk() || !pit->pChunk->isBound() || pit->pChunk == &lastChunk )
                continue;

            tstack.push_back( PortalTraverseInfo(topChunk, *pit) );
            pullIntersectingLights( tstack, outLightsDiffuse );
            tstack.pop_back();
        }
    }
}

/**
 *  Helper class to track an integer counter with a Chunk pointer.
 */
class ChunkCounter
{
public:
    typedef std::pair<Chunk*, int> ChunkIntPair;

public:
    ChunkCounter() : size_(0)
    {}

    size_t size() const { return size_; }
    void clear() { size_ = 0; }

    void add( Chunk* chunk, int diff )
    {
        for (size_t i = 0; i < size_; i++)
        {
            if (counts_[i].first == chunk)
            {
                counts_[i].second += diff;
                return;
            }
        }

        if ( size_ == MAX_DEPTH)
        {
            return;
        }

        counts_[size_].first = chunk;
        counts_[size_].second = diff;
        ++size_;
    }

    ChunkIntPair& operator[]( size_t i )
    {
        MF_ASSERT( i < size_ );
        return counts_[i];
    }

private:
    static const int MAX_DEPTH = 6;
    ChunkIntPair    counts_[ MAX_DEPTH ];
    size_t          size_;
};

/**
 *  Internal helper function for intersectLight.
 */
void intersectLightInternal( const Vector3& oposl, const Vector3& nposl,
                         float oradius, float nradius,
                         Chunk& chunk, ChunkBoundary::Portal& portal,
                         bool transient, ChunkCounter& counter, 
                         int depth )
{
    BW_GUARD;
    bool oposIntersects = portal.intersectSphere( oposl, oradius );
    bool nposIntersects = portal.intersectSphere( nposl, nradius );

    if ( !nposIntersects && !oposIntersects )
    {
        // Nothing to do...
        return;
    }

    if ( nposIntersects )
    {
        counter.add( portal.pChunk, 1 );
    }
    else if ( oposIntersects && !nposIntersects && !transient )
    {
        counter.add( portal.pChunk, -1 );
    }

    // Push through portals
    if (depth < ChunkLightCache::MAX_LIGHT_SEEP_DEPTH)
    {
        for ( Chunk::piterator pit = portal.pChunk->pbegin(); pit != portal.pChunk->pend(); pit++ )
        {
            if (!pit->hasChunk() || !pit->pChunk->isBound() || pit->pChunk == &chunk)
            {
                continue;
            }

            Matrix chunkToChunk;
            chunkToChunk.multiply( chunk.transform(), portal.pChunk->transformInverse() );

            intersectLightInternal(
                    chunkToChunk.applyPoint( oposl ),
                    chunkToChunk.applyPoint( nposl ),
                    oradius, nradius,
                    *portal.pChunk, *pit,
                    transient, counter,
                    depth+1 );
        }
    }
}

/**
 *  Given two spheres (an old position and a new position), this will 
 *  intersect and traverse portals to discover which chunks were intersecting
 *  and chunks which are now intersecting. Uses ref-counts to allow for
 *  chunks to accessable via multiple different portal paths.
 */
void intersectLight( const Vector3& opos, const Vector3& npos, 
                 float orad, float nrad,
                 Chunk& chunk, bool transient,
                 ChunkCounter& counter )
{
    BW_GUARD;
    for ( Chunk::piterator pit = chunk.pbegin(); 
        pit != chunk.pend(); pit++ )
    {
        ChunkBoundary::Portal& portal = (*pit);

        if (!portal.hasChunk() || !portal.pChunk->isBound())
        {
            continue;
        }

        intersectLightInternal( opos, npos, orad, nrad,
                                chunk, portal, transient,
                                counter, 1 );
    }
}

/**
 *  Templated helper function to push a moving light around in the
 *  ChunkLightCache containers.
 */
template<typename T>
void pushLight( const T& pLight,
               const Vector3& opos, float oradius,
               Chunk& rootChunk, bool transient )
{
    BW_GUARD;

    ChunkCounter counts;

    intersectLight( opos, pLight->position(),
                    oradius, pLight->outerRadius(),
                    rootChunk, transient, counts );

    for( size_t i = 0; i < counts.size(); i++ )
    {
        Chunk& chunk    = *counts[i].first;
        int count       =  counts[i].second;

        const Moo::LightContainerPtr & chunkAllLights = 
            ChunkLightCache::instance( chunk ).pAllLights();

        if (!chunkAllLights)
        {
            continue;
        }

        if ( count >= 0 )
        {
            if (chunkAllLights->addLight( pLight, true ))
            {
                //DEBUG_MSG( "Added light 0x%p to chunk %s (incount=%d)!\n",
                //      pLight.get(), chunk.identifier().c_str(), count );
            }
        }
        else
        {
            if (chunkAllLights->delLight( pLight ))
            {
                //DEBUG_MSG( "Removed light 0x%p from chunk %s (incount=%d)!\n",
                //  pLight.get(), chunk.identifier().c_str(), count );
            }
        }
    }
}

} // namespace

/**
 *  Constructor
 */
ChunkLightCache::ChunkLightCache( Chunk & chunk ) :
    chunk_( chunk ),
    pOwnLights_( NULL ),
    pAllLights_( NULL ),
    lightContainerDirty_( true ),
    heavenSeen_( false )
{
    BW_GUARD;
    pOwnLights_ = new Moo::LightContainer();
    pOwnLights_->ambientColour( Moo::Colour( 0, 0, 0, 1 ) );
}

/**
 *  Destructor
 */
ChunkLightCache::~ChunkLightCache()
{
    // smart pointers will release their references for us
}


void ChunkLightCache::update()
{

    // first of all collect all lights
    if (lightContainerDirty_)
    {
        this->collectLights();
        lightContainerDirty_ = false;
    }

    // update the ambient colour if it's changed
    if (heavenSeen_)
    {
        pAllLights_->ambientColour( chunk_.space()->ambientLight() );
    }
}


/**
 *  Draw method. We refresh ourselves if necessary, and
 *  then load ourselves into the Moo render context.
 */
void ChunkLightCache::draw( Moo::DrawContext& drawContext )
{
    BW_GUARD_PROFILER(ChunkLightCache_applyLight);
    static DogWatch drawWatch( "ChunkLightCache" );
    ScopedDogWatch watcher( drawWatch );
    if (Renderer::instance().pipelineType() == IRendererPipeline::TYPE_FORWARD_SHADING)
        return;

    update();

    // tell light manager about them.
    LightsManager::instance().gatherVisibleLights(pAllLights_);
}


/**
 *  Bind method. We flag our cache as dirty, because we
 *  have to pick up lights from adjoining chunks
 */
void ChunkLightCache::bind( bool isUnbind )
{
    BW_GUARD;
    dirtySeep();
}


/**
 *  This static function makes sure that a chunk light cache exists in
 *  the chunk that is about to be loaded, since we want to exist in
 *  every chunk so that their lighting is right.
 */
void ChunkLightCache::touch( Chunk & chunk )
{
    BW_GUARD;
    // this is all we have to do!
    ChunkLightCache::instance( chunk );
}


/**
 *  Flag this light container and the light container of all adjoining
 *  bound chunks as dirty.
 */
void ChunkLightCache::dirtySeep( int seepDepth, Chunk* parent )
{
    BW_GUARD;

    lightContainerDirty_ = true;

    if (seepDepth > 0)
    {
        for ( Chunk::piterator pit = chunk_.pbegin(); pit != chunk_.pend(); pit++ )
        {
            ChunkBoundary::Portal& portal = (*pit);

            if (!portal.hasChunk() || !portal.pChunk->isBound() || portal.pChunk == parent)
            {
                continue;
            }

            ChunkLightCache::instance( *portal.pChunk ).dirtySeep( seepDepth-1, &chunk_ );
        }
    }
}


/**
 *  Notification that one of our omni lights has moved (but is still in this chunk), 
 *  so check if light seepage needs to be updated.
 */
void ChunkLightCache::moveOmni( const Moo::OmniLightPtr& omni, 
                               const Vector3& opos, float oradius, 
                               bool transient )
{
    BW_GUARD_PROFILER( ChunkLightCache_moveOmni );
    pushLight( omni, opos, oradius, chunk_, transient );
}

/**
 *  Notification that one of our spot lights has moved (but is still in this chunk), 
 *  so check if light seepage needs to be updated.
 */
void ChunkLightCache::moveSpot( const Moo::SpotLightPtr& spot, 
                               const Vector3& opos, float oradius, 
                               bool transient )
{
    BW_GUARD_PROFILER( ChunkLightCache_moveSpot );
    pushLight( spot, opos, oradius, chunk_, transient );
}


/**
 *  Private method to collect lights that might
 *  seep through from adjoining chunks.
 */
void ChunkLightCache::collectLights()
{
    BW_GUARD_PROFILER( ChunkLightCache_collectLights );

    const BoundingBox& lightBB = chunk_.boundingBox();


    // Setup light containers
    pAllLights_ = new Moo::LightContainer();
    pAllLights_->ambientColour( pOwnLights_->ambientColour() );
    pAllLights_->addToSelf( pOwnLights_, lightBB, false );

    heavenSeen_ = chunk_.canSeeHeaven();
    if (heavenSeen_)
    {
        pAllLights_->ambientColour( chunk_.space()->ambientLight() );
        pAllLights_->addDirectional( chunk_.space()->sunLight() );
    }

    // Now find all neighbouring lights
    PTIVector tstack;
    tstack.reserve( MAX_LIGHT_SEEP_DEPTH );

    for ( Chunk::piterator pit = chunk_.pbegin(); pit != chunk_.pend(); pit++ )
    {
        ChunkBoundary::Portal& portal = (*pit);

        if (!portal.hasChunk() || !portal.pChunk->isBound())
            continue;

        tstack.push_back( PortalTraverseInfo(chunk_, portal) );
        pullIntersectingLights( tstack, *pAllLights_ );
        tstack.pop_back();
    }
}


/// Static instance accessor initialiser
ChunkCache::Instance<ChunkLightCache> ChunkLightCache::instance;

BW_END_NAMESPACE

// chunk_light.cpp
