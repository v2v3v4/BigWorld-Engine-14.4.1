#ifndef STREAM_PSA_HPP
#define STREAM_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This action acts as a stream that pushes particles along a particular
 *	velocity. It is different from the  force PSA because it does not
 *	apply an acceleration to the particle velocity. Instead, it converges
 *	the velocity to that of the stream. This is similar to the effect of
 *	wind or a stream of water on a leaf particle.
 *
 *	Drag can be simulated by setting the vector to the zero vector.
 */
class StreamPSA : public ParticleSystemAction
{
public:
	///	@name Constructor(s) and Destructor.
	//@{
	StreamPSA( float x = 0.0f, float y = 0.0f, float z = 0.0f,
		float newHalfLife = -1.0f );
	StreamPSA( const Vector3 &newVector, float newHalfLife = -1.0f );	
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Accessors for the StreamPSA.
	//@{
	const Vector3 &vector( void ) const;
	void vector( const Vector3 &newVector );

	float halfLife( void ) const;
	void halfLife( float newHalfLife );
	//@}

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(StreamPSA); }
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Auxiliary Variables for the StreamPSA.
	//@{
	static int typeID_;		///< TypeID of the StreamPSA.

	Vector3 vector_;		///< The vector describing the stream.
	float halfLife_;		///< Half-life to stream vector in seconds.
	//@}
};

typedef SmartPointer<StreamPSA> StreamPSAPtr;


/*~ class Pixie.PyStreamPSA
 *
 *	PyStreamPSA is a PyParticleSystemAction the converges the velocity of particles
 *	to the velocity of the stream.
 *
 *	Drag can be simulated by setting the velocity to the zero vector.
 *
 *	A new PyStreamPSA is created using Pixie.StreamPSA function.
 */
class PyStreamPSA : public PyParticleSystemAction
{
	Py_Header( PyStreamPSA, PyParticleSystemAction )
public:
	PyStreamPSA( StreamPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Accessors to PyStreamPSA properties.
	//@{
	const Vector3 &vector( void ) const;
	void vector( const Vector3 &newVector );

	float halfLife( void ) const;
	void halfLife( float newHalfLife );
	//@}

	///	@name Python Interface to the PyStreamPSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, vector, vector )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, halfLife, halfLife )
	//@}
private:
	StreamPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyStreamPSA )


#ifdef CODE_INLINE
#include "stream_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* stream_psa.hpp */
