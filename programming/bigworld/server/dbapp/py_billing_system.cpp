#include "script/first_include.hpp"

#include "py_billing_system.hpp"

#include "cstdmf/debug.hpp"
#include "connection/log_on_status.hpp"
#include "db_storage/db_entitydefs.hpp"
#include "pyscript/pyobject_plus.hpp"

DECLARE_DEBUG_COMPONENT2( "Billing", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyBillingSystem
// -----------------------------------------------------------------------------

class PyBillingResponse : public PyObjectPlus
{
	Py_Header( PyBillingResponse, PyObjectPlus )

public:
	PyBillingResponse( IGetEntityKeyForAccountHandler * pHandler,
		   const EntityDefs & entityDefs ) :
		PyObjectPlus( &s_type_ ),
		pHandler_( pHandler ),
		entityDefs_( entityDefs )
	{
	}

	~PyBillingResponse()
	{
		if (pHandler_)
		{
			ERROR_MSG( "PyBillingResponse::~PyBillingResponse: "
					"No response sent. Sending general failure\n" );

			if (!this->failureDBError( "No response from script" ))
			{
				PyErr_Clear();
			}
		}
	}

private:
	bool loadEntity( const BW::string & entityType, DatabaseID dbID,
			const BW::string & dataForClient,
			const BW::string & dataForBaseEntity );
	bool loadEntityByName( const BW::string & entityType,
			const BW::string & entityName,
			bool shouldCreateUnknown, bool shouldRememberUnknown,
			const BW::string & dataForClient,
			const BW::string & dataForBaseEntity );
	bool createNewEntity( const BW::string & entityType, bool shouldRemember,
			const BW::string & dataForClient,
			const BW::string & dataForBaseEntity );
	bool getEntityTypeID( const BW::string & entityType,
			EntityTypeID & resultID ) const;

	bool failureInvalidPassword( const BW::string & errorMsg );
	bool failureNoSuchUser( const BW::string & errorMsg );
	bool failureDBError( const BW::string & errorMsg );
	bool failure( int statusAsInt, const BW::string & errorMsg );
	bool sendFailure( LogOnStatus status, const BW::string & errorMsg );

	PY_AUTO_METHOD_DECLARE( RETOK, loadEntity,
					ARG( BW::string, ARG( DatabaseID,
					OPTARG( BW::string, BW::string(),
					OPTARG( BW::string, BW::string(), END ) ) ) ) )

	PY_AUTO_METHOD_DECLARE( RETOK, loadEntityByName,
			ARG( BW::string, ARG( BW::string, ARG( bool,
						OPTARG( bool, false,
						OPTARG( BW::string, BW::string(),
						OPTARG( BW::string, BW::string(), END ) ) ) ) ) ) )

	PY_AUTO_METHOD_DECLARE( RETOK, createNewEntity,
			ARG( BW::string, ARG( bool, OPTARG( BW::string, BW::string(),
			OPTARG( BW::string, BW::string(), END ) ) ) ) )

	PY_AUTO_METHOD_DECLARE( RETOK, failureInvalidPassword,
			OPTARG( BW::string, BW::string(), END ) )

	PY_AUTO_METHOD_DECLARE( RETOK, failureNoSuchUser,
			OPTARG( BW::string, BW::string(), END ) )

	PY_AUTO_METHOD_DECLARE( RETOK, failureDBError,
			OPTARG( BW::string, BW::string(), END ) )

	PY_AUTO_METHOD_DECLARE( RETOK, failure,
			ARG( int, OPTARG( BW::string, BW::string(), END ) ) )

	IGetEntityKeyForAccountHandler * pHandler_;
	const EntityDefs & entityDefs_;
};

/*~ class NoModule.PyBillingResponse
 *	@components{ db }
 *	An instance of PyBillingResponse is passed as an argument to
 *	getEntityKeyForAccount. One of its methods should be called to send a
 *	response that indicates how login should proceed.
 */
PY_TYPEOBJECT( PyBillingResponse )

PY_BEGIN_METHODS( PyBillingResponse )
	/*~ function PyBillingResponse loadEntity
	 *	@components{ db }
	 *	This method should be called on successful user authentication when
	 *	there is a specific entity to load.
	 *
	 *	@param entityType The type of the entity to load as a string.
	 *	@param entityID The database id of the entity to load.
	 *	@param dataForClient="" Optional string to send to the client.
	 *	@param dataForBaseEntity="" Optional string to send to the entity.
	 */
	PY_METHOD( loadEntity )

	/*~ function PyBillingResponse loadEntityByName
	 *	@components{ db }
	 *	This method can be called after successful user authentication to
	 *	request that an entity with a given identifier. This method is more
	 *	useful when authentication is actually done by a base entity.
	 *	PyBillingResponse.loadEntity is the more common response.
	 *
	 *	@param entityType The type of the entity to load as a string.
	 *	@param entityName The name of the entity to load as indicated by the
	 *		entity's Identifier property.
	 *	@param shouldCreateUnknown If true, an entity will be created even if
	 *		there is no match in the database.
	 *	@param shouldRememberUnknown If true, any unknown entity that is
	 *		created will be stored in the database.
	 *	@param dataForClient="" Optional string to send to the client.
	 *	@param dataForBaseEntity="" Optional string to send to the entity.
	 */
	PY_METHOD( loadEntityByName )

	/*~ function PyBillingResponse createNewEntity
	 *	@components{ db }
	 *	This method should be called on successful authentication and when a
	 *	brand new entity should be created instead of loading an existing
	 *	entity.
	 *
	 *	@param entityType The type of the entity to load as a string.
	 *	@param shouldRemember A boolean indicating whether the new entity
	 *		should be stored by the database and billing system.
	 *	@param dataForClient="" Optional string to send to the client.
	 *	@param dataForBaseEntity="" Optional string to send to the entity.
	 */
	PY_METHOD( createNewEntity )

	/*~ function PyBillingResponse failureInvalidPassword
	 *	@components{ db }
	 *	This method should be called when authentication fails due to an invalid
	 *	password.
	 *
	 *	@param errorMsg An optional string to pass to the client.
	 */
	PY_METHOD( failureInvalidPassword )

	/*~ function PyBillingResponse failureNoSuchUser
	 *	@components{ db }
	 *	This method should be called when authentication fails due to an invalid
	 *	username.
	 *
	 *	@param errorMsg An optional string to pass to the client.
	 */
	PY_METHOD( failureNoSuchUser )

	/*~ function PyBillingResponse failureDBError
	 *	@components{ db }
	 *	This method should be called when authentication fails due to a database
	 *	or scripting error.
	 *
	 *	@param errorMsg An optional string to pass to the client.
	 */
	PY_METHOD( failureDBError )

	/*~ function PyBillingResponse failure
	 *	@components{ db }
	 *	This method can be called to send an arbitrary error code back to the
	 *	client.
	 *
	 *	@param errorCode The error code to return to the client.
	 *	@param errorMsg An optional string to pass to the client.
	 */
	PY_METHOD( failure )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyBillingResponse )
