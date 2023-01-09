#ifndef SOURCE_PSA_HPP
#define SOURCE_PSA_HPP

#include "particle_system_action.hpp"
#include "romp/romp_collider.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This action creates particles. The source PSA generally creates particles
 *	at a rate over time, particles per second.
 *
 *	The source PSA can also generate particles on demand, and if set to a
 *	particular node, can generate particles by movement. The activePeriod
 *	and sleepPeriod/sleepPeriodMax values allow periodic creation of particles
 *	over time.
 *
 *	All of these properties can be working simultaneously for a single
 *	source.
 *
 *	The grounded attribute, if set to true, will force particles to drop to
 *	the ground or scene from where they appear.
 */
class SourcePSA : public ParticleSystemAction
{
public:
	///	@name Constructor(s) and Destructor.
	//@{
	SourcePSA( VectorGenerator *pPositionSrc = NULL,
		VectorGenerator *pVelocitySrc = NULL );
	~SourcePSA();
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Accessors to SourcePSA properties.
	//@{
	bool motionTriggered( void ) const;
	void motionTriggered( bool flag );

	bool timeTriggered( void ) const;
	void timeTriggered( bool flag );

	RompColliderPtr groundSpecifier( void ) const;
	void groundSpecifier( RompColliderPtr pGS );

	bool grounded( void ) const;
	void grounded( bool flag );

	float dropDistance( void ) const;
	void dropDistance( float newHeight );

	float rate( void ) const;
	void rate( float amount );

	float sensitivity( void ) const;
	void sensitivity( float value );

	float activePeriod( void ) const;
	void activePeriod( float timeInSeconds );

	float sleepPeriod( void ) const;
	void sleepPeriod( float timeInSeconds );

	float sleepPeriodMax( void ) const;
	void sleepPeriodMax( float timeInSeconds );

	float minimumSize( void ) const;
	void minimumSize( float newScale );

	float maximumSize( void ) const;
	void maximumSize( float newScale );

	int forcedUnitSize( void ) const;
	void forcedUnitSize( int newUnitSize );

	float allowedTimeInSeconds( void ) const;
	void allowedTimeInSeconds( float timeInSeconds );

	Vector2 initialRotation( void ) const;
	void initialRotation( const Vector2& rotation);

	Vector2 randomInitialRotation( void ) const;
	void randomInitialRotation( const Vector2& rot );

	Vector4 initialColour( void ) const;
	void initialColour( Vector4 colour);

	bool randomSpin( void ) const;
	void randomSpin(bool enable);

	float minSpin( void ) const;
	void minSpin(float amount);

	float maxSpin( void ) const;
	void maxSpin(float amount);

	bool ignoreRotation( void ) const;
	void ignoreRotation( bool option);

    float inheritVelocity() const;
    void inheritVelocity(float amount);

    float maxSpeed() const;
    void maxSpeed(float amount);
	//@}
	virtual void setFirstUpdate()
	{
		ParticleSystemAction::setFirstUpdate();
		firstUpdate_ = true;
	}

	///	@name Methods for SourcePSA.
	//@{
	void create( int number );
	void force( int number );

	void setPositionSource( VectorGenerator *pPositionSrc );
	void setVelocitySource( VectorGenerator *pVelocitySrc );

	VectorGenerator * getPositionSource() { return pPositionSrc_; }
	VectorGenerator * getVelocitySource() { return pVelocitySrc_; }
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

	///	@name Helper Methods for the SourcePSA.
	//@{
	void createParticles( ParticleSystem & particleSystem,
		const Matrix & objectToWorld, int numberOfParticles,
		const Vector3 & dispPerParticle );

	float generateSleepPeriod() const;	///< Generates a random sleep period
	//@}

	///	@name Properties of the SourcePSA.
	//@{
	bool motionTriggered_;	///< Flag for motion triggered particle creation.
	bool timeTriggered_;	///< Flag for time triggered particle creation.
	bool grounded_;			///< Flag for dropping particles to ground.
	float rate_;			///< Rate of time triggered generation.
	float sensitivity_;		///< Rate of motion triggered generation.
	float maxSpeed_;		///< speed limit for time triggered particles.
	float activePeriod_;	///< Length of active period in seconds.
	float sleepPeriod_;		///< Length of sleep period in seconds.
	float sleepPeriodMax_;	///< Maximum length of sleep period in seconds.
	float minimumSize_;		///< Smallest scale for the generated particles.
	float maximumSize_;		///< Largest scale for the generated particles.
	int forcedUnitSize_;	///< Unit to be created per forced value.
	uint64 allowedTime_;	///< Allowed time per frame to create particles.

	VectorGenerator * pPositionSrc_;	///< Generator for position.
	VectorGenerator * pVelocitySrc_;	///< Generator for velocity.
	//@}

	///	@name Auxiliary Variables for the SourcePSA.
	//@{
	static int typeID_;			///< TypeID of the SourcePSA.

