#ifndef MAGNET_PSA_HPP
#define MAGNET_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This action acts as a magnet to the particles. The magnet PSA is the
 *	like a black hole, it applies an acceleration to
 *	the particles towards a particular point, based on distanceSq
 */
class MagnetPSA : public ParticleSystemAction
{
public:
	///	@name Constructor(s) and Destructor.
	//@{
	MagnetPSA( float s = 1.f );
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem &particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(MagnetPSA); }
	//@}

	/// Accessors to MagnetPSA properties.
	//@{
	void strength( float s );
	float strength() const;

	void minDist( float s );
	float minDist() const;

	void source( MatrixProviderPtr s );
	MatrixProviderPtr source() const;
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Auxiliary Variables for the MagnetPSA.
	//@{
	static int typeID_;		///< TypeID of the MagnetPSA.

	float strength_;		///< The strength of the magnet.
	float minDist_;			///< To avoid singularities in the magnetic field.
	MatrixProviderPtr source_;///< The origin of the magnetic field.
	//@}
};

typedef SmartPointer<MagnetPSA> MagnetPSAPtr;


/*~ class Pixie.PyMagnetPSA
 *
 *	The PyMagnetPSA PyParticleSystemAction sets up an acceleration on particles
 *	towards a particular point (magnitude based upon the square of the distance
 *	from that point).
 *
 *	A new PyMagnetPSA is created using Pixie.MagnetPSA function.
 *
 */
class PyMagnetPSA : public PyParticleSystemAction
{
	Py_Header( PyMagnetPSA, PyParticleSystemAction )
public:
	PyMagnetPSA( MagnetPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	/// Accessors to PyMagnetPSA properties.
	//@{
	void strength( float s );
	float strength() const;

	void minDist( float s );
	float minDist() const;

	void source( MatrixProviderPtr s );
	MatrixProviderPtr source() const;
	//@}

	///	@name Python Interface to the PyCollidePSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, strength, strength )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( MatrixProviderPtr, source, source )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minDist, minDist )
	//@}
private:
	MagnetPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyMagnetPSA )


#ifdef CODE_INLINE
#include "magnet_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* magnet_psa.hpp */
