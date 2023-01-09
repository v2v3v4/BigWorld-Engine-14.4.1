#include "pch.hpp"
#include "trail_particle_renderer.hpp"
#include "particle/particle_serialisation.hpp"
#include "moo/render_context.hpp"
#include "moo/moo_math.hpp"
#include "moo/geometrics.hpp"
#include "moo/fog_helper.hpp" 
#include "moo/draw_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

const BW::string TrailParticleRenderer::nameID_ = "TrailParticleRenderer";


// -----------------------------------------------------------------------------
//	Section - TrailParticleRenderer
// -----------------------------------------------------------------------------

TrailParticleRenderer::TrailParticleRenderer():	
	useFog_( true ),
	width_( 0.25f ),
	steps_(),
	particles_( NULL ),
	frame_( 0 ),
	skip_( 0 )
{
	BW_GUARD;
	material_.alphaTestEnable( false );
	material_.alphaBlended( true );
	material_.destBlend( Moo::Material::ONE );
	material_.srcBlend( Moo::Material::ONE );
	material_.zBufferRead( true );
	material_.zBufferWrite( false );
	material_.fogged( true );
	material_.doubleSided( true );

	Moo::TextureStage ts;
	ts.colourOperation( Moo::TextureStage::MODULATE,
		Moo::TextureStage::CURRENT,
		Moo::TextureStage::TEXTURE );
	ts.alphaOperation( Moo::TextureStage::DISABLE );
	material_.addTextureStage( ts );
	ts.colourOperation( Moo::TextureStage::DISABLE );
	material_.addTextureStage( ts );

	this->steps( 5 );
}


TrailParticleRenderer::~TrailParticleRenderer()
{
	BW_GUARD;
}


/**
 *	This method sets the texture name for the renderer.
 */
void TrailParticleRenderer::textureName( const BW::StringRef & v )
{
	BW_GUARD;
	Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get(
		v, true, true, true, "texture/particle" );
	if ( !pTexture.hasObject() )
		return;
	textureName_.assign( v.data(), v.size() );
	material_.textureStage(0).pTexture( pTexture );
}


void TrailParticleRenderer::draw( Moo::DrawContext& drawContext,
								 const Matrix & worldTransform,
								 Particles::iterator beg,
								 Particles::iterator end,
								 const BoundingBox & inbb )
{
	BW_GUARD;
	if ( beg == end ) return;

	float distance = 
		(worldTransform.applyToOrigin() - Moo::rc().invView().applyToOrigin()).length();
	sortedDrawItem_.set( this, worldTransform, beg, end );
	drawContext.drawUserItem( &sortedDrawItem_,
		Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );

}


void TrailParticleRenderer::update( Particles::iterator beg,
		Particles::iterator end, float dTime )
{
	BW_GUARD;

	frame_++;
	if ( ( skip_ == 0 ) || ( ( frame_ % skip_ ) == 0 ) )
	{
		particles_->copyPositions();
	}
}


/**
 *	This method views the given particle system using this renderer.
 */
void TrailParticleRenderer::realDraw( const Matrix & worldTransform,
	Particles::iterator beg,
	Particles::iterator end )
{
	BW_GUARD_PROFILER( TrailParticleRenderer_realDraw );

	if ( beg == end )
	{
		return;
	}

	//set render states
	this->setupRenderState();
	material_.set();

	//draw
	// Iterate through the particles.

	Vector3 a;
	Vector3 b;

	float uvStep = 1.f / (float)steps_;

	if ( Geometrics::beginTexturedWorldLines() )
	{
		Particles::iterator iter = beg;
		while ( iter != end )
		{
			Particle &particle = *iter++;

			if (!particle.isAlive())
				continue;

			float uvStart = 0.f;
			a = particles_->getPosition( particle.index(), 0 );

			for ( int i = 1; i < steps_; i++ )
			{
				b = particles_->getPosition( particle.index(), i );
				Geometrics::texturedWorldLine( a, b, width_, Moo::Colour(particle.colour()), uvStep, uvStart );
				a = b;
				uvStart += uvStep;
			}
		}
		Geometrics::endTexturedWorldLines( local() ? worldTransform : Matrix::identity );
	}
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
}

size_t TrailParticleRenderer::sizeInBytes() const
{
	BW_GUARD;

	return sizeof(TrailParticleRenderer);
}


TrailParticleRenderer::TrailParticles::TrailParticles()
:	positionHistories_( NULL )
{
}

TrailParticleRenderer::TrailParticles::~TrailParticles()
{
	bw_safe_delete_array(positionHistories_);
}

Particle* TrailParticleRenderer::TrailParticles::addParticle( const Particle &particle, bool isMesh )
{
	Particle* p = FixedIndexParticles::addParticle( particle, isMesh );
	this->resetHistory( *p );
	return p;
}

void TrailParticleRenderer::TrailParticles::reserve( size_t amount )
{
	FixedIndexParticles::reserve( amount );
	for ( uint i = 0; i < steps_; i++ )
	{
		positionHistories_[i].resize( amount );
	}
}

Vector3 TrailParticleRenderer::TrailParticles::getPosition( uint index, uint age ) const
{
	uint h = ( steps_ + historyIndex_ - age - 1 ) % steps_;
	Vector3 v = ( positionHistories_[h] )[index];
	return ( positionHistories_[h] )[index];
}

void TrailParticleRenderer::TrailParticles::copyPositions()
{
	for ( iterator it = begin(); it != end(); ++it )
	{
		Particle &particle = *it;
		if ( particle.isAlive() )
		{
			Vector3 v = particle.position();
			( this->positionHistories_[historyIndex_] )[particle.index()] = particle.position();
		}
	}

	historyIndex_ = ( historyIndex_ + 1 ) % steps_;
}

