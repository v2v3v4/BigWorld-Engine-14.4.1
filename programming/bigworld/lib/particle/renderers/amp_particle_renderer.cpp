#include "pch.hpp"
#include "amp_particle_renderer.hpp"
#include "particle/particle_serialisation.hpp"
#include "moo/draw_context.hpp"
#include "moo/render_context.hpp"
#include "moo/moo_math.hpp"
#include "moo/geometrics.hpp"
#include "moo/fog_helper.hpp"
#include "moo/draw_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

const BW::string AmpParticleRenderer::nameID_ = "AmpParticleRenderer";


// -----------------------------------------------------------------------------
//	Section - AmpParticleRenderer
// -----------------------------------------------------------------------------
AmpParticleRenderer::AmpParticleRenderer():	
	useFog_( true ),
	width_( 0.25f ),
	height_( 1.f ),
	steps_( 5 ),
	variation_( 1.f ),
	circular_( false )
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


AmpParticleRenderer::~AmpParticleRenderer()
{
}


/**
 *	This method sets the texture name for the renderer.
 */
void AmpParticleRenderer::textureName( const BW::StringRef & v )
{
	BW_GUARD;
	Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get(
		v, true, true, true, "texture/particle" );
	if ( !pTexture.hasObject() )
		return;
	textureName_.assign( v.data(), v.size() );
	material_.textureStage( 0 ).pTexture( pTexture );
}


void AmpParticleRenderer::draw( Moo::DrawContext& drawContext,
							   const Matrix & worldTransform,
							   Particles::iterator beg,
							   Particles::iterator end,
							   const BoundingBox & inbb )
{
	BW_GUARD;
	if ( beg != end )
	{
		float distance = (worldTransform.applyToOrigin() - Moo::rc().invView().applyToOrigin()).length();
		sortedDrawItem_.set( this, worldTransform, beg, end );
		drawContext.drawUserItem( &sortedDrawItem_,
			Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );

	}
}


/**
 *	This method views the given particle system using this renderer.
 */
void AmpParticleRenderer::realDraw( const Matrix & worldTransform,
	Particles::iterator beg,
	Particles::iterator end )
{
	BW_GUARD;
	if ( beg == end ) return;

	this->setupRenderState();
	material_.set();

	//draw
	// Iterate through the particles.
	if ( Geometrics::beginTexturedWorldLines() )
	{
		bool circular = false;

		float v = variation_;
		Vector3 origin = local() ? Vector3::zero() : worldTransform.applyToOrigin();
		bool first = true;
		Vector3 last;

		Particles::iterator iter = beg;
		while ( iter != end )
		{
			Particle &particle = *iter;

			if (!particle.isAlive())
			{
				++iter;
				continue;
			}

			if ( circular_ && first )
			{
				last = particle.position();
				first = false;
				++iter;
				continue;
			}

			Vector3 start(circular_ ? last : origin);
			Vector3 pos(start);
			Vector3 dpos = particle.position();
			dpos -= start;
			dpos /= (float)steps_;

			v = variation_ * powf( dpos.length(), 0.1f );	//so longer lines have more variance ( adheres to self similarity constraint )

			for ( int i=0; i<steps_; i++ )
			{
				pos += dpos;

				float x = (float)rand() / (float)RAND_MAX - (float)rand() / (float)RAND_MAX;
				float y = (float)rand() / (float)RAND_MAX - (float)rand() / (float)RAND_MAX;
				float z = (float)rand() / (float)RAND_MAX - (float)rand() / (float)RAND_MAX;
				Vector3 rd(x*v, y*v, z*v);
				if ( i<(steps_-1))
					Geometrics::texturedWorldLine( start, pos + rd, width_, particle.colour(), height_ );
				else
					Geometrics::texturedWorldLine( start, particle.position(), width_, particle.colour(), height_ );
				start = pos + rd;
			}

			if ( circular_ )
				last = particle.position();

			++iter;
		}

		Geometrics::endTexturedWorldLines( local() ? worldTransform : Matrix::identity );
	}

	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	//Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );

}