PY_END_ATTRIBUTES()


bool PyBillingResponse::loadEntity( const BW::string & entityType,
		DatabaseID dbID,
		const BW::string & dataForClient,
		const BW::string & dataForBaseEntity )
{
	if (dbID == 0)
	{
		PyErr_SetString( PyExc_ValueError, "0 is an invalid database id" );
		return false;
	}

	EntityTypeID entityTypeID;

	if (!this->getEntityTypeID( entityType, entityTypeID ))
	{
		return false;
	}

	EntityKey entityKey( entityTypeID, dbID );

	pHandler_->onGetEntityKeyForAccountSuccess( entityKey, dataForClient,
		dataForBaseEntity );
	pHandler_ = NULL;

	return true;
}


bool PyBillingResponse::loadEntityByName( const BW::string & entityType,
		const BW::string & entityName,
		bool shouldCreateUnknown,
		bool shouldRememberUnknown,
		const BW::string & dataForClient,
		const BW::string & dataForBaseEntity )
{
	EntityTypeID entityTypeID;

	if (!this->getEntityTypeID( entityType, entityTypeID ))
	{
		return false;
	}

	pHandler_->onGetEntityKeyForAccountLoadFromUsername( entityTypeID,
			entityName, shouldCreateUnknown, shouldRememberUnknown,
			dataForClient, dataForBaseEntity );
	pHandler_ = NULL;

	return true;
}


