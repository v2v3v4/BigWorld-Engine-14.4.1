#ifndef TINT_SHADER_PSA_HPP
#define TINT_SHADER_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This action tints a particle according to its age. If given mulitple tints
 *	over time, it will blend the particle's tint over time to each of the
 *	specified R, G, B, and Alpha components.
 *
 *	A tint is supplied with an age time (in seconds.) 
 */
class TintShaderPSA : public ParticleSystemAction
{
public:
	///	@name Public Types for the TintShader.
	//@{
	typedef std::pair< float, Vector4 > Tint;
	typedef BW::vector< Tint > Tints;
	//@}

	///	@name Constructor(s) and Destructor.
	//@{
	TintShaderPSA();
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Operations on the TintShaderPSA.
	//@{
	void addTintAt( float time, const Vector4 & tint );
	void clearAllTints();
	//@}

	///	@name Accessors to TintShaderPSA properties.
	//@{
	bool repeat( void ) const;
	void repeat( bool flag );

	Vector4ProviderPtr modulator( void ) const;
	void modulator( Vector4ProviderPtr vector );

	float period() const;
	void period( float p );

	float fogAmount( void ) const;
	void fogAmount( float f );

	Tints & tintSet() { return tints_; }
	//@}

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
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

	///	@name Auxiliary Variables for the TintShaderPSA.
	//@{
	static int typeID_;			///< TypeID of the TintShaderPSA.

	Tints tints_;				///< The map of Age to Tints.
	bool repeat_;				///< Repeat cycle of Tints.
	float period_;				///< Length of tint cycle.
	float fogAmount_;			///< Amount of fog to blend in.
	Vector4ProviderPtr	modulator_;	///< Global multiplier for the tints.
	//@}
};

typedef SmartPointer<TintShaderPSA> TintShaderPSAPtr;


/*~ class Pixie.PyTintShaderPSA
 *
 *	PyTintShaderPSA is a PyParticleSystemAction that tints a particle according
 *	to its age. The initial colour of a particle is ( 0.5, 0.5, 0.5, 1.0 ) in
 *	RGBA at time 0 and tinting occurs linearly from one tint to the next tint
 *	over the time interval between which they are set. Tints can be set at
 *	particular times using the addTintAt function.
 *
 *	A new PyTintShaderPSA is created using Pixie.TintShaderPSA function.
 */
class PyTintShaderPSA : public PyParticleSystemAction
{
	Py_Header( PyTintShaderPSA, PyParticleSystemAction )
public:
	PyTintShaderPSA( TintShaderPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Accessors to PyTintShaderPSA properties.
	//@{
	bool repeat( void ) const;
	void repeat( bool flag );

	float period( void ) const;
	void period( float p );

	float fogAmount( void ) const;
	void fogAmount( float f );

	Vector4ProviderPtr modulator( void ) const;
	void modulator( Vector4ProviderPtr vector );
	//@}

	///	@name Python Interface to the PyStreamPSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_addTintAt )
	PY_METHOD_DECLARE( py_clearAllTints )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, repeat, repeat )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, period, period )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, fogAmount, fogAmount )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector4ProviderPtr, modulator, modulator )
	//@}
private:
	TintShaderPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyTintShaderPSA )


#ifdef CODE_INLINE
#include "tint_shader_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* tint_shader_psa.hpp */
