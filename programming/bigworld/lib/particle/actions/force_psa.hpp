#ifndef FORCE_PSA_HPP
#define FORCE_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

// Forward Class Declarations.
class ParticleSystem;
class VectorGenerator;


/*~ class Pixie.PyForcePSA
 *	PyForcePSA is a PyParticleSystemAction that applies a constant acceleration to
 *	particles in a particular direction.
 *
 *	A new PyForcePSA is created using Pixie.ForcePSA function.
 */

/**
 *	This action acts as a force to the particles. The force PSA is the
 *	Newtonian concept of a force, that is, it applies an acceleration to
 *	the particles along a particular direction.
 *
 */
class ForcePSA : public ParticleSystemAction
{
public:
	///	@name Constructor(s)
	//@{
	ForcePSA( float x = 0.0f, float y = 0.0f, float z = 0.0f );
	ForcePSA( const Vector3 &newVector );
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Accessors for the ForcePSA.
	//@{
	const Vector3 &vector( void ) const;
	void vector( const Vector3 &newVector );
	//@}

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem &particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(ForcePSA); }
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Auxiliary Variables for the ForcePSA.
	//@{
	static int typeID_;		///< TypeID of the ForcePSA.

	Vector3 vector_;		///< The vector describing the force.
	//@}
};

typedef SmartPointer<ForcePSA> ForcePSAPtr;


class PyForcePSA : public PyParticleSystemAction
{
	Py_Header( PyForcePSA, PyParticleSystemAction )
public:
	PyForcePSA( ForcePSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Accessors for the PyForcePSA.
	//@{
	const Vector3 &vector( void ) const;
	void vector( const Vector3 &newVector );
	//@}

	///	@name Python Interface to the PyCollidePSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, vector, vector )
	//@}
private:
	ForcePSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyForcePSA )


#ifdef CODE_INLINE
#include "force_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* force_psa.hpp */