bool PyBillingResponse::createNewEntity( const BW::string & entityType,
		bool shouldRemember, const BW::string & dataForClient,
		const BW::string & dataForBaseEntity )
{
	EntityTypeID entityTypeID;

	if (!this->getEntityTypeID( entityType, entityTypeID ))
	{
		return false;
	}

	pHandler_->onGetEntityKeyForAccountCreateNew( entityTypeID,
			shouldRemember, dataForClient, dataForBaseEntity );
	pHandler_ = NULL;

	return true;
}


bool PyBillingResponse::getEntityTypeID( const BW::string & entityType,
		EntityTypeID & resultID ) const
{
	if (!pHandler_)
	{
		PyErr_SetString( PyExc_ValueError, "Response already sent" );
		return false;
	}

	resultID = entityDefs_.getEntityType( entityType.c_str() );

	if (resultID == INVALID_ENTITY_TYPE_ID)
	{
		PyErr_Format( PyExc_ValueError, "Invalid entity type '%s'\n",
				entityType.c_str() );
		return false;
	}

	return true;
}


bool PyBillingResponse::failureInvalidPassword( const BW::string & errorMsg )
{
	return this->sendFailure(
			LogOnStatus::LOGIN_REJECTED_INVALID_PASSWORD, errorMsg );
}


bool PyBillingResponse::failureNoSuchUser( const BW::string & errorMsg )
{
	return this->sendFailure(
			LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER, errorMsg );
}


bool PyBillingResponse::failureDBError( const BW::string & errorMsg )
{
	return this->sendFailure(
			LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE, errorMsg );
}


bool PyBillingResponse::failure( int statusAsInt, const BW::string & errorMsg )
{
	LogOnStatus status( (LogOnStatus::Status) statusAsInt );

	if (status.okay() || (statusAsInt != status.value()))
	{
		PyErr_Format( PyExc_ValueError, "Invalid failure status %d",
				statusAsInt );
		return false;
	}

	return this->sendFailure( status, errorMsg );
}


/**
 *	This method sends a failure result.
 */
bool PyBillingResponse::sendFailure( LogOnStatus status,
		const BW::string & errorMsg )
{
	if (!pHandler_)
	{
		PyErr_SetString( PyExc_ValueError, "Response already sent" );
		return false;
	}

	pHandler_->onGetEntityKeyForAccountFailure( status, errorMsg );
	pHandler_ = NULL;

	return true;
}


// -----------------------------------------------------------------------------
// Section: PyBillingSystem
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PyBillingSystem::PyBillingSystem( PyObject * pPyObject,
		const EntityDefs & entityDefs ) :
	BillingSystem( entityDefs ),
	entityDefs_( entityDefs ),
	getEntityKeyFunc_( getMember( pPyObject, "getEntityKeyForAccount" ) ),
	setEntityKeyFunc_( getMember( pPyObject, "setEntityKeyForAccount" ) )
{
}


/**
 *	Destructor.
 */
PyBillingSystem::~PyBillingSystem()
{
}


/**
 *	This static method gets a member from the given object.
 */