template < typename Serialiser >
void AmpParticleRenderer::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, AmpParticleRenderer, width );
	BW_PARITCLE_SERIALISE( s, AmpParticleRenderer, height );
	BW_PARITCLE_SERIALISE( s, AmpParticleRenderer, steps );
	BW_PARITCLE_SERIALISE( s, AmpParticleRenderer, variation );
	BW_PARITCLE_SERIALISE( s, AmpParticleRenderer, circular );
	// // tagged "useFog" for legacy reasons
	BW_PARITCLE_SERIALISE_NAMED( s, AmpParticleRenderer, useFog, "useFog" );
	BW_PARITCLE_SERIALISE( s, AmpParticleRenderer, textureName );
}


void AmpParticleRenderer::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< AmpParticleRenderer >( pSect,
		this ) );
}


void AmpParticleRenderer::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< AmpParticleRenderer >( pSect,
		this ) );
}


AmpParticleRenderer * AmpParticleRenderer::clone() const
{
	BW_GUARD;
	AmpParticleRenderer * psr = new AmpParticleRenderer();
	ParticleSystemRenderer::clone( psr );
	this->serialise( BW::CloneObject< AmpParticleRenderer >( this, psr ) );
	return psr;
}

/*static*/ void AmpParticleRenderer::prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output )
{
	BW_GUARD;
	const BW::string& textureName = pSection->readString( "textureName_" );
	if (textureName.length())
		output.insert(textureName);
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to the AmpParticleRenderer.
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyAmpParticleRenderer::

PY_TYPEOBJECT( PyAmpParticleRenderer )

/*~ function Pixie.AmpParticleRenderer
 *	Factory function to create and return a new PyAmpParticleRenderer object. An
 *	AmpParticleRenderer is a ParticleRenderer that renders segmented lines from
 *	the particle position back to the origin of the particle system. 
 *	@return A new PyAmpParticleRenderer object.
 */
PY_FACTORY_NAMED( PyAmpParticleRenderer, "AmpParticleRenderer", Pixie )

PY_BEGIN_METHODS( PyAmpParticleRenderer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyAmpParticleRenderer )	
	/*~ attribute PyAmpParticleRenderer.textureName
	 *	Name of the texture used to render the lines.
	 *	@type String. Default is "".
	 */
	PY_ATTRIBUTE( textureName )
	/*~ attribute PyAmpParticleRenderer.width
	 *	Width is the thickness of the lines drawn.
	 *	@type Float. Default is 0.25.
	 */
	PY_ATTRIBUTE( width )
	/*~ attribute PyAmpParticleRenderer.height
	 *	Height is applied as a division to the length of the line, to determine
	 *	texture repeat along the u coordinate in uv texture coordinate space.
	 *	@type Float. Default is 1.0.
	 */
	PY_ATTRIBUTE( height )
	/*~ attribute PyAmpParticleRenderer.steps
	 *	Steps is the number of lines drawn between the start of the line
	 *	(origin) and the end of the line (particle position).
	 *	@type Float. Default is 1.0.
	 */
	PY_ATTRIBUTE( steps )
	/*~ attribute PyAmpParticleRenderer.variation
	 *	Variation is a scaling factor for random variation of the segment
	 *	positions when the number of steps is > 1. The default random factor
	 *	is 0 to 1 scaled by this value and function ofthe line length.
	 *	@type Float. Default is 1.0.
	 */
	PY_ATTRIBUTE( variation )
	/*~ attribute PyAmpParticleRenderer.circular
	 *	Circular determines whether the origin is the source position of the
	 *	particle system, or the position of the previous particles (ordered by
	 *	age).
	 *	@type Integer as boolean. 0 or 1. Default is false (0).
	 */
	PY_ATTRIBUTE( circular )
	/*~ attribute PyAmpParticleRenderer.useFog
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
PyAmpParticleRenderer::PyAmpParticleRenderer( AmpParticleRendererPtr pR, PyTypeObject *pType ):
	PyParticleSystemRenderer( pR, pType ),
	pR_(pR)
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
PyObject *PyAmpParticleRenderer::pyNew( PyObject *args )
{
	BW_GUARD;
	char *nameFromArgs = "None";
	if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
	{
		PyErr_SetString( PyExc_TypeError, "AmpParticleRenderer() expects "
			"an optional texture name string" );
		return NULL;
	}

	AmpParticleRenderer * apr = new AmpParticleRenderer();
	if ( bw_stricmp(nameFromArgs,"None") )
		apr->textureName( BW::string( nameFromArgs ) );

	PyAmpParticleRenderer * pyAmp = new PyAmpParticleRenderer(apr);

	return pyAmp;
}

BW_END_NAMESPACE

// amp_particle_renderer.cpp
