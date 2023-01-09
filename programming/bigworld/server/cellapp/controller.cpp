#include "script/first_include.hpp"

#include "controller.hpp"

#include "entity.hpp"
#include "real_entity.hpp"
#include "cell.hpp"
#include "cellapp.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Controller
// -----------------------------------------------------------------------------

/**
 * 	Constructor.
 * 	Just initialises the controller to a sane state. The entity and controller
 * 	id will be initialised by the entity later on, when this controller is
 * 	actually added to an entity.
 */
Controller::Controller() :
	pEntity_( NULL ),
	userArg_( 0 ),
	controllerID_( 0 )
{
}


/**
 * 	Destructor
 */
Controller::~Controller()
{
}


/**
 *	This method initialises this controller.
 */
void Controller::init( Entity * pEntity, ControllerID id, int userArg )
{
	pEntity_ = pEntity;
	controllerID_ = id;
	userArg_ = userArg;
}


/**
 *	This method clears the entity pointer when we have been disowned.
 */
void Controller::disowned()
{
	pEntity_ = NULL;
}


/**
 *	This method saves the state necessary to initialise this controller.
 *
 *	Overrides of this method should call their base implementation first.
 */
void Controller::writeRealToStream( BinaryOStream & /*stream*/ )
{
}


/**
 *	This method initialises this controller from the data in the input stream.
 *
 *	Overrides of this method should call their base implementation first.
 */
bool Controller::readRealFromStream( BinaryIStream & /*stream*/ )
{
	return true;
}


/**
 *	This method saves the state necessary to initialise this controller.
 *
 *	Overrides of this method should call their base implementation first.
 */
void Controller::writeGhostToStream( BinaryOStream & /*stream*/ )
{
}


/**
 *	This method initialises this controller from the data in the input stream.
 *
 *	Overrides of this method should call their base implementation first.
 */
bool Controller::readGhostFromStream( BinaryIStream & /*stream*/ )
{
	return true;
}


/**
 *	This method cancels this controller. As part of deleting it from the
 *	entity's list, it will be stopped in whatever domains it runs in.
 */
void Controller::cancel()
{
	if (pEntity_)
	{
		MF_ASSERT( pEntity_->isReal() );

		pEntity_->delController( controllerID_ );
	}
	else
	{
		CRITICAL_MSG( "Controller::cancel: "
				"Trying to cancel non-attached controller!!!!\n" );
	}
}


/**
 *	This method updates the ghost of this controller on all ghosts of the
 *	entity. It can only be called on controllers on real entities.
 */
void Controller::ghost()
{
	pEntity_->modController( this );
}


/**
 * 	This method calls a named callback on the script object with two arguments:
 * 	the id of the controller, and the user data.
 *
 * 	@param methodName		Name of the method to call
 */
void Controller::standardCallback( const char * methodName )
{
	MF_ASSERT( this->isAttached() );

	START_PROFILE( SCRIPT_CALL_PROFILE );

	EntityID entityID = this->entity().id();
	const char* name = this->entity().pType()->name();
	ControllerID controllerID = controllerID_;
	int userArg = userArg_;

	this->entity().callback( methodName,
		Py_BuildValue( "(ii)", controllerID, userArg ),
		methodName, false );
	// note: controller (and even entity) could be have been deleted by here

	STOP_PROFILE_WITH_CHECK( SCRIPT_CALL_PROFILE )
	{
		WARNING_MSG( "Controller::standardCallback: "
			"method = %s; type = %s, id = %u; controllerID = %d; "
				"userArg = %d\n",
			methodName, name, entityID, controllerID, userArg );
	}
}


// -----------------------------------------------------------------------------
// Section: Overridables
// -----------------------------------------------------------------------------

/**
 *	This virtual method is called when this controller is started on a real
 *	entity. It can be overridden by derived controllers to perform tasks when
 *	the controller is started initially or when the controller has been
 *	offloaded with an offloaded entity.
 *
 *	The controller must be in DOMAIN_REAL for this to be called.
 *
 *	@param isInitialStart	True if this is the first time this controller has
 *							been started, false if from an offload (or a
 *							restore).
 */
void Controller::startReal( bool /*isInitialStart*/ )
{
}


/**
 *	This virtual method is called when this controller is started on a real
 *	entity. It can be overridden by derived controllers to perform tasks when
 *	the controller is stopped for the last time or when the controller is being
 *	offloaded with an offloaded entity.
 *
 *	The controller must be in DOMAIN_REAL for this to be called.
 *
 *	Nothing must be done in these methods which could cause a controller to
 *	be created.
 *
 *	@param isFinalStop	True if this is the final time this controller will be
 *						stopped, false if caused by an offload.
 */