PyObjectPtr PyBillingSystem::getMember( PyObject * pPyObject, const char * member )
{
	PyObjectPtr pMember( PyObject_GetAttrString( pPyObject,
							const_cast< char * >( member ) ),
						PyObjectPtr::STEAL_REFERENCE );

	if (!pMember)
	{
		ERROR_MSG( "PyBillingSystem::getMember: No such method %s\n",
				member );
		PyErr_Clear();
	}

	return pMember;
}


/**
 *	This method validates a login attempt and returns the details of the entity
 *	to use via the handler.
 */
void PyBillingSystem::getEntityKeyForAccount(
	const BW::string & username, const BW::string & password,
	const Mercury::Address & clientAddr,
	IGetEntityKeyForAccountHandler & handler )
{
	if (!getEntityKeyFunc_)
	{
		ERROR_MSG( "PyBillingSystem::getEntityKeyForAccount: "
				"No getEntityKeyForAccount method\n" );
		handler.onGetEntityKeyForAccountFailure(
				LogOnStatus::LOGIN_REJECTED_DB_GENERAL_FAILURE );
		return;
	}

	PyObjectPtr pUsername( Script::getData( username ),
			PyObjectPtr::STEAL_REFERENCE );

	PyObjectPtr pPassword( Script::getData( password ),
			PyObjectPtr::STEAL_REFERENCE );

	PyObjectPtr pClientAddr( Script::getData( clientAddr ),
			PyObjectPtr::STEAL_REFERENCE );

	PyObjectPtr pResponse( new PyBillingResponse( &handler, entityDefs_ ),
			PyObjectPtr::STEAL_REFERENCE );

	PyObjectPtr pArgs( PyTuple_Pack( 4,
						pUsername.get(), pPassword.get(),
						pClientAddr.get(), pResponse.get() ),
				   PyObjectPtr::STEAL_REFERENCE );
	PyObjectPtr pResult(
			PyObject_CallObject( getEntityKeyFunc_.get(), pArgs.get() ),
			PyObjectPtr::STEAL_REFERENCE );

	if (!pResult)
	{
		ERROR_MSG( "PyBillingSystem::getEntityKeyForAccount: "
				"Failed. username = %s\n", username.c_str() );
		PyErr_PrintEx( false );
		PyErr_Clear();
	}
}


/**
 *	This method is used to inform the billing system that a new entity has been
 *	associated with a login.
 */
void PyBillingSystem::setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler )
{
	if (!setEntityKeyFunc_)
	{
		ERROR_MSG( "PyBillingSystem::setEntityKeyForAccount: "
				"No setEntityKeyForAccount function exists. sendCreateNew "
				"should not be called with shouldRemember as True\n" );
		handler.onSetEntityKeyForAccountFailure( ekey );
		return;
	}

	const EntityDescription & entityDesc =
		entityDefs_.getEntityDescription( ekey.typeID );

	PyObjectPtr pArgs( PyTuple_New( 4 ),
			PyObjectPtr::STEAL_REFERENCE );

	PyTuple_SET_ITEM( pArgs.get(), 0,
						Script::getData( username ) );
	PyTuple_SET_ITEM( pArgs.get(), 1,
						Script::getData( password ) );
	PyTuple_SET_ITEM( pArgs.get(), 2,
						Script::getData( entityDesc.name() ) );
	PyTuple_SET_ITEM( pArgs.get(), 3,
						Script::getData( ekey.dbID ) );

	PyObjectPtr pResult(
			PyObject_CallObject( setEntityKeyFunc_.get(), pArgs.get() ),
			PyObjectPtr::STEAL_REFERENCE );

	if (!pResult)
	{
		ERROR_MSG( "PyBillingSystem::setLoginMapping: "
				"Failed. username = %s\n", username.c_str() );
		handler.onSetEntityKeyForAccountFailure( ekey );
		PyErr_Print();
	}
	else
	{
		handler.onSetEntityKeyForAccountSuccess( ekey );
	}
}

BW_END_NAMESPACE

// py_billing_system.cpp
