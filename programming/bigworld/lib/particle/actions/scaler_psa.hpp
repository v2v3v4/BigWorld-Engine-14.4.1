#ifndef SCALER_PSA_HPP
#define SCALER_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This action grows or shrinks a particle. The scaler PSA accepts a new size
 *	value for the particle and period of time. The scaling rate is the value
 *	per second that the particle's size moves towards the new size.
 *
 *	At present the scaling is linear between the two sizes. It is also 
 *	absolute - that is, all particles will scale eventually to the same
 *	size, regardless of their original size. Larger particles will shrink to
 *	correct size; smaller particles will grow to the correct size.
 */
class ScalerPSA : public ParticleSystemAction
{
public:
	///	@name Constructor(s) and Destructor.
	//@{
	ScalerPSA( float newSize = 0.0f, float newRate = 0.0f );	
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Accessors to ScalerPSA properties.
	//@{
	float size( void ) const;
	void size( float newSize );

	float rate( void ) const;
	void rate( float newRate );
	//@}

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem &particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(ScalerPSA); }
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Properties of the ScalerPSA.
	//@{
	float size_;		///< Eventual size of the particles.
	float rate_;		///< Increment per second towards the size.
	//@}

	///	@name Auxiliary Variables for the ScalerPSA.
	//@{
	static int typeID_;			///< TypeID of the ScalerPSA.
	//@}
};

typedef SmartPointer<ScalerPSA> ScalerPSAPtr;


/*~ class Pixie.PyScalerPSA
 *
 *	PyScalerPSA is a PyParticleSystemAction that scales a particle over time.
 *	All particles eventually end up at the specified size, and the rate
 *	determines the speed at which they converge linearly to this size.
 *
 *	A new PyScalerPSA is created using Pixie.ScalerPSA function.
 */
class PyScalerPSA : public PyParticleSystemAction
{
	Py_Header( PyScalerPSA, PyParticleSystemAction )
public:
	PyScalerPSA( ScalerPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Accessors to PyScalerPSA properties.
	//@{
	float size( void ) const;
	void size( float newSize );

	float rate( void ) const;
	void rate( float newRate );
	//@}

	///	@name Python Interface to the PyScalerPSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, size, size )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, rate, rate )
	//@}
private:
	ScalerPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyScalerPSA )


#ifdef CODE_INLINE
#include "scaler_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* particle_system_action.hpp */