void Controller::stopReal( bool /*isFinalStop*/ )
{
}


/**
 *	This virtual method is called when this controller is started on a real or
 *	ghost entity.
 *
 *	The controller must be DOMAIN_GHOST for this to be called. If the controller
 *	is in both domains, it is called before startReal when first created.
 */
void Controller::startGhost()
{
}


/**
 *	This virtual method is called when this controller is started on a real or
 *	ghost entity.
 *
 *	The controller must be DOMAIN_GHOST for this to be called. If the controller
 *	is in both domains, it is called after stopReal when finally stopped.
 */
void Controller::stopGhost()
{
}


// -----------------------------------------------------------------------------
// Section: ControllerFactoryCaller
// -----------------------------------------------------------------------------

// static initialiser
Entity * Controller::s_factoryFnEntity_ = NULL;

/**
 *	This class is the Python object for a controller factory method
 */
class ControllerFactoryCaller : public PyObjectPlus
{
	Py_Header( ControllerFactoryCaller, PyObjectPlus )

public:
	ControllerFactoryCaller( Entity * pEntity, Controller::FactoryFn factory,
			PyTypeObject * pType = &s_type_ ) :
		PyObjectPlus( pType ),
		pEntity_( pEntity ),
		factory_( factory )
	{ }

	PY_KEYWORD_METHOD_DECLARE( pyCall )

private:
	EntityPtr				pEntity_;
	Controller::FactoryFn	factory_;
};


/**
 *	Call method for a controller factory method
 */
PyObject * ControllerFactoryCaller::pyCall( PyObject * args, PyObject * kwargs )
{
	// make sure this entity will accept controllers
	if (pEntity_->isDestroyed())
	{
		PyErr_SetString( PyExc_TypeError,
			"Entity for Controller factory no longer exists" );
		return NULL;
	}

	if (!pEntity_->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"Entity for Controller factory is not real" );
		return NULL;
	}

	Entity * oldFFnE = Controller::s_factoryFnEntity_;
	Controller::s_factoryFnEntity_ = pEntity_.get();

	// attempt to create the controller with these arguments then
	Controller::FactoryFnRet factoryFnRet = (*factory_)( args, kwargs );

	Controller::s_factoryFnEntity_ = oldFFnE;

	if (factoryFnRet.pController == NULL) return NULL;

	// ok, we have a controller, so let the entity add it,
	// and return its controller id to python
	return Script::getData( pEntity_->addController(
		factoryFnRet.pController, factoryFnRet.userArg ) );
}

PY_TYPEOBJECT_WITH_CALL( ControllerFactoryCaller )

PY_BEGIN_METHODS( ControllerFactoryCaller )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ControllerFactoryCaller )
PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: Controller registration
// -----------------------------------------------------------------------------

/**
 *	Registration info
 */
struct ControllerTypeInfo
{
	ControllerTypeInfo( const char * typeName, Controller::CreatorFn creator,
			ControllerType * pTypeNum, Controller::FactoryFn factory ) :
		typeName_( typeName ),
		creator_( creator ),
		pTypeNum_( pTypeNum ),
		factory_( factory )
	{ }

	const char * typeName_;
	Controller::CreatorFn creator_;
	ControllerType * pTypeNum_;
	//ControllerDomain domain_;
	Controller::FactoryFn factory_;
};

typedef BW::vector< ControllerTypeInfo > Creators;
static Creators * s_pCreators = NULL;


/**
 *	This static method creates a new controller given a type.
 *
 *	@param type		Type of controller to create.
 *
 *	@return			Pointer to new controller.
 */
ControllerPtr Controller::create( ControllerType type )
{
	if (s_pCreators == NULL || type >= s_pCreators->size())
	{
		CRITICAL_MSG( "Controller::create: "
			"Asked to create unknown type %d (max is %" PRIzu ")\n",
			type, s_pCreators != NULL ? s_pCreators->size() : 0 );
		return NULL;
	}

	return (*(*s_pCreators)[ type ].creator_)();
}

const char * Controller::typeName() const
{
	ControllerType type = this->type();

	if (s_pCreators == NULL || type >= s_pCreators->size())
	{
		CRITICAL_MSG( "Controller::typeName: Invalid type\n" );
		return "";
	}

	return (*s_pCreators)[ type ].typeName_;
}


