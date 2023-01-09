#include "pch.hpp"
#include "visual_particle_renderer.hpp"
#include "particle/particle_serialisation.hpp"
#include "moo/render_context.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/visual_manager.hpp"
#include "moo/moo_math.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

const BW::string VisualParticleRenderer::nameID_ = "VisualParticleRenderer";


// -----------------------------------------------------------------------------
//	Section - VisualParticleRenderer
// -----------------------------------------------------------------------------

VisualParticleRenderer::VisualParticleRenderer()
{
}


VisualParticleRenderer::~VisualParticleRenderer()
{
}


/**
 *	This method sets the visual name for the renderer.
 */
void VisualParticleRenderer::visual( const BW::StringRef & v )
{
	BW_GUARD;
	visualName_.assign( v.data(), v.size() );
	if (v.empty())
    {
		pVisual_ = NULL;
    }
	else
    {        
		pVisual_ = Moo::VisualManager::instance()->get( visualName_ );
    }
}


/**
 * TODO: to be documented.
 */
class ParticleMeshTintConstant : public Moo::EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		pEffect->SetVector( constantHandle, (D3DXVECTOR4*)&tint_ );		

		return true;
	}

    void tint( const Vector4 & colour ) { tint_ = colour; }

private:
	Vector4 tint_;
};


static SmartPointer<ParticleMeshTintConstant> s_particleMeshTintConstant = 
    new ParticleMeshTintConstant;


/**
 *	This method views the given particle system using this renderer.
 */
void VisualParticleRenderer::draw( Moo::DrawContext& drawContext,
								  const Matrix& worldTransform,
								  Particles::iterator beg,
								  Particles::iterator end,
								  const BoundingBox & inbb )
{
	BW_GUARD;
	if ( beg == end ) return;
	if ( !pVisual_ ) return;

	DX::Device* device = Moo::rc().device();	

	Moo::rc().push();

	size_t nParticles = end - beg;
	Matrix particleTransform;

	Particles::iterator it = beg;

	while ( it!=end )
	{		
		Particle &particle = *it++;

		if (!particle.isAlive())
			continue;

        // Set particle transform as world matrix
		particleTransform.setRotate( particle.yaw(), particle.pitch(), 0.f );
		particleTransform.translation( particle.position() );
        // Add spin:
        float spinSpeed = particle.meshSpinSpeed();
		if (spinSpeed != 0.f )
		{
            Vector3 spinAxis = particle.meshSpinAxis();
			Matrix spin;
			D3DXMatrixRotationAxis
            ( 
                &spin, 
                &spinAxis, 
                spinSpeed * PARTICLE_MESH_MAX_SPIN * (particle.age() - particle.meshSpinAge()) 
            );
			particleTransform.preMultiply( spin );
		}

		if (local())
			particleTransform.postMultiply(worldTransform);
		Moo::rc().world( particleTransform );

        static Moo::EffectConstantValuePtr* pMeshTint_ = NULL;
		if ( !pMeshTint_ )
		{
			pMeshTint_ = Moo::rc().effectVisualContext().getMapping( "ParticleMeshTint" );
		}
        s_particleMeshTintConstant->tint(Colour::getVector4Normalised(particle.colour()));
		*pMeshTint_ = s_particleMeshTintConstant;

		if (pVisual_)
            pVisual_->draw( drawContext, true );
	}

	Moo::rc().pop();
}


template < typename Serialiser >
void VisualParticleRenderer::serialise( const Serialiser & s ) const
{
	// tagged "visualName_" for legacy resons
	BW_PARITCLE_SERIALISE_NAMED( s, VisualParticleRenderer, visual,
		"visualName_" );
}


void VisualParticleRenderer::loadInternal( DataSectionPtr pSect )
{
	BW_GUARD;
	this->serialise( BW::LoadFromDataSection< VisualParticleRenderer >( pSect,
		this ) );
}


void VisualParticleRenderer::saveInternal( DataSectionPtr pSect ) const
{
	BW_GUARD;
	this->serialise( BW::SaveToDataSection< VisualParticleRenderer >( pSect,
		this ) );
}


VisualParticleRenderer * VisualParticleRenderer::clone() const
{
	BW_GUARD;
	VisualParticleRenderer * psr = new VisualParticleRenderer();
	ParticleSystemRenderer::clone( psr );
	this->serialise( BW::CloneObject< VisualParticleRenderer >( this, psr ) );
	return psr;
}


/*static*/ void VisualParticleRenderer::prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output )
{
	BW_GUARD;
	const BW::string& name = pSection->readString( "visualName_" );
	if (name.length())
		output.insert(name);
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to the PyVisualParticleRenderer.
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyVisualParticleRenderer::

PY_TYPEOBJECT( PyVisualParticleRenderer )

/*~ function Pixie.VisualParticleRenderer
 *	Factory function to create and return a new PyVisualParticleRenderer object. A
 *	VisualParticleRenderer is a ParticleSystemRenderer which renders each particle
 *	as a visual.
 *	Note that it is quicker to use MeshParticle objects exported specifically for
 *	use with the MeshParticleRenderer, as they can be drawn 15-at-a-time; however
 *	the MeshParticle objects have more limited material functionality.  The
 *	VisualParticleRenderer can render any material.
 *	@return A new PyVisualParticleRenderer object.
 */
PY_FACTORY_NAMED( PyVisualParticleRenderer, "VisualParticleRenderer", Pixie )

PY_BEGIN_METHODS( PyVisualParticleRenderer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyVisualParticleRenderer )	
	/*~ attribute PyVisualParticleRenderer.visual
	 *	This is the name of the visual particle type to use for the particles.
	 *	Default value is "".
	 *	@type String.
	 */
	PY_ATTRIBUTE( visual )    
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
PyVisualParticleRenderer::PyVisualParticleRenderer( VisualParticleRendererPtr pR, PyTypeObject *pType ):
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
PyObject *PyVisualParticleRenderer::pyNew( PyObject *args )
{
	BW_GUARD;
	char *nameFromArgs = "None";
	if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
	{
		PyErr_SetString( PyExc_TypeError, "VisualParticleRenderer() expects "
			"an optional visual name string" );
		return NULL;
	}

	VisualParticleRenderer * vpr = new VisualParticleRenderer();
	if ( _stricmp(nameFromArgs,"None") )
		vpr->visual( BW::string( nameFromArgs ) );

	return new PyVisualParticleRenderer(vpr);
}

BW_END_NAMESPACE

// visual_particle_renderer.cpp
