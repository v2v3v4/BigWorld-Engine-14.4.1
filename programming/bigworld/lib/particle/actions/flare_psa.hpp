#ifndef FLARE_PSA_HPP
#define FLARE_PSA_HPP

#include "particle_system_action.hpp"
#include "romp/lens_effect.hpp"
#include "..\particles.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class draws a lens flare on particle.
 *  Number of lens flares depend on flareStep_
 *  0 - draw only one flare on the youngest particles
 *  1 - draw lens flare on every particle
 *  2..n - draw lens flare on each n-th particle
 */

class FlarePSA : public ParticleSystemAction
{
public:
	FlarePSA( const BW::StringRef & flareName = "",
		int step = 0, int colour = 0, int size = 0 );

	static void prerequisites( DataSectionPtr pSection,
		BW::set< BW::string > & output );

    ParticleSystemActionPtr clone() const;

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem & particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(FlarePSA); }
	//@}

	///	@name Accessors to FlarePSA properties.
	//@{
	const BW::string & flareName() const;
	void flareName( const BW::StringRef & name );

	int flareStep() const;
	void flareStep( int step );

	bool colourize() const;
	void colourize( bool colour );

	bool useParticleSize() const;
	void useParticleSize( bool size );
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	void addFlareToParticle( ParticleSystem & particleSystem,
		const Matrix & worldTransform, Particles::iterator & pItr );

private:
	///	@name Auxiliary Variables for the FlarePSA.
	//@{
	static int typeID_;			///< TypeID of the FlarePSA.

	LensEffect le_;
	BW::string flareName_;
	void loadLe( void );		// perform the load of the lens effect

	int			flareStep_;
	bool		colourize_;
	bool		useParticleSize_;
	//@}
};

typedef SmartPointer<FlarePSA> FlarePSAPtr;


/*~ class Pixie.PyFlarePSA
 *	PyFlarePSA is a PyParticleSystemAction that draws a lens flare at
 *	the location of one or more particles (either the oldest particle, all
 *	particles or every n particles). The flare resource used to draw the flare
 *	is specified when the PyFlarePSA is created using the FlarePSA factory
 *	function in the Pixie module.
 */
class PyFlarePSA : public PyParticleSystemAction
{
	Py_Header( PyFlarePSA, PyParticleSystemAction )
public:
	PyFlarePSA( FlarePSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	const BW::string & flareName();
	void flareName( const BW::string & name );

	int flareStep();
	void flareStep( int step );

	bool colourize();
	void colourize( bool colour );

	bool useParticleSize();
	void useParticleSize( bool size );

	///	@name Python Interface to the PyCollidePSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, flareName, flareName );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, flareStep, flareStep );	//0 == leader only, 1 == all, 2..x == every X particles do a flare
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, colourize, colourize );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, useParticleSize, useParticleSize );
	//@}
private:
	FlarePSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyFlarePSA )


#ifdef CODE_INLINE
#include "flare_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* flare_psa.hpp */