/**
 *	This static method returns a python object representing the factory
 *	method for the given named controller, if such a controller exists.
 */
PyObject * Controller::factory( Entity * pEntity, const char * name )
{
	if (s_pCreators == NULL) return NULL;

	uint len = strlen(name);
	if (len > 200)
	{
		return NULL;
	}

	char fullName[256];
	memcpy( fullName, name, len );
	memcpy( fullName+len, "Controller", 11 );

	// for now do a linear search ... could use a map but I can't be bothered
	for (uint i = 0; i < s_pCreators->size(); i++)
	{
		if ((*s_pCreators)[ i ].factory_ != NULL &&
			strcmp( (*s_pCreators)[ i ].typeName_, fullName ) == 0)
		{
			return new ControllerFactoryCaller(
				pEntity, (*s_pCreators)[ i ].factory_ );
		}
	}
	return NULL;
}


/**
 *	This static method adds the controller factory method names to the given
 *	tuple, and returns the number it added.
 */
ControllerType Controller::factories( 
		const ScriptList & pList, const char * prefix )
{
	if (s_pCreators == NULL)
	{
		return 0;
	}

	uint prefixLen = (prefix == NULL) ? 0 : strlen( prefix );
	char attr[256];
	memcpy( attr, prefix, prefixLen );

	uint nfact = 0;
	for (uint i = 0; i < s_pCreators->size(); i++)
	{
		// make sure it has a factory
		if ((*s_pCreators)[ i ].factory_ == NULL)
		{
			continue;
		}

		// make sure the name meets the requirements (ends in 'Controller')
		const char * fullName = (*s_pCreators)[ i ].typeName_;
		uint fullNameLen = strlen( fullName );
		if (fullNameLen <= 10 || fullNameLen > 200)
		{
			continue;
		}

		if (strcmp( fullName + fullNameLen-10, "Controller" ) != 0)
		{
			continue;
		}

		// now add it to the list
		memcpy( attr+prefixLen, fullName, fullNameLen-10 );
		attr[ prefixLen + fullNameLen-10 ] = '\0';
		pList.append( ScriptString::create( attr ) );

		// and increment the count
		nfact++;
	}

	return nfact;
}


/**
 *	This static method registers a controller type.
 *
 *	It keeps them in sorted order so type numbers are stable across
 *	builds (with the same types), e.g. builds on different platforms.
 */
void Controller::registerControllerType( CreatorFn cfn, const char * typeName,
	ControllerType & ct, FactoryFn ffn )
{
	if (s_pCreators == NULL)
	{
		s_pCreators = new Creators();
	}

	uint i;
	for (i = 0; i < s_pCreators->size(); i++)
	{
		int diff = strcmp( (*s_pCreators)[ i ].typeName_, typeName );

		if (diff == 0)
		{
			CRITICAL_MSG( "Controller::registerControllerType: "
						"Tried to register two controller types of type '%s'\n",
					typeName );
		}

		if (diff > 0)
		{
			break;
		}
	}

	s_pCreators->insert( s_pCreators->begin() + i,
		ControllerTypeInfo( typeName, cfn, &ct, ffn ) );
	for (i = 0; i < s_pCreators->size(); i++)
	{
		*(*s_pCreators)[i].pTypeNum_ = ControllerType( i );
	}
}


/**
 *	This method returns the ControllerID associated with an exclusive category
 *	of controllers.
 *	@param exclusiveClass The name of the exclusive category.
 *	@param createIfNecessary If true and the exclusiveClass does not yet exist,
 *		a new ID is allocated and stored.
 *	@return	A non-zero ControllerID on success, otherwise 0.
 */
ControllerID Controller::getExclusiveID(
		const char * exclusiveClass, bool createIfNecessary )
{
	ControllerID newID = 0;

	if (exclusiveClass)
	{
		typedef BW::map< BW::string, ControllerID > ExclusiveIDs;
		static ExclusiveIDs s_exclusiveIDs;

		ExclusiveIDs::iterator iter = s_exclusiveIDs.find( exclusiveClass );
		if (iter != s_exclusiveIDs.end())
		{
			newID = iter->second;
		}
		else if (createIfNecessary)
		{
			newID = Controller::EXCLUSIVE_CONTROLLER_ID_MASK +
					s_exclusiveIDs.size() + 1;
			s_exclusiveIDs[ exclusiveClass ] = newID;
		}
	}

	return newID;
}

BW_END_NAMESPACE

// controller.cpp