	int queuedTotal_;			///< Number of particles to create as demanded.
	int forcedTotal_;			///< Number of units to be force-created.
	float periodTime_;			///< Current position in the operating cycle.
	float accumulatedTime_;		///< Time since last time triggered particle.
	Vector3 lastPositionOfPS_;	///< Previous position of the particle system.
	Vector3 velocityOfPS_;		///< The calculated velocity of the particle system.
	bool firstUpdate_;			///< If true, need to ignore lastPositionOfPS.
	float dropDistance_;		///< Height-downwards for ground collision.
	RompColliderPtr pGS_;		///< Specifier for grounded sources.
	uint64 currentTime_;		///< Timestamp value for the current frame.
	Vector2 initialRotation_;	///< for mesh systems, initial pitch/yaw
	Vector2 randomInitialRotation_;	///< for mesh systems, initial pitch/yaw randomisation
	Vector4 initialColour_;		///< primarily for mesh systems, initial rotation
	bool randomSpin_;			///< primarily for mesh systems, randomise angular velocity
	float minSpin_;				///< minimum random angular velocity, in MESH_MAX_SPIN radians/second
	float maxSpin_;				///< maximum random angular velocity, in MESH_MAX_SPIN radians/second
	bool ignoreRotation_;		///< strips out the rotation of the world transform before ejecting particles
	float inheritVelocity_;		///< inherit how much velocity from particle system when spawning particles? (pct)
	float currentSleepPeriod_;	///< the current sleep period time
	//@}
};

typedef SmartPointer<SourcePSA> SourcePSAPtr;


/*~ class Pixie.PySourcePSA
 *
 *	PySourcePSA is a PyParticleSystemAction that creates particles within
 *	a ParticleSystem. Particles can be created at a rate over time, on demand
 *	or in response to movement (see the sensitivity attribute) by the node to
 *	which the particle system is attached.
 *
 *	A new PySourcePSA is created using Pixie.SourcePSA function.
 */
class PySourcePSA : public PyParticleSystemAction
{
	Py_Header( PySourcePSA, PyParticleSystemAction )
public:
	PySourcePSA( SourcePSAPtr pAction, PyTypeObject *pType = &s_type_ );

	SourcePSAPtr pAction();

	int typeID( void ) const;

	///	@name Accessors to PySourcePSA properties.
	//@{
	bool motionTriggered( void ) const;
	void motionTriggered( bool flag );

	bool timeTriggered( void ) const;
	void timeTriggered( bool flag );

	bool grounded( void ) const;
	void grounded( bool flag );

	float dropDistance( void ) const;
	void dropDistance( float newHeight );

	float rate( void ) const;
	void rate( float amount );

	float sensitivity( void ) const;
	void sensitivity( float value );

	float activePeriod( void ) const;
	void activePeriod( float timeInSeconds );

	float sleepPeriod( void ) const;
	void sleepPeriod( float timeInSeconds );

	float sleepPeriodMax( void ) const;
	void sleepPeriodMax( float timeInSeconds );

	float minimumSize( void ) const;
	void minimumSize( float newScale );

	float maximumSize( void ) const;
	void maximumSize( float newScale );

	int forcedUnitSize( void ) const;
	void forcedUnitSize( int newUnitSize );

	float allowedTime( void ) const;
	void allowedTime( float timeInSeconds );

	Vector2 initialRotation( void ) const;
	void initialRotation( const Vector2& rotation);

	Vector2 randomInitialRotation( void ) const;
	void randomInitialRotation( const Vector2& rot );

	Vector4 initialColour( void ) const;
	void initialColour( Vector4 colour);

	bool randomSpin( void ) const;
	void randomSpin(bool enable);

	float minSpin( void ) const;
	void minSpin(float amount);

	float maxSpin( void ) const;
	void maxSpin(float amount);

	bool ignoreRotation( void ) const;
	void ignoreRotation( bool option);

    float inheritVelocity() const;
    void inheritVelocity(float amount);

    float maxSpeed() const;
    void maxSpeed(float amount);
	//@}

	///	@name Python Interface to the PySourcePSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_create )
	PY_METHOD_DECLARE( py_force )
	PY_METHOD_DECLARE( py_setPositionSource )
	PY_METHOD_DECLARE( py_setVelocitySource )
	PY_METHOD_DECLARE( py_getPositionSourceMaxRadius )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, motionTriggered, motionTriggered )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, timeTriggered, timeTriggered )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, grounded, grounded )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, dropDistance, dropDistance )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, rate, rate )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, sensitivity, sensitivity )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, activePeriod, activePeriod )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, sleepPeriod, sleepPeriod )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, sleepPeriodMax, sleepPeriodMax )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minimumSize, minimumSize )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maximumSize, maximumSize )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, forcedUnitSize, forcedUnitSize )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, allowedTime, allowedTime )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector2, initialRotation, initialRotation )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector2, randomInitialRotation, randomInitialRotation )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector4, initialColour, initialColour )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, randomSpin, randomSpin )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minSpin, minSpin )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxSpin, maxSpin )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, ignoreRotation, ignoreRotation )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, inheritVelocity, inheritVelocity )
	//@}
private:
	SourcePSAPtr pAction_;		
};

typedef SmartPointer<PySourcePSA> PySourcePSAPtr;

PY_SCRIPT_CONVERTERS_DECLARE( PySourcePSA )

#ifdef CODE_INLINE
#include "source_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* source_psa.hpp */
