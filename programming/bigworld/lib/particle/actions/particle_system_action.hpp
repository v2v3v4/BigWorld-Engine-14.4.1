#ifndef PARTICLE_SYSTEM_ACTION_HPP
#define PARTICLE_SYSTEM_ACTION_HPP

// Standard MF Library Headers.
#include "moo/moo_math.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/stl_to_py.hpp"

// Standard Library Headers.
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/static_array.hpp"


BW_BEGIN_NAMESPACE

// Forward Class Declarations.
class ParticleSystem;
class ParticleSystemAction;
class VectorGenerator;

enum ParticleSystemActionType 
{
	PSA_SOURCE_TYPE_ID			=  1,
	PSA_SINK_TYPE_ID			=  2,
	PSA_BARRIER_TYPE_ID			=  3,
	PSA_FORCE_TYPE_ID			=  4,
	PSA_STREAM_TYPE_ID			=  5,
	PSA_JITTER_TYPE_ID			=  6,
	PSA_SCALAR_TYPE_ID			=  7,
	PSA_TINT_SHADER_TYPE_ID		=  8,
	PSA_NODE_CLAMP_TYPE_ID		=  9,
	PSA_ORBITOR_TYPE_ID			= 10,
	PSA_FLARE_TYPE_ID			= 11,
	PSA_COLLIDE_TYPE_ID			= 12,
	PSA_MATRIX_SWARM_TYPE_ID	= 13,
	PSA_MAGNET_TYPE_ID			= 14,
	PSA_SPLAT_TYPE_ID			= 15,
	PSA_TYPE_ID_MAX				= 16
};


/**
 *	Class for updating actions which require positions of nodes or
 *	MatrixProviders which must be updated first.
 */
struct UpdateLaterAction
{
	UpdateLaterAction() :
		action_( NULL ),
		particleSystem_( NULL ),
		dTime_( 0.0f )
	{}
	UpdateLaterAction( ParticleSystemAction& action,
		ParticleSystem& particleSystem,
		float dTime ) :
		action_( &action ),
		particleSystem_( &particleSystem ),
		dTime_( dTime )
	{}
	UpdateLaterAction( const UpdateLaterAction& other ) :
		action_( other.action_ ),
		particleSystem_( other.particleSystem_ ),
		dTime_( other.dTime_ )
	{}
	UpdateLaterAction& operator=( const UpdateLaterAction& other )
	{
		action_ = other.action_;
		particleSystem_ = other.particleSystem_;
		dTime_ = other.dTime_;
		return (*this);
	}
	ParticleSystemAction* action_;
	ParticleSystem* particleSystem_;
	float dTime_;
};


/// Adjust size as necessary
const size_t MAX_PARTICLE_SYSTEM_ACTIONS = 4096;

typedef SmartPointer<ParticleSystemAction> ParticleSystemActionPtr;
/**
 *	This is the base class defining the interface shared by all particle
 *	system actions.
 *
 *	All actions have a delay attribute. The action will not perform its
 *	actions until after the specified time as passed. There is also a
 *	minimum age attribute. The action will affect only the particles that
 *	are over that minimum age.
 */
class ParticleSystemAction : public ReferenceCount
{
public:
	static void pushLateUpdate( ParticleSystemAction& action,
		ParticleSystem& particleSystem,
		float dTime );
	static void flushLateUpdates();
	static void removeLateUpdates( const ParticleSystemAction* action );
	static void removeLateUpdates( const ParticleSystem* system );
	static void clearLateUpdates();

	///	@name Constructor(s) and Destructor.
	//@{
	ParticleSystemAction();
	virtual ~ParticleSystemAction();
	//@}

    virtual ParticleSystemActionPtr clone() const = 0;

	void load( DataSectionPtr pSect );
	void save( DataSectionPtr pSect ) const;

	static int nameToType( const BW::string & name );
	static const BW::string & typeToName( int type );
	static ParticleSystemActionPtr createActionOfType( int type );

	// Return the resources required for a ParticleSystemAction
	static void prerequisitesOfType( DataSectionPtr pSection,
		BW::set< BW::string > & output );
	static void prerequisites( DataSectionPtr pSection,
		BW::set< BW::string > & output ) {};

	virtual size_t sizeInBytes() const = 0;