void TrailParticleRenderer::TrailParticles::historySteps( uint32 steps )
{
	size_t size = 0;

	if ( positionHistories_ )
	{
		size = positionHistories_[0].size();
		bw_safe_delete_array(positionHistories_);
	}

	steps_ = steps;
	historyIndex_ = 0;
	positionHistories_ = new BW::vector<Vector3>[steps_];

	// If reserve() has been called in the past we need to re-do the
	// reservation on the new vectors.
	if ( size )
	{
		for ( uint i = 0; i < steps_; i++ )
		{
			positionHistories_[i].resize( size );
		}
	}
}

void TrailParticleRenderer::TrailParticles::resetHistory( const Particle &particle )
{
	for ( uint i = 0; i < steps_; i++ )
	{
		Vector3 v = particle.position();
		( positionHistories_[i] )[ particle.index() ] = particle.position();
	}
}


template < typename Serialiser >
void TrailParticleRenderer::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, TrailParticleRenderer, width );
	BW_PARITCLE_SERIALISE( s, TrailParticleRenderer, skip );
	// tagged "useFog" for legacy reasons
	BW_PARITCLE_SERIALISE_NAMED( s, TrailParticleRenderer, useFog, "useFog" );
	// tagged "steps" for legacy reasons
	BW_PARITCLE_SERIALISE_NAMED( s, TrailParticleRenderer, steps, "steps" );
	BW_PARITCLE_SERIALISE( s, TrailParticleRenderer, textureName );
}


void TrailParticleRenderer::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< TrailParticleRenderer >( pSect,
		this ) );
}


void TrailParticleRenderer::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< TrailParticleRenderer >( pSect,
		this ) );
}


TrailParticleRenderer * TrailParticleRenderer::clone() const
{
	BW_GUARD;
	TrailParticleRenderer * psr = new TrailParticleRenderer();
	ParticleSystemRenderer::clone( psr );
	this->serialise( BW::CloneObject< TrailParticleRenderer >( this, psr ) );
	return psr;
}


/*static*/ void TrailParticleRenderer::prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output )
{
	BW_GUARD;
	const BW::string& name = pSection->readString( "textureName_" );
	if (name.length())
		output.insert(name);
}



void TrailParticleRenderer::steps( int value )
{
	BW_GUARD;

	steps_ = value;

	if ( particles_ )
	{
		particles_->historySteps( steps_ );
	}
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to the PyTrailParticleRenderer.
// -----------------------------------------------------------------------------


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyTrailParticleRenderer::

PY_TYPEOBJECT( PyTrailParticleRenderer )

/*~ function Pixie.TrailParticleRenderer
 *	This function creates and returns a new PyTrailParticleRenderer object.
 */
PY_FACTORY_NAMED( PyTrailParticleRenderer, "TrailParticleRenderer", Pixie )

PY_BEGIN_METHODS( PyTrailParticleRenderer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyTrailParticleRenderer )	
	/*~ attribute PyTrailParticleRenderer.textureName
	 *	This attribute specifies the name of the texture used to render the
	 *	trails of the particles.
	 */
	PY_ATTRIBUTE( textureName )
	/*~ attribute PyTrailParticleRenderer.width
	 *	This attribute specifies the width of the line renderer as the trail
	 *	on the particles.
	 */
	PY_ATTRIBUTE( width )
	/*~ attribute PyTrailParticleRenderer.steps
	 *	The step attribute specifies the number of old particle positions
	 *	to remember for each of the particles. These are points which the
	 *	TrailParticleRenderer uses to render the trail lines for each particle.
	 *	Default value is 5.
	 *	@type Integer.
	 */
	PY_ATTRIBUTE( steps )
	/*~ attribute PyTrailParticleRenderer.skip
	 *	The attribute skip is used to skip the render caching of trail elements
	 *	every skip-1 frames. Thus if skip is 5, TrailParticleRenderer only
	 *	caches render elements every 5 frames - based off the formula
	 *	(frame % skip) == 0. Default value is 0 (no skipping).
	 *	@type Integer.
	 */
	PY_ATTRIBUTE( skip )
	/*~ attribute PyTrailParticleRenderer.useFog
	 *	Specifies whether the renderer should enable scene fogging or not.  You
	 *	may want to turn off scene fogging if you are using particle systems in
	 *	the sky box, or if you want to otherwise explicitly control the amount
	 *	of fogging via the tint shader fog blend control.
	 */
	PY_ATTRIBUTE( useFog )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
PyTrailParticleRenderer::PyTrailParticleRenderer( TrailParticleRendererPtr pR, PyTypeObject *pType ):
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
 *	Static Python factory method. This is declared through the factory
 *	declaration in the class definition.
 *
 *	@param	args	The list of parameters passed from Python. This should
 *					just be a string (textureName.)
 */
PyObject *PyTrailParticleRenderer::pyNew( PyObject *args )
{
	BW_GUARD;
	char *nameFromArgs = "None";
	if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
	{
		PyErr_SetString( PyExc_TypeError, "TrailParticleRenderer() expects "
			"an optional texture name string" );
		return NULL;
	}

	TrailParticleRenderer * apr = new TrailParticleRenderer();
	if ( _stricmp(nameFromArgs,"None") )
		apr->textureName( BW::string( nameFromArgs ) );

	return new PyTrailParticleRenderer(apr);
}

BW_END_NAMESPACE

// trail_particle_renderer.cpp
