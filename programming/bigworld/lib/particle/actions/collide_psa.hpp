#ifndef COLLIDE_PSA_HPP
#define COLLIDE_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

// Forward Class Declarations.
class ParticleSystem;

/**
 *	This class collides particles with the collision scene ( erk.. )
 */
class CollidePSA : public ParticleSystemAction
{
public:
	CollidePSA( float elasticity = 0.0f );

    ParticleSystemActionPtr clone() const;

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem & particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(CollidePSA); }
	//@}

	///	@name Accessors to CollidePSA properties.
	//@{
	bool spriteBased() const;
	void spriteBased( bool s );

	float elasticity() const;
	void elasticity( float e );

	float minAddedRotation() const;
	void minAddedRotation( float e );

	float maxAddedRotation() const;
	void maxAddedRotation( float e );

	bool soundEnabled() const;
	void soundEnabled( bool e );

	const BW::string& soundTag() const;
	void soundTag( const BW::string& tag );

	int soundSrcIdx() const;
	void soundSrcIdx( int i );

	const BW::string& soundProject() const;
	void soundProject( const BW::string& project );

	const BW::string& soundGroup() const;
	void soundGroup( const BW::string& group );

	const BW::string& soundName() const;
	void soundName( const BW::string& sound );
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

	void updateSoundTag();

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Auxiliary Variables for the CollidePSA.
	//@{
	static int typeID_;			///< TypeID of the CollidePSA.
	bool spriteBased_;
	float elasticity_;
	float minAddedRotation_;
	float maxAddedRotation_;
	BW::string soundTag_;
	bool soundEnabled_;
	int soundSrcIdx_;

	BW::string soundProject_;
	BW::string soundGroup_;
	BW::string soundName_;
	//@}
};

typedef SmartPointer<CollidePSA> CollidePSAPtr;


/*~ class Pixie.PyCollidePSA
 *
 *	PyCollidePSA is a PyParticleSystemAction that collides particles with the
 *	collision scene, based upon their position and velocity.
 *
 *	A new PyCollidePSA is created using Pixie.CollidePSA function.
 */
class PyCollidePSA : public PyParticleSystemAction
{
	Py_Header( PyCollidePSA, PyParticleSystemAction )
public:
	PyCollidePSA( CollidePSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	bool spriteBased() const;
	void spriteBased( bool s );

	float elasticity() const;
	void elasticity( float e );

	float minAddedRotation() const;
	void minAddedRotation( float e );

	float maxAddedRotation() const;
	void maxAddedRotation( float e );

	bool soundEnabled() const;
	void soundEnabled( bool e );

	const BW::string& soundTag() const;
	void soundTag( const BW::string& tag );

	int soundSrcIdx() const;
	void soundSrcIdx( int i );

	const BW::string& soundProject() const;
	void soundProject( const BW::string& bank );

	const BW::string& soundGroup() const;
	void soundGroup( const BW::string& group );

	const BW::string& soundName() const;
	void soundName( const BW::string& sound );

	///	@name Python Interface to the PyCollidePSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, spriteBased, spriteBased );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, elasticity, elasticity );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minAddedRotation, minAddedRotation );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxAddedRotation, maxAddedRotation );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, soundTag, soundTag );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, soundSrcIdx, soundSrcIdx );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, soundEnabled, soundEnabled );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, soundProject, soundProject );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, soundGroup, soundGroup );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, soundName, soundName );
	//@}
private:
	CollidePSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyCollidePSA )


#ifdef CODE_INLINE
#include "collide_psa.ipp"
#endif

BW_END_NAMESPACE

#endif // COLLIDE_PSA_HPP
/* collide_psa.hpp */
