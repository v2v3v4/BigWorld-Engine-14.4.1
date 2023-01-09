#include "pch.hpp"
#include "blur_particle_renderer.hpp"
#include "particle/particle_serialisation.hpp"
#include "moo/render_context.hpp"
#include "moo/moo_math.hpp"
#include "moo/geometrics.hpp"
#include "moo/fog_helper.hpp" 
#include "moo/draw_context.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
//	Section - BlurParticleRenderer
// -----------------------------------------------------------------------------
BlurParticleRenderer::BlurParticleRenderer():
	useFog_( true ),
	width_( 0.25f ),
	time_( 0.25f )
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
}


BlurParticleRenderer::~BlurParticleRenderer()
{
}


/**
 *	This method sets the texture name for the renderer.
 */
void BlurParticleRenderer::textureName( const BW::StringRef & v )
{
	BW_GUARD;
	Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get( v, true, true, true, "texture/particle" );
	if ( !pTexture.hasObject() )
		return;
	textureName_.assign( v.data(), v.size() );
	material_.textureStage(0).pTexture( pTexture );
}


void BlurParticleRenderer::draw( Moo::DrawContext& drawContext,
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


/**
 *	This method views the given particle system using this renderer.
 */
void BlurParticleRenderer::realDraw( const Matrix & worldTransform,
	Particles::iterator beg,
	Particles::iterator end )
{
	BW_GUARD;
	if ( beg == end ) return;

	//set render states
	this->setupRenderState();
	material_.set();

	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	//draw
	// Iterate through the particles.
	if ( Geometrics::beginTexturedWorldLines() )
	{
		Particles::iterator iter = beg;
		while ( iter != end )
		{
			Particle &particle = *iter++;

			if (particle.isAlive())
			{
				Vector3 velocity;
				particle.getVelocity( velocity );

				Geometrics::texturedWorldLine( 
					particle.position(), 
					particle.position() - (velocity * time_), width_, 
					Moo::Colour(particle.colour()), 1.f, 0.f );
			}
		}
		Geometrics::endTexturedWorldLines( local() ? worldTransform : Matrix::identity );
	}

	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
}


template < typename Serialiser >
void BlurParticleRenderer::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, BlurParticleRenderer, width );
	BW_PARITCLE_SERIALISE( s, BlurParticleRenderer, time );
	// tagged "useFog" for legacy reasons
	BW_PARITCLE_SERIALISE_NAMED( s, BlurParticleRenderer, useFog, "useFog" );
	BW_PARITCLE_SERIALISE( s, BlurParticleRenderer, textureName );
}


void BlurParticleRenderer::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< BlurParticleRenderer >( pSect,
		this ) );
}


void BlurParticleRenderer::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< BlurParticleRenderer >( pSect,
		this ) );
}


BlurParticleRenderer * BlurParticleRenderer::clone() const
{
	BW_GUARD;
	BlurParticleRenderer * psr = new BlurParticleRenderer();
	ParticleSystemRenderer::clone( psr );
	this->serialise( BW::CloneObject< BlurParticleRenderer >( this, psr ) );
	return psr;
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to the PyBlurParticleRenderer.
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyBlurParticleRenderer::

PY_TYPEOBJECT( PyBlurParticleRenderer )

/*~ function Pixie.BlurParticleRenderer
 *	This function creates and returns a new PyBlurParticleRenderer object.
 */
PY_FACTORY_NAMED( PyBlurParticleRenderer, "BlurParticleRenderer", Pixie )

PY_BEGIN_METHODS( PyBlurParticleRenderer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyBlurParticleRenderer )	
	/*~ attribute PyBlurParticleRenderer.textureName
	 *	This attribute specifies the name of the texture used to render the
	 *	trails of the particles.
	 */
	PY_ATTRIBUTE( textureName )
	/*~ attribute PyBlurParticleRenderer.width
	 *	This attribute specifies the width of the line renderer as the trail
	 *	on the particles.
	 */
	PY_ATTRIBUTE( width )
	/*~ attribute PyBlurParticleRenderer.time
	 *	The time attribute specifies the length of the trail, in seconds.
	 *	The length of the tail is (particle velocity * time)
	 *	Default value is 0.25.
	 *	@type Float.
	 */
	 PY_ATTRIBUTE( time )
	 /*~ attribute PyBlurParticleRenderer.useFog
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
PyBlurParticleRenderer::PyBlurParticleRenderer( BlurParticleRendererPtr pR, PyTypeObject *pType ):
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
PyObject *PyBlurParticleRenderer::pyNew( PyObject *args )
{
	BW_GUARD;
	char *nameFromArgs = "None";
	if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
	{
		PyErr_SetString( PyExc_TypeError, "BlurParticleRenderer() expects "
			"an optional texture name string" );
		return NULL;
	}

	BlurParticleRenderer * apr = new BlurParticleRenderer();
	if ( bw_stricmp(nameFromArgs,"None") )
		apr->textureName( BW::string( nameFromArgs ) );

	return new PyBlurParticleRenderer(apr);
}

const BW::string BlurParticleRenderer::nameID_ = "BlurParticleRenderer";

BW_END_NAMESPACE

// blur_particle_renderer.cpp
