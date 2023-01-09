#ifndef SPLAT_PSA_HPP
#define SPLAT_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

/**
 * This class calls the given callback class when a particle collides with the collision scene.
 */
class SplatPSA : public ParticleSystemAction
{
public:
	SplatPSA();

    ParticleSystemActionPtr clone() const;

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem &particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(SplatPSA); }
	//@}

	///	@name Accessors to SplatPSA properties.
	//@{
	SmartPointer<PyObject>	callback() const;
	void callback( SmartPointer<PyObject> c );
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	///	@name Auxiliary Variables for the SplatPSA.
	//@{
	static int typeID_;			///< TypeID of the SplatPSA.
	SmartPointer<PyObject> callback_;
	//@}
};

typedef SmartPointer<SplatPSA> SplatPSAPtr;


/*~ class Pixie.PySplatPSA
 *
 *	PySplatPSA is a PyParticleSystemAction that collides particles with the
 *	collision scene, based upon their position and velocity.  When a particle
 *	collides, it dies and calls the given callback class, which must have the
 *	following method callback.onSplat( self, (position3),(velocity3),(colour4) )
 *
 *	Note - this is a very expensive particle action, and should be used
 *	sparingly! Also note, the current implementation limits the action to
 *	0.5msec per frame - after that, particles will simply fall through geometry.
 *
 *	A new PySplatPSA is created using Pixie.SplatPSA function.
 */
class PySplatPSA : public PyParticleSystemAction
{
	Py_Header( PySplatPSA, PyParticleSystemAction )
public:
	PySplatPSA( SplatPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Accessors to PySplatPSA properties.
	//@{
	SmartPointer<PyObject>	callback() const;
	void callback( SmartPointer<PyObject> c );
	//@}

	///	@name Python Interface to the PySplatPSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( SmartPointer<PyObject>, callback, callback )
	//@}
private:
	SplatPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PySplatPSA )


#ifdef CODE_INLINE
#include "splat_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* splat_psa.hpp */
