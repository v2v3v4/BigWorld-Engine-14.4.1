#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "Python.h"

#include "network/basictypes.hpp"
#include "cstdmf/binary_stream.hpp"
#include "cstdmf/smartpointer.hpp"
#include "script/script_object.hpp"



BW_BEGIN_NAMESPACE

typedef uint16 ControllerType;
typedef uint16 ControllerID;

class Entity;
class Controller;

typedef SmartPointer< Controller > ControllerPtr;

/**
 *	This enum determines the domain where a controller is active.
 *	If both are specified, then the streaming of real data happens
 *	on top of existing ghost controllers.
 */
enum ControllerDomain
{
	DOMAIN_GHOST = 1,
	DOMAIN_REAL = 2,
	DOMAIN_GHOST_AND_REAL = DOMAIN_GHOST | DOMAIN_REAL
};

/**
 * A controller is a server object that is owned by a real entity. It must
 * move between cells when the entity moves, and must be able to serialise its
 * state for this reason.
 */
class Controller : public ReferenceCount
{
public:
	static const ControllerID EXCLUSIVE_CONTROLLER_ID_MASK = 0x7fff;

	Controller();
	virtual ~Controller();

	/// Initialise this new controller
	void init( Entity * pEntity, ControllerID id, int userArg );
	/// Let this controlled know it has been disowned
	void disowned();

	/// Return our type, for serialisation only
	virtual ControllerType		type() const = 0;
	/// Return our domain
	virtual ControllerDomain	domain() const = 0;

	virtual ControllerID		exclusiveID() const = 0;

	const char *				typeName() const;

	/// Return whether or not we are attached to an entity
	bool					isAttached() const	{ return !!pEntity_; }

	/// Return the entity that owns us
	Entity&					entity() 			{ return *pEntity_; }

	/// Return our unique ID
	ControllerID			id() const			{ return controllerID_; }

	/// Return the user data for this controller
	int						userArg()			{ return userArg_; }

	/// Write our state to a buffer. (Don't stop running yet.)
	virtual void			writeRealToStream( BinaryOStream & stream );

	/// Read our state from a buffer. (Don't start running yet.)
	virtual bool			readRealFromStream( BinaryIStream & stream );

	virtual void			writeGhostToStream( BinaryOStream & stream );
	virtual bool			readGhostFromStream( BinaryIStream & stream );

private:
	// Only the Controllers class may call these functions

	virtual void			startReal( bool isInitialStart );
	virtual void			stopReal( bool isFinalStop );

	virtual void			startGhost();
	virtual void			stopGhost();


	friend class Controllers;

protected:
	/// Cancel this controller. This can be done by other Controllers
	void					cancel();

	/// Ghost the ghost part of this controller to all the haunts of its entity
	void					ghost();

	/// Send a standard callback to the script object.
	void					standardCallback( const char * methodName );

public:
	/// Create a controller given a type
	static ControllerPtr 	create( ControllerType type );
	//static ControllerDomain domain( ControllerType type );
	static PyObject *		factory( Entity * pEntity, const char * name );
	static ControllerType	factories( const ScriptList & pList, 
		const char * prefix = NULL );

	/// Controller creator function prototype
	typedef Controller * (*CreatorFn)();

	/// Return type of python factory function (sucks I know)
	struct FactoryFnRet
	{
		FactoryFnRet( void * = NULL ) :
			pController( NULL ), userArg( 0 ) { }
		FactoryFnRet( Controller * pC, int ua ) :
			pController( pC ), userArg( ua ) { }

		Controller * pController;
		int userArg;
	};
	/// Python factory function prototype
	typedef FactoryFnRet (*FactoryFn)( PyObject * args, PyObject * kwargs );

	/// Register a controller type
	static void				registerControllerType( CreatorFn cfn,
		const char * typeName, ControllerType & ct, FactoryFn ffn );

	static ControllerID getExclusiveID( const char * exclusiveClass,
				bool createIfNecessary );

	/**
	 *	Helper class for derived classes
	 */
	class TypeRegisterer
	{
	public:

		TypeRegisterer( CreatorFn cfn, const char * typeName,
				ControllerDomain cd, FactoryFn ffn,
				const char * exclusiveClass ) :
			type_( ControllerType( -1 ) ),
			domain_( cd )
		{
			Controller::registerControllerType( cfn, typeName, type_, ffn );
			exclusiveID_ = Controller::getExclusiveID( exclusiveClass, true );
		}

		ControllerType		type() const	{ return type_; }
		ControllerDomain	domain() const	{ return domain_; }
		ControllerID		exclusiveID() const	{ return exclusiveID_; }

	private:
		ControllerType		type_;
		ControllerDomain	domain_;
		ControllerID		exclusiveID_;
	};

	/// Entity that factory fn is being called for
	static Entity *			s_factoryFnEntity_;

private:
	Entity *				pEntity_;
	int						userArg_;
	ControllerID			controllerID_;
};



/// Put this macro as the first thing in a controller class declaration
#define DECLARE_CONTROLLER_TYPE( CLASS_NAME )								\
	static TypeRegisterer s_registerer_;									\
	static Controller * create()		{ return new CLASS_NAME(); }		\
protected:																	\
	virtual ControllerType type() const										\
							{ return s_registerer_.type(); }				\
	virtual ControllerDomain domain() const									\
							{ return s_registerer_.domain(); }				\
	virtual ControllerID exclusiveID() const								\
							{ return s_registerer_.exclusiveID(); }


/// Put one of these macros somewhere in your controller class implementation
#define IMPLEMENT_CONTROLLER_TYPE( CLASS_NAME, DOMAIN )						\
	Controller::TypeRegisterer CLASS_NAME::s_registerer_(					\
		&CLASS_NAME::create, #CLASS_NAME, DOMAIN, NULL, NULL );				\

#define IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE( CLASS_NAME, DOMAIN, CATEGORY )	\
	Controller::TypeRegisterer CLASS_NAME::s_registerer_(					\
		&CLASS_NAME::create, #CLASS_NAME, DOMAIN, NULL, CATEGORY );			\

#define IMPLEMENT_CONTROLLER_TYPE_WITH_PY_FACTORY( CLASS_NAME, DOMAIN )		\
	Controller::TypeRegisterer CLASS_NAME::s_registerer_(					\
		&CLASS_NAME::create, #CLASS_NAME, DOMAIN,							\
		&CLASS_NAME::controllerNew,											\
		NULL );																\

#define IMPLEMENT_EXCLUSIVE_CONTROLLER_TYPE_WITH_PY_FACTORY(				\
		CLASS_NAME, DOMAIN, CATEGORY )										\
	Controller::TypeRegisterer CLASS_NAME::s_registerer_(					\
		&CLASS_NAME::create, #CLASS_NAME, DOMAIN,							\
		&CLASS_NAME::controllerNew,											\
		CATEGORY );															\



/// Macro to define python factory method for a controller
#define PY_CONTROLLER_FACTORY_DECLARE()										\
	static FactoryFnRet controllerNew( PyObject * args, PyObject * kwargs );\

/// Macro to define auto parsing python factory method for a controller
#define PY_AUTO_CONTROLLER_FACTORY_DECLARE( CLASS_NAME, ARGS )				\
	static FactoryFnRet														\
			controllerNew( PyObject * args, PyObject * /*kwargs*/ )			\
	{																		\
		PY_AUTO_DEFINE_INT( RETOWN, CLASS_NAME, CLASS_NAME::New, ARGS )		\
	}																		\


BW_END_NAMESPACE

#endif // CONTROLLER_HPP
