#ifndef PY_PARTICLE_SYSTEM_HPP
#define PY_PARTICLE_SYSTEM_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/stl_to_py.hpp"
#include "duplo/py_attachment.hpp"
#include "particle_system.hpp"


BW_BEGIN_NAMESPACE

class ParticleSystemRenderer;
typedef SmartPointer<ParticleSystemRenderer> ParticleSystemRendererPtr;

/*~ class Pixie.PyParticleSystem
 *
 *	The PyParticleSystem class provides a base particle system to manage
 *	particles. A particle system must have particle system actions attached to
 *	it to specify how the particle system evolves over time.
 *	
 *	A particle system must have at least a souce particle generator (SourcePSA)
 *	and a renderer specified for it to be able to generate particles. The two
 *	main types of particle renderers are SpriteParticleRenderer and
 *	MeshParticleRenderer.
 *
 *	A new PyParticleSystem is created using Pixie.ParticleSystem function.
 */
/**
 *	This class is the abstract picture of the particles that is invariant
 *	to a renderer.
 */
class PyParticleSystem : public PyAttachment
{
	Py_Header( PyParticleSystem, PyAttachment )

public:	
	/// @name Constructor(s) and Destructor.
	//@{
	PyParticleSystem( ParticleSystemPtr pSystem, PyTypeObject *pType = &s_type_ );
	~PyParticleSystem();
	//@}

	/// A collection of PyActions for a Particle System.
	typedef BW::vector<ParticleSystemActionPtr> PyActions;

	/// @name PyAttachment overrides
	//@{
	virtual void tick( float dTime );

	virtual bool needsUpdate() const { return true; }
	virtual bool isLodVisible() const;
#if ENABLE_TRANSFORM_VALIDATION
	virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
	virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod );

	virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );
	virtual void localBoundingBox( BoundingBox & bb, bool skinny = false );
	virtual void localVisibilityBox( BoundingBox & vbb, bool skinny = false );
	virtual void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false );
	virtual void worldVisibilityBox( BoundingBox & vbb, const Matrix& world, bool skinny = false );
	virtual bool attach( MatrixLiaison * pOwnWorld );
	virtual void detach();
	virtual void enterWorld();
	virtual void leaveWorld();
	//@}


	/// @name Particle System attribute accessors.
	//@{
	ParticleSystemPtr pSystem()		{ return pSystem_; }

	int capacity( void ) const		{ return pSystem_->capacity(); }
	void capacity( int number )		{ pSystem_->capacity( number );}

	float windFactor( void ) const	{ return pSystem_->windFactor(); }
	void windFactor( float ratio )	{ pSystem_->windFactor( ratio ); }

	void explicitPosition( const Vector3& v )	{ pSystem_->explicitPosition(v); }
	const Vector3& explicitPosition() const		{ return pSystem_->explicitPosition(); }

	void explicitDirection( const Vector3& v )	{ pSystem_->explicitDirection(v); }
	const Vector3& explicitDirection() const	{ return pSystem_->explicitDirection(); }

	float fixedFrameRate() const	{ return pSystem_->fixedFrameRate(); }
	void fixedFrameRate( float f )	{ pSystem_->fixedFrameRate(f); }

	void maxLod(float m)			{ pSystem_->maxLod( m ); }
	float maxLod() const			{ return pSystem_->maxLod(); }

	float duration() const			{ return pSystem_->duration(); }

	void pRenderer( PyParticleSystemRendererPtr pNewRenderer )
	{
		pSystem_->pRenderer( pNewRenderer->pRenderer() );
	}

	PyParticleSystemRendererPtr pRenderer()
	{		
		return PyParticleSystemRenderer::createPyRenderer(pSystem_->pRenderer());
	}


	/// @name The Python Interface to the Particle System.
	//@{
	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_addAction )
	PY_METHOD_DECLARE( py_removeAction )
	PY_METHOD_DECLARE( py_action )
	PY_METHOD_DECLARE( py_clear )
	PY_METHOD_DECLARE( py_update )
	PY_METHOD_DECLARE( py_render )
	PY_METHOD_DECLARE( py_load )
	PY_METHOD_DECLARE( py_save )
	PY_METHOD_DECLARE( py_size )
	PY_METHOD_DECLARE( py_force )
	PY_AUTO_METHOD_DECLARE( RETDATA, duration, END )

	PY_RW_ATTRIBUTE_DECLARE( actionsHolder_, actions )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, fixedFrameRate, fixedFrameRate )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, capacity, capacity )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, windFactor, windFactor )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( PyParticleSystemRendererPtr, pRenderer, renderer )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, explicitPosition, explicitPosition )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, explicitDirection, explicitDirection )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxLod, maxLod )	
	//@}

private:
	ParticleSystemPtr		pSystem_;
	float lod_;

	/// Python Interface to the Actions vector.
	PySTLSequenceHolder<ParticleSystem::Actions> actionsHolder_;
};


void ParticleSystemAction_release( ParticleSystemAction *&pAction );
/*
 *	Wrapper to let the C++ vector of actions work as a vector/list in Python.
 */
namespace PySTLObjectAid
{
	/// Releaser is same for all PyObject's
	template <> struct Releaser< ParticleSystemAction * >
	{
		static void release( ParticleSystemAction *&pAction )
        {
        	ParticleSystemAction_release( pAction );
        }
	};
}

PY_SCRIPT_CONVERTERS_DECLARE( PyParticleSystem )

#ifdef CODE_INLINE
#include "py_particle_system.ipp"
#endif

BW_END_NAMESPACE

#endif // PY_PARTICLE_SYSTEM_HPP

/* py_particle_system.hpp */