	///	@name Interface Accessors for ParticleSystemAction.
	//@{
	float delay() const;
	void delay( float seconds );

	float minimumAge() const;
	void minimumAge( float seconds );

    const BW::string & name() const;
    void name(const BW::string & str); 

	virtual void setFirstUpdate() { age_ = 0; }

	bool enabled() const;
    void enabled( bool state );
	//@}

	///	@name Interface Methods to the ParticleSystemAction.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime ) = 0;
	virtual void lateExecute( ParticleSystem & particleSystem, float dTime ) {}
	virtual int typeID() const = 0;
	virtual const BW::string & nameID() const = 0;
	//@}

protected:
	virtual void loadInternal( DataSectionPtr pSect ) = 0;
	virtual void saveInternal( DataSectionPtr pSect ) const = 0;
	void clone( ParticleSystemAction * psa ) const;

private:
	typedef BW::DynamicEmbeddedArrayWithWarning< UpdateLaterAction,
		MAX_PARTICLE_SYSTEM_ACTIONS > UpdateLaterQueue;
	static UpdateLaterQueue s_updateLaterQueue_;

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

protected:

	float delay_;		///< Minimum time in seconds before activation.
	float minimumAge_;	///< Minimum age in seconds for an acted particle.
	float age_;			///< Elapsed time in seconds before activation.
	bool  enabled_;		///< Whether or not the action should be executed.
    BW::string name_;	///< Name, only used in particle editor
};


PY_SCRIPT_CONVERTERS_DECLARE( ParticleSystemAction )


class PyParticleSystemAction;
typedef SmartPointer<PyParticleSystemAction> PyParticleSystemActionPtr;

/*~ class Pixie.PyParticleSystemAction
 *	PyParticleSystemAction is the base class of all actions that modify the
 *	behaviour of a particle system over time. All actions can have a delay
 *	associated with them (the time an action will wait before affecting the
 *	particle system) and minimumAge (the age a particle must be before it can be
 *	affected by the action).
 *
 *	Multiple PyParticleSystemAction's can be stacked onto the particle system
 *	to create more complex behaviour.
 *
 *	Each PyParticleSystemAction has a typeID integer attribute to represent its type.
 *	They are as follows:
 *	
 *	@{
 *		PSA_SOURCE_TYPE_ID			 1
 *		PSA_SINK_TYPE_ID			 2
 *		PSA_BARRIER_TYPE_ID			 3
 *		PSA_FORCE_TYPE_ID			 4
 *		PSA_STREAM_TYPE_ID			 5
 *		PSA_JITTER_TYPE_ID			 6
 *		PSA_SCALAR_TYPE_ID			 7
 *		PSA_TINT_SHADER_TYPE_ID		 8
 *		PSA_NODE_CLAMP_TYPE_ID		 9
 *		PSA_ORBITOR_TYPE_ID			10
 *		PSA_FLARE_TYPE_ID			11
 *		PSA_COLLIDE_TYPE_ID			12
 *		PSA_MATRIX_SWARM_TYPE_ID	13
 *		PSA_MAGNET_TYPE_ID			14
 *		PSA_SPLAT_TYPE_ID			15
 *	@}
 */
class PyParticleSystemAction : public PyObjectPlus
{
	Py_Header( PyParticleSystemAction, PyObjectPlus )
public:
	PyParticleSystemAction( ParticleSystemActionPtr pAction, PyTypeObject *pType = &s_type_ );
	virtual ~PyParticleSystemAction() {};

	ParticleSystemActionPtr pAction() { return pA_; }

	static PyParticleSystemActionPtr createPyAction(ParticleSystemAction * pAction);

	virtual int typeID( void ) const =0;

	float delay( void ) const;
	void delay( float seconds );

	float minimumAge( void ) const;
	void minimumAge( float seconds );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, delay, delay )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minimumAge, minimumAge )
	PY_RO_ATTRIBUTE_DECLARE( typeID(), typeID )

private:
	ParticleSystemActionPtr pA_;
};


typedef SmartPointer<PyParticleSystemAction> PyParticleSystemActionPtr;


PY_SCRIPT_CONVERTERS_DECLARE( PyParticleSystemAction )


#ifdef CODE_INLINE
#include "particle_system_action.ipp"
#endif

BW_END_NAMESPACE

#endif
/* particle_system_action.hpp */
