#ifndef JITTER_PSA_HPP
#define JITTER_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This action adds random movement to the particles. The jitter PSA can
 *	effect a random jitter to either position or velocity or both at once.
 *
 *	There is a vector generator specified for both effects.
 */
class JitterPSA : public ParticleSystemAction
{
public:
	///	@name Constructor(s) and Destructor.
	//@{
	JitterPSA( VectorGenerator *pPositionSrc = NULL,
		VectorGenerator *pVelocitySrc = NULL );
	~JitterPSA();
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Accessors to JitterPSA properties.
	//@{
	bool affectPosition( void ) const;
	void affectPosition( bool flag );

	bool affectVelocity( void ) const;
	void affectVelocity( bool flag );
	//@}

	///	@name Methods for JitterPSA.
	//@{
	void setPositionSource( VectorGenerator *pPositionSrc );
	VectorGenerator * getPositionSource() { return pPositionSrc_; }
	void setVelocitySource( VectorGenerator *pVelocitySrc );
	VectorGenerator * getVelocitySource() { return pVelocitySrc_; }
	//@}

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem &particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const;
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Properties of the JitterPSA.
	//@{
	bool affectPosition_;	///< Flag for jittering particle position.
	bool affectVelocity_;	///< Flag for jittering particle velocity.

	VectorGenerator * pPositionSrc_;	///< Jitter Generator for position.
	VectorGenerator * pVelocitySrc_;	///< Jitter Generator for velocity.
	//@}

	///	@name Auxiliary Variables for the JitterPSA.
	//@{
	static int typeID_;			///< TypeID of the JitterPSA.
	//@}
};

typedef SmartPointer<JitterPSA> JitterPSAPtr;


/*~ class Pixie.PyJitterPSA
 *
 *	PyJitterPSA is a PyParticleSystemAction that adds random motion to the
 *	particles. JitterPSA can affect the position and/or veclocity of the
 *	particles.
 *
 *	The position source and/or velocity source will need to be set before this
 *	ParticleSystemAction will produce a visible effect - both default to
 *	( 0, 0, 0 ).
 *
 *	Position and Velocity "jitter" are calculated each frame.
 *
 *	A new PyJitterPSA is created using Pixie.JitterPSA function.
 */
class PyJitterPSA : public PyParticleSystemAction
{
	Py_Header( PyJitterPSA, PyParticleSystemAction )
public:
	PyJitterPSA( JitterPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Accessors to PyJitterPSA properties.
	//@{
	bool affectPosition( void ) const;
	void affectPosition( bool flag );

	bool affectVelocity( void ) const;
	void affectVelocity( bool flag );
	//@}

	///	@name Python Interface to the PyJitterPSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_setPositionSource )
	PY_METHOD_DECLARE( py_setVelocitySource )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, affectPosition, affectPosition )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, affectVelocity, affectVelocity )
	//@}
private:
	JitterPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyJitterPSA )

#ifdef CODE_INLINE
#include "jitter_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* jitter_psa.hpp */
