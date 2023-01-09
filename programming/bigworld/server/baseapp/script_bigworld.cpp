#include "script/first_include.hpp"

#include "script_bigworld.hpp"

#include "base.hpp"
#include "baseapp.hpp"
#include "baseapp_config.hpp"
#include "entity_creator.hpp"
#include "mailbox.hpp"
#include "proxy.hpp"
#include "py_bases.hpp"
#include "py_replay_data_file_reader.hpp"
#include "py_replay_data_file_writer.hpp"
#include "service.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"

#include "cellappmgr/cellappmgr_interface.hpp"

#include "db/dbapp_interface.hpp"
#include "db/dbapp_interface_utils.hpp"

#include "connection/log_on_status.hpp"
#include "connection/replay_metadata.hpp"

#include "chunk/user_data_object.hpp"

#include "entitydef/constants.hpp"
#include "entitydef/py_deferred.hpp"

#include "network/compression_type.hpp"
#include "network/nub_exception.hpp"
#include "network/bundle.hpp"

#include "pyscript/keyword_parser.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "resmgr/resource_file_path.hpp"

#include "server/look_up_entities.hpp"
#include "server/shared_data.hpp"
#include "server/watcher_forwarding.hpp"


BW_BEGIN_NAMESPACE

extern int PyReplayDataFileReader_token;
extern int PyURLRequest_token;


namespace
{

int tokenSet = PyReplayDataFileReader_token | PyURLRequest_token;


// -----------------------------------------------------------------------------
// Section: Script related
// -----------------------------------------------------------------------------

/**
 *	This function implements the BigWorld.createBaseAnywhere script method.
 */
PyObject * py_createBaseAnywhere( PyObject * args, PyObject * kwargs )
{
	return BaseApp::instance().pEntityCreator()->
		createBaseAnywhere( args, kwargs );
}
/*~ function BigWorld.createBaseAnywhere
 *	@components{ base }
 *	The function createBaseAnywhere creates a new Base entity in the BigWorld
 *	server system. The server may choose to create the Base entity on any
 *	BaseApp.
 *
 *	This method should be preferred to BigWorld.createBaseLocally and
 *	BigWorld.createBaseRemotely as the server has the flexibility to create
 *	the entity on an appropriately loaded BaseApp.
 *
 *	The function takes the name of the entity type to be created and optionally
 *	a Python dictionary or keyword arguments to be used to provide initial
 *	values for the entity.
 *
 *	Any values not provided will default to the entity's default value provided
 *	by the ".def" file.
 *
 *	@param entityType Specifies the type of base entity to be	created. It must be
 *	a string specifying the name of the entity type. Valid entity types are
 *	those listed in scripts/entities.xml that have a base entity component.
 *
 *	@param *params The remaining, optional arguments. These arguments can be
 *	Python dictionaries, keyword arguments, or a DataSection. If a specified
 *  key is a Base property, its value will be used to initialise the Base entity
 *	property. If the key is a Cell property, it will be added into the
 *	'cellData' attribute on the base entity, which is a Python dictionary, and
 *	later be sent to initialise the cell entity properties.
 *
 *	If the same properties are provided multiple times via a DataSection,
 *	dictionary or keyword argument, the keyword argument has precedence and then
 *	the other arguments from left to right.
 *
 *	Note: If the last argument is callable, it is not included in *params.
 *
 *	@param callback callback is an optional callable object that will be called
 *	when the entity has been created. It is called with one argument that is a
 *	mailbox to the new base entity on success and None on failure.
 *
 *	@return A mailbox to the new Base entity.
 */
PY_MODULE_FUNCTION_WITH_KEYWORDS( createBaseAnywhere, BigWorld )


/**
 *	This function implements the BigWorld.createBaseRemotely script method.
 */
PyObject * py_createBaseRemotely( PyObject * args, PyObject * kwargs )
{
	return BaseApp::instance().pEntityCreator()->createBaseRemotely( args, kwargs );
}
/*~ function BigWorld.createBaseRemotely
 *	@components{ base }
 *	The function createBaseRemotely creates a new Base entity in the BigWorld
 *	server system on the BaseApp specified by the baseMB argument. The function
 *	takes the name of the entity type to be created and optionally a Python
 *	dictionary or keyword arguments to be used to provide initial values for the
 *	entity.
 *
 *	BigWorld.createBaseAnywhere should be preferred to this method and
 *	BigWorld.createBaseLocally as the server has the flexibility to create the
 *	entity on an appropriately loaded BaseApp.
 *
 *	Any values not provided will default to the entity's default value provided
 *	by the ".def" file.
 *
 *	@param entityType Specifies the type of base entity to be	created. It must be
 *	a string specifying the name of the entity type. Valid entity types are
 *	those listed in scripts/entities.xml that have a base entity component.
 *
 *	@param baseMB This is a Base entity mailbox. The new base entity will be
 *	created on the same BaseApp as this base entity.
 *
 *	@param *params The remaining, optional arguments. These arguments can be
 *	Python dictionaries, keyword arguments, or a DataSection. If a specified
 *  key is a Base property, its value will be used to initialise the Base entity
 *	property. If the key is a Cell property, it will be added into the
 *	'cellData' attribute on the base entity, which is a Python dictionary, and
 *	later be sent to initialise the cell entity properties.
 *
 *	If the same properties are provided multiple times via a DataSection,
 *	dictionary or keyword argument, the keyword argument has precedence and then
 *	the other arguments from left to right.
 *
 *	Note: If the last argument is callable, it is not included in *params.
 *
 *	@param callback callback is an optional callable object that will be called
 *	when the entity has been created. It is called with one argument that is a
 *	mailbox to the new base entity on success and None on failure.
 *
 *	@return A mailbox to the new Base entity.
 */
PY_MODULE_FUNCTION_WITH_KEYWORDS( createBaseRemotely, BigWorld )


/**
 *	This function implements the BigWorld.createBaseLocally script method.
 */
PyObject * py_createBaseLocally( PyObject * args, PyObject * kwargs )
{
	return BaseApp::instance().pEntityCreator()->createBaseLocally( args, kwargs );
}
/*~ function BigWorld.createBaseLocally
 *	@components{ base }
 *	The function createBaseLocally creates a new Base entity in the BigWorld
 *	server system. The function takes the name of the entity type to be created
 *	and optionally Python dictionaries, PyDataSections and keyword arguments to
 *	be used to provide initial values for the entity.
 *
 *	Any values not provided will default to the entity's default value provided
 *	by the ".def" file.
 *
 *	BigWorld.createBaseAnywhere should be preferred to this method and
 *	BigWorld.createBaseRemotely as the server has the flexibility to create the
 *	entity on an appropriately loaded BaseApp.
 *
 *	@param entityType Specifies the type of base entity to be created. It
 *  must be a string specifying the name of the entity type. Valid entity types
 *  are those listed in scripts/entities.xml that have a base entity component.
 *
 *	@param *params The remaining, optional arguments. These arguments can be
 *	Python dictionaries, keyword arguments, or a DataSection. If a specified
 *  key is a Base property, its value will be used to initialise the Base entity
 *	property. If the key is a Cell property, it will be added into the
 *	'cellData' attribute on the base entity, which is a Python dictionary, and
 *	later be sent to initialise the cell entity properties.
 *
 *	If not specified in the ".def" file, the property will be added to the base
 *	entity as a normal attribute. This behaviour is deprecated. You should look
 *	to add these values to the ".def" file.
 *
 *	If the same properties are provided multiple times via a DataSection,
 *	dictionary or keyword argument, the keyword argument has precedence and then
 *	the other arguments from left to right.
 *
 *	@return The newly created base entity (see Base for a description of the
 *	base entity returned).
 */
PY_MODULE_FUNCTION_WITH_KEYWORDS( createBaseLocally, BigWorld )

/*~ function BigWorld.createBase
 *	@components{ base }
 *	Alias for BigWorld.createBaseLocally.
 */
PyObject * py_createBase( PyObject * args, PyObject * kwargs )
{
	static bool isFirst = true;
	if (isFirst)
	{
		isFirst = false;
		WARNING_MSG( "py_createBase: BigWorld.createBase is deprecated as of "
				"BW 1.9. BigWorld.createBaseLocally has the same functionality "
				"and BigWorld.createBaseAnywhere is the preferred method.\n" );
	}

	return py_createBaseLocally( args, kwargs );
}
PY_MODULE_FUNCTION_WITH_KEYWORDS( createBase, BigWorld )

/*~ function BigWorld.createEntity
 *	@components{ base }
 *	Alias for BigWorld.createBaseLocally.
 */
PyObject * py_createEntity( PyObject * args, PyObject * kwargs )
{
	return py_createBaseLocally( args, kwargs );
}
PY_MODULE_FUNCTION_WITH_KEYWORDS( createEntity, BigWorld )

/**
 *	This function implements the BigWorld.time script method.
 */
PyObject * py_time( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError,
				"BigWorld.time: Excepted no arguments." );
		return NULL;
	}

	return PyFloat_FromDouble( (double)BaseApp::instance().time()/
			BaseAppConfig::updateHertz() );
}
/*~ function BigWorld.time
 *	@components{ base }
 *
 *	This method returns the current game time in seconds as a floating
 *	point number.
 *
 *	Game time is incremented with each game tick.
 *
 *  @return The current game time in seconds as a float.
 */
PY_MODULE_FUNCTION_WITH_DOC( time, BigWorld, "This method returns the current "
	"game time in seconds as a floating point number. Game time is "
	"incremented with each game tick." )



/**
 *	This function implements the BigWorld.getLoad script method.
 *
 *  @return The current BaseApp load as a float.
 */
float getLoad()
{
	return BaseApp::instance().getLoad();
}
/*~ function BigWorld.getLoad
 *	@components{ base }
 *	The function getLoad returns the current BaseApp load.
 */
PY_AUTO_MODULE_FUNCTION( RETDATA, getLoad, END, BigWorld )


/**
 *	This function implements the BigWorld.hasStarted script method.
 */
PyObject * py_hasStarted( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError,
				"BigWorld.hasStarted: Excepted no arguments." );
		return NULL;
	}

	return Script::getData( BaseApp::instance().hasStarted() );
}
/*~ function BigWorld.hasStarted
 *	@components{ base }
 *	The function hasStarted returns whether or not the server has started. This
 *	can be useful for identifying entities that are being created from the
 *	database during a controlled start-up.
 */
PY_MODULE_FUNCTION( hasStarted, BigWorld );


/**
 *	This function implements the BigWorld.isShuttingDown script method.
 */
PyObject * py_isShuttingDown( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError,
				"BigWorld.isShuttingDown: Excepted no arguments." );
		return NULL;
	}

	return Script::getData( BaseApp::instance().isShuttingDown() );
}
/*~ function BigWorld.isShuttingDown
 *	@components{ base }
 *	The function isShuttingDown returns whether or not the server is shutting
 *	down. This return True after the onBaseAppShuttingDown personality callback
 *	has been called. It can be useful for not allowing operations to start when
 *	in the process of shutting down.
 */
PY_MODULE_FUNCTION( isShuttingDown, BigWorld );



/*~ function BigWorld.createBaseLocallyFromDB
 *	@components{ base }
 *	This function attempts to create a new Base entity from data loaded from
 * 	the database. The new Base entity will be created on the BaseApp where
 * 	this function was called. If the entity is already active then a reference
 * 	to the existing Base entity will be returned.
 *
 *	@param entityType This is a string specifying the type of Base entity to
 * 	load. Entity types are listed in <res>/scripts/entities.xml.
 *
 *	@param identifier  This is a string specifying the unique identifier of
 *	                   the entity to load. The identifier property of an entity
 *	                   is specified by using the <Identifier> tag in the entity
 *	                   definition file.
 *
 *	@param callback This is an optional callable object that will be called
 *	when operation completes. The callable object will be called with three
 * 	arguments: baseRef, databaseID and wasActive. If the operation was
 * 	successful then baseRef will be a reference to the newly created Base
 * 	entity, databaseID will be the database ID of the entity and wasActive will
 * 	indicate whether the entity was already active. If wasExisting is true,
 * 	then baseRef is referring to a pre-existing Base entity and may be a
 * 	mailbox rather than a direct reference to a base entity. If the operation
 * 	failed, then baseRef will be None, databaseID will be 0 and wasActive will
 * 	be false. The most common reason for failure is the that entity does not
 * 	exist in the database but intermittent failures like timeouts or unable to
 * 	allocate IDs may also occur.
 */
bool createBaseLocallyFromDB( const BW::string & entityType,
		const BW::string & identifier, PyObjectPtr pResultHandler )
{
	return BaseApp::instance().pEntityCreator()->createBaseFromDB(
									entityType, identifier, pResultHandler );
}
PY_AUTO_MODULE_FUNCTION( RETOK, createBaseLocallyFromDB,
	ARG( BW::string, ARG( BW::string, OPTARG( PyObjectPtr, NULL, END ) ) ),
	BigWorld )

/*~ function BigWorld.createBaseFromDB
 *	@components{ base }
 *	Alias for BigWorld.createBaseLocallyFromDB.
 */
PY_MODULE_FUNCTION_ALIAS( createBaseLocallyFromDB, createBaseFromDB, BigWorld )

/*~ function BigWorld.createBaseLocallyFromDBID
 *	@components{ base }
 *	This function attempts to create a new Base entity from data loaded from
 * 	the database. The new Base entity will be created on the BaseApp where
 * 	this function was called. If the entity is already active then a reference
 * 	to the existing Base entity will be returned.
 *
 *	@param entityType This is a string specifying the type of Base entity to
 * 	load. Entity types are listed in <res>/scripts/entities.xml.
 *
 *	@param id This is a number specifying the database ID of the entity to
 * 	create. The database ID of an entity is stored in the databaseID property
 * 	of the entity.
 *
 *	@param callback This is an optional callable object that will be called
 *	when operation completes. The callable object will be called with three
 * 	arguments: baseRef, databaseID and wasActive. If the operation was
 * 	successful then baseRef will be a reference to the newly created Base
 * 	entity, databaseID will be the database ID of the entity and wasActive will
 * 	indicate whether the entity was already active. If wasExisting is true,
 * 	then baseRef is referring to a pre-existing Base entity and may be a
 * 	mailbox rather than a direct reference to a base entity. If the operation
 * 	failed, then baseRef will be None, databaseID will be 0 and wasActive will
 * 	be false. The most common reason for failure is the that entity does not
 * 	exist in the database but intermittent failures like timeouts or unable to
 * 	allocate IDs may also occur.
 */
bool createBaseLocallyFromDBID( const BW::string& entityType, DatabaseID id,
					   PyObjectPtr pResultHandler )
{
	return BaseApp::instance().pEntityCreator()->createBaseFromDB( entityType,
			id, pResultHandler );
}
PY_AUTO_MODULE_FUNCTION( RETOK, createBaseLocallyFromDBID,
	ARG( BW::string, ARG( DatabaseID, OPTARG( PyObjectPtr, NULL, END ) ) ),
	BigWorld )

/*~ function BigWorld.createBaseFromDBID
 *	@components{ base }
 *	Alias for BigWorld.createBaseLocallyFromDBID.
 */
PY_MODULE_FUNCTION_ALIAS( createBaseLocallyFromDBID, createBaseFromDBID,
 	BigWorld )

/*~ function BigWorld.createBaseAnywhereFromDB
 *	@components{ base }
 *	This function creates a new Base entity from data loaded from the database.
 *  BigWorld can choose any BaseApp on which to create the Base entity, with
 * 	the aim of balancing the load between all the BaseApps.  If the entity is
 * 	already active then a reference to the existing Base entity will be
 * 	returned.
 *
 *	@param entityType This is a string specifying the type of Base entity to
 * 	load. Entity types are listed in <res>/scripts/entities.xml.
 *
 *	@param identifier  This is a string specifying the unique identifier of
 *	                   the entity to load. The identifier property of an entity
 *	                   is specified by using the <Identifier> tag in the entity
 *	                   definition file.
 *
 *	@param callback This is an optional callable object that will be called
 *	when operation completes. The callable object will be called with three
 * 	arguments: baseRef, databaseID and wasActive. If the operation was
 * 	successful then baseRef will be a mailbox or direct reference
 *  to the newly created base entity, databaseID will be the database ID of
 * 	the entity and wasActive will indicate whether the entity was already
 * 	active. If wasActive is true then baseRef is a reference to a pre-existing
 * 	entity. If the operation failed, then baseRef will be None, databaseID will
 * 	be 0 and wasActive will be false. The most common reason for failure is the
 * 	that entity does not exist in the database but intermittent failures like
 * 	timeouts or unable to allocate IDs may also occur.
 */
PyObject * py_createBaseAnywhereFromDB( PyObject * args )
{
	const char * 	typeStr;
	const char *	identifierStr;
	PyObject *		pCallback = NULL;
	if (!PyArg_ParseTuple( args, "ss|O:createBaseAnywhereFromDB",
				&typeStr, &identifierStr, &pCallback ))
	{
		return NULL;
	}

	return BaseApp::instance().pEntityCreator()->createRemoteBaseFromDB(
			typeStr, 0, identifierStr, NULL, pCallback,
			"createBaseAnywhereFromDB" );
}
PY_MODULE_FUNCTION( createBaseAnywhereFromDB, BigWorld )

/*~ function BigWorld.createBaseAnywhereFromDBID
 *	@components{ base }
 *	This function creates a new Base entity from data loaded from the database.
 *  BigWorld can choose any BaseApp on which to create the Base entity, with
 * 	the aim of balancing the load between all the BaseApps. If the entity is
 * 	already active then a reference to the existing Base entity will be
 * 	returned.
 *
 *	@param entityType This is a string specifying the type of Base entity to
 * 	load. Entity types are listed in <res>/scripts/entities.xml.
 *
 *	@param dbID This is a number specifying the database ID of the entity to
 * 	create. The database ID of an entity is stored in the databaseID property
 * 	of the entity.
 *
 *	@param callback This is an optional callable object that will be called
 *	when operation completes. The callable object will be called with three
 * 	arguments: baseRef, databaseID and wasActive. If the operation was
 * 	successful then baseRef will be a mailbox or direct reference
 *  to the newly created base entity, databaseID will be the database ID of
 * 	the entity and wasActive will indicate whether the entity was already
 * 	active. If wasActive is true then baseRef is a reference to a pre-existing
 * 	entity. If the operation failed, then baseRef will be None, databaseID will
 * 	be 0 and wasActive will be false. The most common reason for failure is the
 * 	that entity does not exist in the database but intermittent failures like
 * 	timeouts or unable to allocate IDs may also occur.
 */
PyObject * py_createBaseAnywhereFromDBID( PyObject * args )
{
	const char * 	typeStr;
	DatabaseID		dbID;
	PyObject *		pCallback = NULL;
	if (!PyArg_ParseTuple( args, "sL|O:createBaseAnywhereFromDBID",
				&typeStr, &dbID, &pCallback ))
	{
		return NULL;
	}

	if (dbID == 0)
	{
		PyErr_SetString( PyExc_ValueError,
			"BigWorld.createBaseAnywhereFromDBID() "
				"DatabaseID must be non-zero" );
		return NULL;
	}

	return BaseApp::instance().pEntityCreator()->createRemoteBaseFromDB(
			typeStr, dbID, NULL, NULL, pCallback, "createBaseAnywhereFromDBID" );
}
PY_MODULE_FUNCTION( createBaseAnywhereFromDBID, BigWorld )

/*~ function BigWorld.createBaseRemotelyFromDB
 *	@components{ base }
 *	This function creates a new Base entity from data loaded from the database.
 *  The new Base entity will be created on the same BaseApp as an existing
 * 	Base entity specified by the caller. If the entity is already active then a
 * 	reference to the active Base entity will be returned and there is no
 * 	guarantee that the active is on the same BaseApp as the Base entity passed
 * 	to this function.
 *
 *	@param entityType This is a string specifying the type of Base entity to
 * 	load. Entity types are listed in <res>/scripts/entities.xml.
 *
 *	@param identifier  This is a string specifying the unique identifier of
 *	                   the entity to load. The identifier property of an entity
 *	                   is specified by using the <Identifier> tag in the entity
 *	                   definition file.
 *
 * 	@param baseMB This is a Base entity mailbox. The new base entity will be
 * 	created on the same BaseApp as this base entity
 *
 *	@param callback This is an optional callable object that will be called
 *	when operation completes. The callable object will be called with three
 * 	arguments: baseRef, databaseID and wasActive. If the operation was
 * 	successful then baseRef will be a mailbox or direct reference
 *  to the newly created base entity, databaseID will be the database ID of
 * 	the entity and wasActive will indicate whether the entity was already
 * 	active. If wasActive is true then baseRef is a reference to a pre-existing
 * 	entity. If the operation failed, then baseRef will be None, databaseID will
 * 	be 0 and wasActive will be false. The most common reason for failure is the
 * 	that entity does not exist in the database but intermittent failures like
 * 	timeouts or unable to allocate IDs may also occur.
 */
PyObject * py_createBaseRemotelyFromDB( PyObject * args )
{
	const char * 	typeStr;
	const char *	identifierStr;
	PyObject *		pBaseMB;
	PyObject *		pCallback = NULL;
	if (!PyArg_ParseTuple( args, "ssO|O:createBaseRemotelyFromDB",
				&typeStr, &identifierStr, &pBaseMB, &pCallback ))
	{
		return NULL;
	}

	Mercury::Address addr;
	if (BaseEntityMailBox::Check( pBaseMB ))
	{
		addr = static_cast<BaseEntityMailBox *>( pBaseMB )->address();
	}
	else if (Base::Check( pBaseMB ))
	{
		addr = BaseApp::instance().intInterface().address();
	}
	else
	{
		PyErr_Format( PyExc_TypeError,
			"createBaseRemotely() argument 2 must be a mailbox or an entity" );
		return NULL;
	}

	return BaseApp::instance().pEntityCreator()->createRemoteBaseFromDB(
			typeStr, 0, identifierStr, &addr, pCallback,
			"createBaseRemotelyFromDB" );
}
PY_MODULE_FUNCTION( createBaseRemotelyFromDB, BigWorld )

/*~ function BigWorld.createBaseRemotelyFromDBID
 *	@components{ base }
 *	This function creates a new Base entity from data loaded from the database.
 *  The new Base entity will be created on the same BaseApp as an existing
 * 	Base entity specified by the caller. If the entity is already active then a
 * 	reference to the active Base entity will be returned and there is no
 * 	guarantee that the active is on the same BaseApp as the Base entity passed
 * 	to this function.
 *
 *	@param entityType This is a string specifying the type of Base entity to
 * 	load. Entity types are listed in <res>/scripts/entities.xml.
 *
 *	@param dbID This is a number specifying the database ID of the entity to
 * 	create. The database ID of an entity is stored in the databaseID property
 * 	of the entity.
 *
 * 	@param baseMB This is a Base entity mailbox. The new base entity will be
 * 	created on the same BaseApp as this base entity
 *
 *	@param callback This is an optional callable object that will be called
 *	when operation completes. The callable object will be called with three
 * 	arguments: baseRef, databaseID and wasActive. If the operation was
 * 	successful then baseRef will be a mailbox or direct reference
 *  to the newly created base entity, databaseID will be the database ID of
 * 	the entity and wasActive will indicate whether the entity was already
 * 	active. If wasActive is true then baseRef is a reference to a pre-existing
 * 	entity. If the operation failed, then baseRef will be None, databaseID will
 * 	be 0 and wasActive will be false. The most common reason for failure is the
 * 	that entity does not exist in the database but intermittent failures like
 * 	timeouts or unable to allocate IDs may also occur.
 */
PyObject * py_createBaseRemotelyFromDBID( PyObject * args )
{
	const char * 	typeStr;
	DatabaseID		dbID;
	PyObject *		pBaseMB;
	PyObject *		pCallback = NULL;
	if (!PyArg_ParseTuple( args, "sLO|O:createBaseRemotelyFromDBID",
				&typeStr, &dbID, &pBaseMB, &pCallback ))
	{
		return NULL;
	}

	if (dbID == 0)
	{
		PyErr_SetString( PyExc_ValueError,
			"BigWorld.createBaseRemotelyFromDBID() "
				"DatabaseID must be non-zero" );
		return NULL;
	}

	Mercury::Address addr;
	if (BaseEntityMailBox::Check( pBaseMB ))
	{
		addr = static_cast<BaseEntityMailBox *>( pBaseMB )->address();
	}
	else if (Base::Check( pBaseMB ))
	{
		addr = BaseApp::instance().intInterface().address();
	}
	else
	{
		PyErr_Format( PyExc_TypeError,
			"createBaseRemotely() argument 2 must be a mailbox or an entity" );
		return NULL;
	}

	return BaseApp::instance().pEntityCreator()->createRemoteBaseFromDB(
		typeStr, dbID, NULL, &addr, pCallback, "createBaseRemotelyFromDBID" );
}
PY_MODULE_FUNCTION( createBaseRemotelyFromDBID, BigWorld )

/*~ function BigWorld.createBaseAnywhereFromTemplate
 *	@components{ base }
 *	The function createBaseAnywhereFromTemplate creates a new Base entity
 *	in the BigWorld server system using a template to populate entity properties.
 *  The server may choose to create the Base entity on any BaseApp.
 *
 *	The function takes the name of the entity type to be created and id of template
 *	to be used to provide initial values for the entity properties.
 *
 *	@param entityType Specifies the type of base entity to be created. It must be
 *	a string specifying the name of the entity type. Valid entity types are
 *	those listed in scripts/entities.xml that have a base entity component.
 *
 *	@param templateID Specifies id of template to create entity from. It must be
 *	a BLOB specifying the template.
 *
 *	@param callback callback is an optional callable object that will be called
 *	when the entity has been created. It is called with one argument that is a
 *	mailbox to the new base entity on success and None on failure.
 *
 *	@return A mailbox to the new Base entity.
 */
PyObject * createBaseAnywhereFromTemplate(
		const BW::string & entityType,
		const BW::string & templateID,
		const ScriptObject & callback )
{
	return BaseApp::instance().pEntityCreator()->createBaseAnywhereFromTemplate(
			entityType, templateID, callback ).newRef();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, createBaseAnywhereFromTemplate,
	ARG( BW::string, 
		ARG( BW::string, 
			OPTARG( ScriptObject, ScriptObject::none(), END ) ) ), 
	BigWorld )


/*~ function BigWorld.createBaseLocallyFromTemplate
 *	@components{ base }
 *	The function createBaseLocallyFromTemplate creates a new Base entity
 *	on this baseapp using a template to populate entity properties.
 *
 *	The function takes the name of the entity type to be created and id of template
 *	to be used to provide initial values for the entity properties.
 *
 *	@param entityType Specifies the type of base entity to be created. It must be
 *	a string specifying the name of the entity type. Valid entity types are
 *	those listed in scripts/entities.xml that have a base entity component.
 *
 *	@param templateID Specifies id of template to create entity from. It must be
 *	a BLOB specifying the template.
 *
 *	@return A new Base entity.
 */
PyObject * createBaseLocallyFromTemplate(
		const BW::string & entityType,
		const BW::string & templateID )
{
	return BaseApp::instance().pEntityCreator()->createBaseLocallyFromTemplate(
			entityType, templateID ).newRef();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, createBaseLocallyFromTemplate,
	ARG( BW::string, 
		ARG( BW::string, END ) ), 
	BigWorld )

/*~ function BigWorld.createBaseRemotelyFromTemplate
 *	@components{ base }
 *	The function createBaseRemotelyFromTemplate creates a new Base entity
 *	on remote baseapp using a template to populate entity properties.
 *
 *	The function takes the name of the entity type to be created and id of template
 *	to be used to provide initial values for the entity properties.
 *  
 *	@param entityType Specifies the type of base entity to be created. It must be
 *	a string specifying the name of the entity type. Valid entity types are
 *	those listed in scripts/entities.xml that have a base entity component.
 *
 *	@param templateID Specifies id of template to create entity from. It must be
 *	a BLOB specifying the template.
 *
 *	@param baseMB This could be a direct base entity reference or a mailbox of type
 *	'base', 'cell via base' or 'client via base'. 
 *  The new base entity will be	created on the same BaseApp as the corresponding base entity.
 *
 *	@param callback callback is an optional callable object that will be called
 *	when the entity has been created. It is called with one argument that is a
 *	mailbox to the new base entity on success and None on failure.
 *
 *	@return A mailbox to the new Base entity.
 */
PyObject * createBaseRemotelyFromTemplate(
		const BW::string & entityType,
		const BW::string & templateID,
		const EntityMailBoxRef & baseMB,
		const ScriptObject & callback )
{
	if (EntityMailBoxRef::BASE != baseMB.component() &&
		EntityMailBoxRef::CELL_VIA_BASE != baseMB.component() &&
		EntityMailBoxRef::CLIENT_VIA_BASE != baseMB.component())
	{
		PyErr_SetString(PyExc_ValueError, 
			"BigWorld.createBaseRemotelyFromTemplate: "
			"Argument 3 must be a 'base', 'cell via base' or 'client via base' mailbox");
		return NULL;
	}

	return BaseApp::instance().pEntityCreator()->createBaseRemotelyFromTemplate(
			baseMB.addr, entityType, templateID, callback ).newRef();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, createBaseRemotelyFromTemplate,
	ARG( BW::string, 
		ARG( BW::string, 
			ARG( EntityMailBoxRef,
				OPTARG( ScriptObject, ScriptObject::none(), END ) ) ) ), 
	BigWorld )

/*
 *	This is a bit hacky. It is used by the templatised class DBResultWaiter to
 *	convert a response. This class is only currently used by one type,
 *	EntityMailBoxRef. If it is used by others, this function will need to be
 *	implemented for them.
 */
PyObject * convertDBResultWaiterReply( EntityMailBoxRef & mailboxRef,
	   PyObject * pAsScript )
{
	if (pAsScript == Py_None)
	{
		WARNING_MSG( "DBResultWaiter::handleMessage: "
					"Unable to convert mailbox. id = %u. addr = %s\n",
				mailboxRef.id, mailboxRef.addr.c_str() );
		WARNING_MSG( "\tThis can occur if an entity on the local BaseApp was "
				"looked up but was destroyed in the meantime.\n" );

		Py_DECREF( pAsScript );
		// Return True as it is no longer checked out.
		pAsScript = Script::getData( true );
	}

	return pAsScript;
}

/**
 *	Stub version for DatabaseID
 */
inline
PyObject * convertDBResultWaiterReply( DatabaseID dbID, PyObject * pAsScript )
{
	return pAsScript;
}

/*
// This can be used if DBResultWaiter is used with any other template args.
template <class C>
PyObject * convertDBResultWaiterReply( C & c, PyObject * pAsScript )
{
	return pAsScript;
}
*/


/// Helper templates for different data
template <class C> struct ResponseParser
{
	static bool ok( int length ) { return length == sizeof(C); }
	static C & parse( BinaryIStream & data, int length )
		{ return *(C*)data.retrieve( sizeof(C) ); }
};

/**
 *	This class waits for a result message from the database.
 *	Then it calls the handler. If the result is empty then it is called
 *	with True. If the result is not empty then it is interpreted as the
 *	template argument, and called with that value converted to a PyObject.
 *	If the result is the wrong size or does not arrive, then it is called
 *	with False.
 */
template <class C> class DBResultWaiter :
	public Mercury::ReplyMessageHandler
{
public:
	DBResultWaiter( PyObjectPtr pResultHandler ) :
		pResultHandler_( pResultHandler.get() )
	{
		Py_XINCREF( pResultHandler_ );
	}

private:
	typedef ResponseParser<C> RC;

	void handleMessage(const Mercury::Address& srcAddr,
		Mercury::UnpackedMessageHeader& header,
		BinaryIStream& data, void * /*arg*/)
	{
		PyObject * callArg;

		if (header.length == 0)
		{
			callArg = Script::getData( true );
		}
		else if (RC::ok( header.length ))
		{
			C & reply = RC::parse( data, header.length );
			callArg = Script::getData( reply );

			callArg = convertDBResultWaiterReply( reply, callArg );

			// None can be returned if the mailbox is local but the entity does not
			// exist.
			if (callArg == Py_None)
			{
				WARNING_MSG( "DBResultWaiter::done:" );
				Py_DECREF( callArg );
				callArg = Script::getData( true );
			}
		}
		else // wrong size data i.e. error - so call with false
		{
			callArg = Script::getData( false );
			data.finish();	// don't care about the result
		}
		this->done( callArg );
	}

	void handleException(const Mercury::NubException& /*ne*/, void* /*arg*/)
	{
		ERROR_MSG( "DBResultWaiter::handleException: "
			"DB call timed out\n" );

		this->done( Script::getData( false ) );
	}

	void handleShuttingDown(const Mercury::NubException& /*ne*/, void* /*arg*/)
	{
		INFO_MSG( "DBResultWaiter::handleShuttingDown: Ignoring\n" );

		delete this;
	}

	/**
	 *	Call the callback, decref the arg, and delete ourselves
	 */
	void done( PyObject * callArg )
	{
		if (pResultHandler_)
		{
			// Note: This consumes the reference to pResultHandler_
			Script::call( pResultHandler_, Py_BuildValue( "(O)", callArg ),
				"DBResultWaiter callback", /*okIfFnNull:*/false );
			// 'call' does the decref of pResultHandler_ for us
			pResultHandler_ = NULL;
		}
		Py_DECREF( callArg );

		delete this;
	}

	PyObject		* pResultHandler_;
};

/*~ function BigWorld.deleteBaseByDBID
 *	@components{ base }
 *	This script function deletes the specified entry from the database,
 *	if it is not already checked out. If it is checked out, then the
 *	result handler will be called with the mailbox of the currently
 *	checked out entity.
 *
 *	@param entityType entityType is a string that specifies the type of the
 *	entity to be deleted.
 *	@param dbID Specifies the database ID of the entity to delete.
 *	@param callback callback is a callable object (e.g. a function) that takes
 *	one argument. If deletion is successful, the callback is called with the
 *	value True, if it is unsuccessful because the specified entry is currently
 *	checked out, it is called with the mailbox of that base entity, and if it
 *	is unsuccessful for any other reason (e.g. specified entry does not exist)
 *	then it is called with the value False.
 */
bool deleteBaseByDBID( const BW::string & entityType, DatabaseID dbID,
	PyObjectPtr pResultHandler )
{
	if (dbID == 0)
	{
		PyErr_SetString( PyExc_ValueError, "BigWorld.deleteBaseByDBID() "
				"DatabaseID must be non-zero" );
		return false;
	}

	EntityTypePtr pType = EntityType::getType( entityType.c_str(),
			/*shouldExcludeServices*/true );

	if (!pType)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.deleteBaseByDBID() "
			"Invalid entity type %s", entityType.c_str() );
		return false;
	}

	if (pResultHandler && !PyCallable_Check( pResultHandler.get() ) )
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.deleteBaseByDBID() "
			"callback must be callable if specified" );
		return false;
	}

	DBApp & dbApp = BaseApp::instance().dbApp();
	Mercury::Bundle & bundle = dbApp.bundle();

	DBAppInterface::deleteEntityArgs & dea =
		DBAppInterface::deleteEntityArgs::startRequest( bundle,
			new DBResultWaiter<EntityMailBoxRef>( pResultHandler ) );

	dea.entityTypeID = pType->id();
	dea.dbid = dbID;

	dbApp.send();

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, deleteBaseByDBID,
	ARG( BW::string, ARG( DatabaseID, OPTARG( PyObjectPtr, NULL, END ) ) ),
	BigWorld )


/*~ function BigWorld.lookUpBaseByDBID
 *	@components{ base }
 *	This script function looks up the specified base entry in the database,
 *	and calls the result handler with its mailbox.
 *
 *	@param entityType entityType is a string that specifies the type of the
 *	entity to look up.
 *	@param dbID Specifies the database ID of the entity to look up.
 *	@param callback callback is a callable object (e.g. a function) that takes
 *	one argument. If the lookup is successful, the callback is called with the
 *	base mailbox; If the entity is not currently checked out, the callback is
 *	called with the value True, regardless of whether the entity exists in
 *	the database or not; For any other cases, it is called with the value False.
 */
bool lookUpBaseByDBID( const BW::string & entityType, DatabaseID dbID,
	PyObjectPtr pResultHandler )
{
	if (dbID == 0)
	{
		PyErr_SetString( PyExc_ValueError, "BigWorld.lookUpBaseByDBID() "
				"DatabaseID must be non-zero" );
		return false;
	}

	EntityTypePtr pType = EntityType::getType( entityType.c_str(),
			/*shouldExcludeServices*/true );

	if (!pType)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.lookUpBaseByDBID() "
			"Invalid entity type %s", entityType.c_str() );
		return false;
	}

	if (!pResultHandler || !PyCallable_Check( pResultHandler.get() ) )
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.lookUpBaseByDBID() "
			"callback must be callable" );
		return false;
	}

	const DBAppGateway & dbApp = BaseApp::instance().dbAppGatewayFor( dbID );
		
	if (dbApp.address().isNone())
	{
		PyErr_Format( PyExc_RuntimeError, "BigWorld.lookUpBaseByDBID() "
			"No DBApp is available, data for %" PRI64 " has been lost", dbID );
		return false;
	}

	Mercury::UDPChannel & dbAppChannel =
		BaseApp::instance().getChannel( dbApp.address() );

	Mercury::Bundle & bundle = dbAppChannel.bundle();

	DBAppInterface::lookupEntityArgs & lea =
		DBAppInterface::lookupEntityArgs::startRequest( bundle,
			new DBResultWaiter<EntityMailBoxRef>( pResultHandler ) );

	lea.entityTypeID = pType->id();
	lea.dbid = dbID;

	if (BaseAppConfig::shouldDelayLookUpSend())
	{
		BaseApp::instance().intInterface().delayedSend( dbAppChannel );
	}
	else
	{
		dbAppChannel.send();
	}

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, lookUpBaseByDBID,
	ARG( BW::string, ARG( DatabaseID, ARG( PyObjectPtr, END ) ) ),
	BigWorld )
PY_MODULE_FUNCTION_ALIAS( lookUpBaseByDBID, lookupBaseByDBID, BigWorld )

/*~ function BigWorld.lookUpBaseByName
 *	@components{ base }
 *	This script function looks up the specified base entry in the database,
 *	and calls the result handler with its mailbox.
 *
 *	@param entityType entityType is a string that specifies the type of the
 *	entity to look up.
 *	@param name Specifies the name of the entity to look up.
 *	@param callback callback is a callable object (e.g. a function) that takes
 *	one argument. If the lookup is successful, the callback is called with the
 *	base mailbox; If the entity is not currently checked out, but exists in the
 *	database, the callback is called with the value True; For any other cases,
 *	it is called with the value False.	
 */
bool lookUpBaseByName( const BW::string & entityType,
	const BW::string & name, PyObjectPtr pResultHandler )
{
	EntityTypePtr pType = EntityType::getType( entityType.c_str(),
			/*shouldExcludeServices*/true );

	if (!pType)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.lookUpBaseByName() "
			"Invalid entity type %s", entityType.c_str() );
		return false;
	}

	if (!pResultHandler || !PyCallable_Check( pResultHandler.get() ) )
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.lookUpBaseByName() "
			"callback must be callable" );
		return false;
	}

	if (pType->description().pIdentifier() == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.lookUpBaseByName() "
				"Entity type %s has no identifier property",
			entityType.c_str() );

		return false;
	}

	DBApp & dbApp = BaseApp::instance().dbApp();
	Mercury::Bundle & bundle = dbApp.bundle();

	bundle.startRequest( DBAppInterface::lookupEntityByName,
		new DBResultWaiter<EntityMailBoxRef>( pResultHandler ) );
	bundle << pType->id() << name;

	dbApp.send();

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, lookUpBaseByName,
	ARG( BW::string, ARG( BW::string, ARG( PyObjectPtr, END ) ) ),
	BigWorld )
PY_MODULE_FUNCTION_ALIAS( lookUpBaseByName, lookupBaseByName, BigWorld )


/*~ function BigWorld.lookUpDBIDByName
 *	@components{ base }
 *	This script function looks up the database ID of an entity based on its
 *  name. It calls the result handler with the database ID.
 *
 *	@param entityType entityType is a string that specifies the type of the
 *	entity to look up.
 *	@param name Specifies the name of the entity to look up.
 *	@param callback callback is a callable object (e.g. a function) that takes
 *	one argument: the database ID. If the specified entity does not exist then
 * 	the database ID will be 0.
 */
bool lookUpDBIDByName( const BW::string & entityType,
	const BW::string & name, PyObjectPtr pResultHandler )
{
	EntityTypePtr pType = EntityType::getType( entityType.c_str(),
			/*shouldExcludeServices*/true );

	if (!pType)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.lookUpDBIDByName() "
			"Invalid entity type %s", entityType.c_str() );
		return false;
	}

	if (pResultHandler && !PyCallable_Check( pResultHandler.get() ) )
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.lookUpDBIDByName() "
			"callback must be callable if specified" );
		return false;
	}

	DBApp & dbApp = BaseApp::instance().dbApp();
	Mercury::Bundle & bundle = dbApp.bundle();

	bundle.startRequest( DBAppInterface::lookupDBIDByName,
		new DBResultWaiter<DatabaseID>( pResultHandler ) );
	bundle << pType->id() << name;

	dbApp.send();

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, lookUpDBIDByName,
	ARG( BW::string, ARG( BW::string, OPTARG( PyObjectPtr, NULL, END ) ) ),
	BigWorld )


/**
 *	This class represents handlers for the asynchronous calls to the database
 *	to look up entities by a single property.
 */
class LookUpBasesByIndexHandler : public Mercury::ReplyMessageHandler
{
public:
	LookUpBasesByIndexHandler( PyObjectPtr pHandler );

	virtual ~LookUpBasesByIndexHandler();


	virtual void handleMessage( const Mercury::Address & source,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			void * arg );

	virtual void handleException( const Mercury::NubException & exception,
			void * arg );

private:
	void notifyFailure();

	PyObject * pHandler_;
};


/**
 *	Constructor.
 */
LookUpBasesByIndexHandler::LookUpBasesByIndexHandler( PyObjectPtr pHandler ):
		pHandler_( pHandler.get() )
{
	Py_INCREF( pHandler_ );
}


/**
 *	Destructor.
 */
LookUpBasesByIndexHandler::~LookUpBasesByIndexHandler()
{
	MF_ASSERT( pHandler_ == NULL );
}


/**
 *	Override for Mercury::ReplyMessageHandler for accepting a reply message for
 *	this request.
 */
void LookUpBasesByIndexHandler::handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg )
{
	int numEntities;
	data >> numEntities;

	if (numEntities < 0)
	{
		this->notifyFailure();
		delete this;
		return;
	}

	PyObject * pList = PyList_New( numEntities );
	for (Py_ssize_t i = 0; i < Py_ssize_t( numEntities ); ++i)
	{
		DatabaseID dbID;
		EntityMailBoxRef location;

		data >> dbID >> location;

		PyObject * pMailBox;

		if (location.id != 0)
		{
			pMailBox = Script::getData( location );
		}
		else
		{
			Py_INCREF( Py_None );
			pMailBox = Py_None;
		}

		PyList_SetItem( pList, i, Py_BuildValue( "(KO)", dbID, pMailBox ) );

		Py_DECREF( pMailBox );
	}

	Script::call( pHandler_, PyTuple_Pack( 1, pList ),
		"BigWorld.lookUpBasesByIndex: " );

	// Script::call decrefs pHandler_ and the argument tuple for us.
	pHandler_ = NULL;

	Py_DECREF( pList );

	delete this;
}


/**
 *	Override from Mercury::ReplyMessageHandler when the request has failed.
 */
void LookUpBasesByIndexHandler::handleException(
		const Mercury::NubException & exception, void * args )
{
	ERROR_MSG( "LookUpBasesByIndexHandler::handleException: %s\n",
		Mercury::reasonToString( exception.reason() ) );
	this->notifyFailure();
	delete this;
}


/**
 *	Called when the request fails.
 */
void LookUpBasesByIndexHandler::notifyFailure()
{
	Script::call( pHandler_, PyTuple_Pack( 1, Py_None ),
		"BigWorld.lookUpBasesByIndex: " );

	// Script::call decrefs pHandler_ and the argument tuple for us.
	pHandler_ = NULL;
}


/*~ function BigWorld.lookUpBasesByIndex
 *	@components{ base }
 *
 *	Perform an asynchronous look-up of persistent base entities stored in the
 *	primary database based on matching some of its properties. For efficiency,
 *	query properties should be an indexed property as defined in the entity
 *	definitions for the given entity type. They are specified in the search
 *	as keyword arguments to this method. The property matches are combined using
 *	the AND operator.
 *
 *	If successful, the callback argument is called with a list of pairs for
 *	each matching persistent entity:
 *
 *	[ (databaseID0, baseMailBox0), (databaseID1, baseMailBox1), ... ]
 *
 *	If the entity is currently checked out, the base mailbox is returned as the
 *	second element of the pair. If the entity is not current checked out, the
 *	second element of the pair will be None.
 *
 *	If the request resulted in a database error of some kind (this does not
 *	include the case where zero entities are matched), the callback argument
 *	will be called with None instead. Examples include if the property is of an
 *	inappropriate type to compare with a string (e.g. sequence and structured
 *	data types, PYTHON types, FLOAT* types).
 *
 *	Note that this method queries the primary database. If secondary databases
 *	are used, the data may be out-of-date with respect to what is contained in
 *	the secondary databases.
 *
 *	@param entityType		The name of the entity type to query for.
 *	@param callback			A callable object that is called back when matched
 *							results are received.
 *	@param filter			(Optional) One of the
							BigWorld.LOOK_UP_ENTITIES_FILTER_constants.
							(default is	BigWorld.LOOK_UP_ENTITIES_FILTER_ALL).
 *	@param sortProperty		(Optional) The property name to sort on.
 *	@param sortAscending	(Optional) If True, sort in ascending or descending
 *							order. Default True.
 *	@param limit			(Optional) Limit for the number of results
 *							returned, defaults to -1, indicating no limit.
 *	@param offset			(Optional) Offset for the results returned,
 *							defaults to 0.
 */
PyObject * py_lookUpBasesByIndex( PyObject * args, PyObject * kwargs )
{
	char * entityType;
	PyObject * callback;
	uint16 filter = LOOK_UP_ENTITIES_FILTER_ALL;
	char * sortPropertyCString = NULL;
	uint8 ascending = 1;
	int limit = -1;
	int offset = 0;

	if (!PyArg_ParseTuple( args, "sO|HsBii", &entityType, &callback,
			&filter, &sortPropertyCString, &ascending, &limit, &offset ))
	{
		return NULL;
	}

	if (!PyCallable_Check( callback ))
	{
		PyErr_SetString( PyExc_TypeError,
			"callback argument must be callable" );
		return NULL;
	}

	LookUpEntitiesCriteria criteria;

	if (filter >= uint16( LOOK_UP_ENTITIES_FILTER_INVALID ))
	{
		PyErr_SetString( PyExc_ValueError, "filter argument is invalid" );
		return NULL;
	}

	criteria.filter( LookUpEntitiesFilter( filter ) );

	if (sortPropertyCString)
	{
		criteria.sortProperty( sortPropertyCString );
		criteria.isAscending( ascending );
	}

	criteria.limit( limit );
	criteria.offset( offset );

	EntityTypePtr pEntityType = EntityType::getType( entityType,
			/*shouldExcludeServices*/ true );
	if (pEntityType == NULL)
	{
		PyErr_Format( PyExc_ValueError, "Invalid entity type \"%s\"",
			entityType );
		return NULL;
	}

	const EntityDescription & entityDesc = pEntityType->description();

	if (!criteria.sortProperty().empty())
	{
		DataDescription * pDataDesc = entityDesc.findCompoundProperty(
			criteria.sortProperty().c_str() );
		if (pDataDesc == NULL)
		{
			PyErr_Format( PyExc_ValueError,
				"\"%s\" is not a property on \"%s\"",
				criteria.sortProperty().c_str(), entityType );
			return NULL;
		}
	}

	Py_ssize_t pos = 0;
	PyObject * key = NULL;
	PyObject * value = NULL;

	while (kwargs && PyDict_Next( kwargs, &pos, &key, &value ))
	{
		PyObjectPtr pValueString( PyObject_Str( value ),
			PyObjectPtr::STEAL_REFERENCE );

		const char * propertyName = PyString_AsString( key );
		DataDescription * pDataDesc = 
			entityDesc.findCompoundProperty( propertyName );
		if (pDataDesc == NULL)
		{
			PyErr_Format( PyExc_ValueError,
				"\"%s\" is not a property on \"%s\"",
				propertyName, entityType );
			return NULL;
		}
		else if (!pDataDesc->dataType()->isSameType( 
				ScriptObject( value, ScriptObject::FROM_BORROWED_REFERENCE ) ))
		{
			PyErr_Format( PyExc_TypeError,
				"Search value for %s.%s has incorrect type",
				entityType, propertyName );
			return NULL;
		}

		criteria.addPropertyQuery( propertyName,
			PyString_AsString( pValueString.get() ) );
	}


	Mercury::Bundle & bundle = BaseApp::instance().dbApp().bundle();

	bundle.startRequest( DBAppInterface::lookupEntities,
		new LookUpBasesByIndexHandler( callback ) );

	bundle << pEntityType->id() << criteria;

	BaseApp::instance().dbApp().send();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION_WITH_KEYWORDS_WITH_DOC( lookUpBasesByIndex, BigWorld,
	"Perform an asynchronous look-up persistent base entities stored in the"
	"primary database based on matching some of its properties specified as "
	"keyword arguments (propertyName=propertyValue). For efficiency, the "
	"properties should be indexed properties as defined in the entity "
	"definitions for the given entity type.\n"
	"If successful, the callback argument is called with a list of pairs for "
	"each matching persistent entity:\n"
	"\n"
	"[ (databaseID0, baseMailBox0), (databaseID1, baseMailBox1), ... ]"
	"\n"
	"If the entity is currently checked out, the base mailbox is returned as "
	"the second element of the pair. If the entity is not current checked out, "
	"the second element of the pair will be None."
	"\n"
	"If the request resulted in a database error of some kind (this does not "
	"include the case where zero entities are matched), the callback argument "
	"will be called with None instead. Examples include if the property is of an "
	"inappropriate type to compare with a string (e.g. sequence and structured "
	"data types, PYTHON types, FLOAT* types)." )


/**
 *	This method implements the BigWorld.executeRawDatabaseCommand function.
 *  See DBAppInterfaceUtils::executeRawDatabaseCommand for more details.
 */
bool executeRawDatabaseCommand( const BW::string & command,
	PyObjectPtr pResultHandler )
{
	BaseApp & baseApp = BaseApp::instance();
	return DBAppInterfaceUtils::executeRawDatabaseCommand( command,
		pResultHandler, baseApp.dbApp().channel() );
}
PY_AUTO_MODULE_FUNCTION( RETOK, executeRawDatabaseCommand,
	ARG( BW::string, OPTARG( PyObjectPtr, NULL, END ) ),
	BigWorld )


bool reloadScript( bool isFullReload )
{
	// Load entity defs in new Python interpreter.
	PyThreadState* pNewPyIntrep = Script::createInterpreter();
	PyThreadState* pOldPyIntrep = Script::swapInterpreter( pNewPyIntrep );

	bool isOK = (isFullReload) ?
					EntityType::init( true/*isReload*/,
							BaseApp::instance().isServiceApp() ) :
					EntityType::reloadScript();

	if (isOK)
	{
		// Restore original Python interpreter. EntityType::init() got a copy
		// of the loaded modules. And EntityType::migrate() will replace the
		// modules in the original Python interpreter with new modules.
		// __kyl__ (8/2/2006) Can't simply throw away old interpreter and
		// use new interpreter since we are currently inside a Python call
		// so the old interpreter is still on the callstack.
		Script::swapInterpreter( pOldPyIntrep );
		Script::destroyInterpreter( pNewPyIntrep );

		EntityType::migrate( isFullReload );

		ServerEntityMailBox::migrateMailBoxes();

		// go through all the bases and proxies and change their class
		const Bases & b = BaseApp::instance().bases();
		for (Bases::const_iterator it = b.begin(); it != b.end(); it++)
		{
			it->second->migrate();
		}

		EntityType::cleanupAfterReload( isFullReload );

		return true;
	}
	else
	{
		// Restore original Python interpreter.
		Script::swapInterpreter( pOldPyIntrep );

		if (!isFullReload)
		{
			// Try using the old scripts again.
			// NOTE: This works (mostly) because when we try to import a module
			// that's already imported, it doesn't try to read it from the
			// disk.
			MF_VERIFY( EntityType::reloadScript( true ) );
		}

		EntityType::cleanupAfterReload( isFullReload );
		Script::destroyInterpreter( pNewPyIntrep );

		return false;
	}
}

/*~ function BigWorld.reloadScript
 * @components{ base }
 *
 *	This method reloads Python modules related to entities and custom
 *	data types. The class of current entities are set to the newly loaded
 *	classes. This method should be used for development only and is not
 *	suitable for production use. The following points should be noted:
 *
 *	1) Scripts are only reloaded on the component baseapp that
 *	this method is called on. The user is responsible for making sure that
 *	reloadScript is called on all server components.
 *
 *	2) The client doesn't have equivalent functionality in Python. Each
 *	BigWorld client needs to use CapsLock + F11 in order to reload the scripts.
 *
 *	3) Doesn't handle custom data types transparently. Instances of custom
 *	data types must have their __class__ reassigned to the proper class type
 *	after reloadScript() completes. For example, if we have a custom data type
 *	named "CustomClass":
 *
 *  @{
 *  for e in BigWorld.entities.values():
 *     if type( e ) is Avatar.Avatar:
 *        e.customData.__class__ = CustomClass
 *  @}
 *	BWPersonality.onInit( True ) is called when this method completes (where
 *	BWPersonality is the name of the personality module specified in
 *	&lt;root&gt;/&lt;personality&gt; in bw.xml).
 *
 *	@param fullReload Optional boolean parameter that specifies whether to
 * 	reload entity definitions as well. If this parameter is False, then
 * 	entities definitions are not reloaded. Default is True.
 *
 *  @return True if reload successful, False otherwise.
 */

PY_AUTO_MODULE_FUNCTION( RETDATA, reloadScript, OPTARG( bool, true, END ),
		BigWorld )


/*~ function BigWorld.startService
 *	@components{ base }
 *
 *	This method starts a service with the given name. If the service is already
 *	running, an exception is raised.
 *
 *	@param serviceName The name of the service to start.
 */
PyObject * startService( const BW::string & serviceName )
{
	BaseApp & app = BaseApp::instance();

	if (!app.isServiceApp())
	{
		PyErr_SetString( PyExc_TypeError,
				"This method is only available on ServiceApps" );
		return NULL;
	}

	Base * pBase = app.localServiceFragments().findInstanceWithType(
			serviceName.c_str() );

	if (pBase)
	{
		PyErr_Format( PyExc_ValueError,
				"%s is already running",
				serviceName.c_str() );
		return NULL;
	}

	EntityTypePtr pType = EntityType::getType( serviceName.c_str() );

	if (!pType || !pType->isService())
	{
		PyErr_Format( PyExc_ValueError,
				"Invalid service type '%s'", serviceName.c_str() );
		return NULL;
	}

	PyObject * pResult = app.pEntityCreator()->createBase( pType.get(), NULL );

	if (!pResult)
	{
		PyErr_Format( PyExc_ValueError,
				"Failed to start %s", serviceName.c_str() );
		return NULL;
	}

	return pResult;
}
PY_AUTO_MODULE_FUNCTION( RETOWN, startService, ARG( BW::string, END ),
		BigWorld )

/*~ function BigWorld.stopService
 *	@components{ base }
 *
 *	This method stops a service with the given name. If the service is not
 *	already running, an exception is raised.
 *
 *	@param serviceName The name of the service to stop.
 */
bool stopService( const BW::string & serviceName )
{
	BaseApp & app = BaseApp::instance();

	if (!app.isServiceApp())
	{
		PyErr_SetString( PyExc_TypeError,
				"This method is only available on ServiceApps" );
		return false;
	}

	Base * pBase = app.localServiceFragments().findInstanceWithType(
			serviceName.c_str() );

	if (!pBase)
	{
		PyErr_Format( PyExc_ValueError,
				"No such service '%s' running", serviceName.c_str() );

		return false;
	}

	pBase->destroy( /*deleteFromDB*/false, /*writeToDB*/false );

	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, stopService, ARG( BW::string, END ),
		BigWorld )


/*
 *	This class is used to handle the response in BigWorld.authenticateAccount.
 */
class AuthenticateAccountHandler : public Mercury::ReplyMessageHandler
{
public:
	AuthenticateAccountHandler() : deferred_() {}

	PyObject * getDeferredForReturn() const
	{
		PyObject * pObject = deferred_.get();

		if (pObject)
		{
			Py_INCREF( pObject );
		}
		else
		{
			PyErr_SetString( PyExc_SystemError,
					"Failed to create twisted.internet.defer.Deferred." );
		}

		return pObject;
	}

	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * arg )
	{
		uint8 statusByte;
		data >> statusByte;

		LogOnStatus::Status status = (LogOnStatus::Status)statusByte;

		bool isOkay = false;

		if (status == LogOnStatus::LOGGED_ON)
		{
			EntityTypeID typeID;
			DatabaseID dbID;
			data >> typeID >> dbID;

			EntityTypePtr pType = EntityType::getType( typeID );

			MF_ASSERT( pType );

			ScriptObject name = ScriptObject::createFrom( pType->description().name() );
			ScriptObject dbIDObject = ScriptObject::createFrom( dbID );
			ScriptArgs arg = ScriptArgs::create( name, dbIDObject );
			isOkay = deferred_.callback( arg );
		}
		else
		{
			BW::string errorMsg;
			data >> errorMsg;

			switch (status)
			{
				case LogOnStatus::LOGIN_REJECTED_NO_SUCH_USER:
					errorMsg = "No such user";
					break;

				case LogOnStatus::LOGIN_REJECTED_INVALID_PASSWORD:
					errorMsg = "Invalid password";
					break;

				default:
					if (errorMsg.empty())
					{
						BW::stringstream stream;
						stream << "Error status: ";
						stream << int( status );
						errorMsg = stream.str();
					}
					break;
			}

			isOkay = deferred_.errback( "BWAuthenticateError",
					errorMsg.c_str() );
		}

		if (!isOkay)
		{
			WARNING_MSG( "AuthenticateAccountHandler::handleMessage: "
					"Handler failed\n" );
		}

		delete this;
	}

	virtual void handleException( const Mercury::NubException & exception,
			void * args )
	{
		if (!deferred_.mercuryErrback( exception ))
		{
			WARNING_MSG( "AuthenticateAccountHandler::handleException: "
					"Handler failed\n" );
		}

		delete this;
	}

private:
	PyDeferred deferred_;
};


/*~ function BigWorld.authenticateAccount
 *	@components{ base }
 *
 *	This method validates whether a username and password pair are valid. On
 *	success, the entity type and database id of the associated entity are
 *	returned to the Twisted Deferred.
 *
 *	@param username The username to test.
 *	@param password The password to test.
 *
 *	@return A Twisted Deferred. On success, callback will be called with a
 *		2-tuple containing the entity type and database id of the entity
 *		associated with this account. On failure, errback is called with a
 *		Failure object indicating the reason for failure.
 */
PyObject * authenticateAccount( BW::string username, BW::string password )
{
	DBApp & dbApp = BaseApp::instance().dbApp();
	Mercury::Bundle & bundle = dbApp.bundle();

	AuthenticateAccountHandler * pHandler = new AuthenticateAccountHandler();

	bundle.startRequest( DBAppInterface::authenticateAccount, pHandler );
	bundle << username << password;

	dbApp.send();

	return pHandler->getDeferredForReturn();
}
PY_AUTO_MODULE_FUNCTION( RETOWN, authenticateAccount,
		ARG( BW::string, ARG( BW::string, END ) ), BigWorld )


/*~ function BigWorld.shutDownApp
 *	@components{ base }
 *
 *	This method induce a shutdown of this BaseApp.
 */
void shutDownApp()
{
	BaseApp::instance().shutDown();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, shutDownApp, END, BigWorld )


/*~ function BigWorld.controlledShutDown
 *	@components{ base }
 *
 *	This method induces a controlled shutdown of the cluster.
 */
void controlledShutDown()
{
	BaseApp::instance().triggerControlledShutDown();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, controlledShutDown, END, BigWorld )


/*~ function BigWorld.createReplayDataFileWriter
 *	@components{ base }
 *
 *	This method creates a new PyReplayDataFileWriter object. A writer can be
 *	created in creation mode or in recovery mode. 
 *
 *	In recovery mode, the recoveryData keyword-only argument is specified, and
 *	is mutually exclusive with the other keyword-only arguments used for
 *	creation mode.
 *
 *	@param resPath 			The resource path to write out to.
 *	@param privateKey		The private key to use, in PEM string form. Should
 *							only be specified for new files, should be omitted
 *							for recovery.
 *	@param recoveryData		This is a keyword-only argument, to be used in
 *							recovery mode. This parameter is used to supply the
 *							serialised recovery data.
 *	@param shouldOverwrite 	This is an optional keyword-only argument, to be
 *							used in creation mode. If set to True, then any
 *							existing data will be lost. If False (the default),
 *							an error is raised (via the callback) when the
 *							existing file is attempted to be opened.
 *	@param compressionType	This is an optional keyword-only argument, to be
 *							used in creation mode, and specifies the
 *							compression level. Defaults to
 *							BigWorld.BW_COMPRESSION_ZIP_BEST_SPEED.
 *	@param numTicksToSign 	This is an optional keyword-only argument, to be
 *							used in creation mode and specifies the number of
 *							ticks to sign in each signing chunk. Defaults to
 *							10.
 *	@param metaData			This is an optional keyword-only argument, to be
 *							used in creation mode, and specifies a list of
 *							key-value string tuples to be used as meta-data.
 *							This is empty by default.
 *
 *	@return  The new writer object.
 */
PyObject * py_createReplayDataFileWriter( PyObject * args, PyObject * kwargs )
{
	char * resPathString = NULL;
	Py_ssize_t resPathStringLength = 0;
	char * privateKeyString = NULL;
	Py_ssize_t privateKeyStringLength = 0;

	if (!PyArg_ParseTuple( args, "s#s#:BigWorld.createReplayDataFileWriter",
			&resPathString, &resPathStringLength,
			&privateKeyString, &privateKeyStringLength ))
	{
		return NULL;
	}

	if (BaseApp::instance().isShuttingDown())
	{
		PyErr_SetString( PyExc_ValueError,
			"App is shutting down, can't create writer at this time" );
		return NULL;
	}

	BW::string path( resPathString, resPathStringLength );

	if (ReplayDataFileWriter::existsForPath( path ))
	{
		PyErr_Format( PyExc_ValueError,
			"Already writing to path: %s", path.c_str() );
		return NULL;
	}

	// Check the keyword-only parameters. Two sets of keyword arguments,
	// mutually exclusive.

	RecordingRecoveryData recoveryData;
	BW::string recoveryDataString;
	bool haveRecoveryData = false;
	KeywordParser recoverArgs;
	recoverArgs.add( "recoveryData", recoveryDataString );

	KeywordParser::ParseResult parseResult = recoverArgs.parse( kwargs );
	if (parseResult == KeywordParser::EXCEPTION_RAISED)
	{
		return NULL;
	}

	if ((haveRecoveryData = (parseResult == KeywordParser::ALL_FOUND)))
	{
		MemoryIStream recoveryDataStream( recoveryDataString.data(),
			recoveryDataString.size() );

		if (!recoveryData.initFromStream( recoveryDataStream ))
		{
			recoveryDataStream.finish();

			PyErr_Format( PyExc_ValueError, "Invalid recovery data" );
			return NULL;
		}
	}

	static const uint DEFAULT_REPLAY_NUM_TICKS_TO_SIGN = 10;

	bool shouldOverwrite = false;
	int compressionType = BW_COMPRESSION_ZIP_BEST_SPEED;
	unsigned short numTicksToSign = DEFAULT_REPLAY_NUM_TICKS_TO_SIGN;
	PyObject * pMetaDataDict = NULL;

	KeywordParser creationArgs;


	creationArgs.add( "shouldOverwrite", shouldOverwrite );
	creationArgs.add( "compressionType", compressionType );
	creationArgs.add( "numTicksToSign", numTicksToSign );
	creationArgs.add( "metaData", pMetaDataDict );

	if ((parseResult = creationArgs.parse( kwargs )) == 
			KeywordParser::EXCEPTION_RAISED)
	{
		return NULL;
	}
	else if (haveRecoveryData && (parseResult != KeywordParser::NONE_FOUND))
	{
		PyErr_Format( PyExc_ValueError, "Cannot specify "
				"shouldOverwrite, compressionType, numTicksToSign or metaData "
				"with recoveryData" );
		return NULL;
	}
	else if (pMetaDataDict && !PyDict_Check( pMetaDataDict ))
	{
		PyErr_SetString( PyExc_TypeError, "metaData must be a dict" );
		return NULL;
	}

	// Argument parsing will consume the arguments it finds, those left are
	// invalid.

	if (kwargs && PyDict_Size( kwargs ))
	{
		// Just get the first key to complain about.
		PyObject * key = NULL;
		Py_ssize_t i = 0;

		PyDict_Next( kwargs, &i, &key, NULL );

		PyErr_Format( PyExc_TypeError, "Invalid keyword argument: \"%s\"",
			PyString_AsString( key ) );
		return NULL;
	}

	if (privateKeyStringLength <= 0)
	{
		PyErr_SetString( PyExc_ValueError, "Empty private key" );
		return NULL;
	}

	ChecksumSchemePtr pChecksumScheme = ReplayChecksumScheme::create(
		BW::string( privateKeyString, privateKeyStringLength ),
		/* isPrivate */ true );

	if (!pChecksumScheme || !pChecksumScheme->isGood())
	{
		PyErr_Format( PyExc_ValueError, "Invalid EC private key string%s%s",
			(pChecksumScheme && !pChecksumScheme->errorString().empty()) ?
				":" : "",
			(pChecksumScheme && !pChecksumScheme->errorString().empty()) ?
				pChecksumScheme->errorString().c_str() : "" );
		return NULL;
	}

	// Initialise meta-data from Python dict.
	ReplayMetaData metaData;

	metaData.init( pChecksumScheme );

	Py_ssize_t pos = 0;
	PyObject * pKey = NULL;
	PyObject * pValue = NULL;

	while (pMetaDataDict && PyDict_Next( pMetaDataDict, &pos, &pKey, &pValue ))
	{
		BW::string keyString;
		BW::string valueString;

		ScriptString pKeyStringObj( PyObject_Str( pKey ), 
			ScriptString::FROM_NEW_REFERENCE );
		if (!pKeyStringObj.exists())
		{
			return NULL;
		}
		pKeyStringObj.getString( keyString );

		ScriptString pValueStringObj( PyObject_Str( pValue ), 
			ScriptString::FROM_NEW_REFERENCE );
		if (!pValueStringObj.exists())
		{
			return NULL;
		}
		pValueStringObj.getString( valueString );

		metaData.add( keyString, valueString );
	}

	BackgroundFileWriter * pBackgroundFileWriter =
		new BackgroundFileWriter( BaseApp::instance().bgTaskManager() );
	
	BackgroundFilePathPtr pPath = ResourceFilePath::create( path );

	pBackgroundFileWriter->initWithPath( pPath,
			/* shouldOverwrite */ (shouldOverwrite || haveRecoveryData),
			/* shouldTruncate */ shouldOverwrite );

	if (!haveRecoveryData)
	{
		return new PyReplayDataFileWriter( path, pBackgroundFileWriter,
			pChecksumScheme,
			numTicksToSign,
			BWCompressionType( compressionType ),
			EntityType::digest(),
			metaData );
	}
	else
	{
		return new PyReplayDataFileWriter( path, pBackgroundFileWriter,
			pChecksumScheme, recoveryData );
	}
}
PY_MODULE_FUNCTION_WITH_KEYWORDS( createReplayDataFileWriter, BigWorld )


// -----------------------------------------------------------------------------
// Section: Initialisation
// -----------------------------------------------------------------------------

/*~ attribute BigWorld LOG_ON_REJECT
 *	@components{ base }
 *	This constant should be returned by onLogOnAttempt to reject a log on
 *	attempt.
 */
/*~ attribute BigWorld LOG_ON_ACCEPT
 *	@components{ base }
 *	This constant should be returned by onLogOnAttempt to allow the new client
 *	to connect to the Proxy.
 */
/*~ attribute BigWorld LOG_ON_WAIT_FOR_DESTROY
 *	@components{ base }
 *	This constant should be returned by Proxy.onLogOnAttempt if the current
 *	attempt to log on should wait until the current Proxy has logged off. Either
 *	Proxy.destroy or Proxy.destroyCellEntity should be called before this is
 *	returned.
 */
/*~ attribute BigWorld EXPOSE_BASE_SERVICE_APPS
 *	@components{ base }
 *	This flag is used by callable watchers to indicate that it is to be run on
 *	all BaseApp and ServiceApp components owned by the manager.
 */
/*~ attribute BigWorld EXPOSE_BASE_APPS
 *	@components{ base }
 *	This flag is used by callable watchers to indicate that it is to be run on
 *	all BaseApp components owned by the manager.
 */
/*~ attribute BigWorld EXPOSE_SERVICE_APPS
 *	@components{ base }
 *	This flag is used by callable watchers to indicate that it is to be run on
 *	all ServiceApp components owned by the manager.
 */
/*~ attribute BigWorld EXPOSE_LEAST_LOADED
 *	@components{ base }
 *	This flag is used by callable watchers to indicate that it is to be run on
 *	the least loaded BaseApp component owned by the manager.
 */

template <class T>
void addToModule( PyObject * pModule, T value, const char * pName )
{
	PyObject * pObj = Script::getData( value );
	if (pObj)
	{
		PyObject_SetAttrString( pModule, pName, pObj );
		Py_DECREF( pObj );
	}
	else
	{
		PyErr_Clear();
	}
}

} // end of anonymous namespace

namespace BigWorldBaseAppScript
{

bool init( const Bases & bases, const Bases * pServices,
		Mercury::EventDispatcher & dispatcher,
		Mercury::NetworkInterface & intInterface,
		bool isServiceApp )
{
	// Initialise the BigWorld module
	PyObject * pModule = PyImport_AddModule( "BigWorld" );

	if (PyObject_SetAttrString( pModule, "Base",
				reinterpret_cast<PyObject *>(&Base::s_type_) ) == -1 ||
		PyObject_SetAttrString( pModule, "Proxy",
				reinterpret_cast<PyObject *>(&Proxy::s_type_) ) == -1 ||
		PyObject_SetAttrString( pModule, "Service",
				reinterpret_cast<PyObject *>(&Service::s_type_) ) == -1)
	{
		return false;
	}

	if (PyObject_SetAttrString( pModule,
			"UserDataObject",
			reinterpret_cast<PyObject *>(&UserDataObject::s_type_) ) == -1)
	{
		return false;
	}

	addToModule( pModule, LOG_ON_REJECT, "LOG_ON_REJECT" );
	addToModule( pModule, LOG_ON_ACCEPT, "LOG_ON_ACCEPT" );
	addToModule( pModule, LOG_ON_WAIT_FOR_DESTROY, "LOG_ON_WAIT_FOR_DESTROY" );

	// Add the watcher forwarding hints
	addToModule( pModule, ForwardingWatcher::BASE_SERVICE_APPS,
			"EXPOSE_BASE_SERVICE_APPS" );
	addToModule( pModule, ForwardingWatcher::BASE_APPS,
			"EXPOSE_BASE_APPS" );
	addToModule( pModule, ForwardingWatcher::SERVICE_APPS,
			"EXPOSE_SERVICE_APPS" );
	addToModule( pModule, ForwardingWatcher::LEAST_LOADED,
			"EXPOSE_LEAST_LOADED" );

	// TODO: Add the following hints when the BaseAppMgr support is
	//       implemented.
	//addToModule( pModule,
	//		ForwardingWatcher::WITH_ENTITY,  "EXPOSE_WITH_ENTITY" );
	//addToModule( pModule,
	//		ForwardingWatcher::WITH_SPACE,   "EXPOSE_WITH_SPACE" );

	addToModule( pModule,
		LOOK_UP_ENTITIES_FILTER_ALL, "LOOK_UP_ENTITIES_FILTER_ALL" );
	addToModule( pModule,
		LOOK_UP_ENTITIES_FILTER_ONLINE, "LOOK_UP_ENTITIES_FILTER_ONLINE" );
	addToModule( pModule,
		LOOK_UP_ENTITIES_FILTER_OFFLINE, "LOOK_UP_ENTITIES_FILTER_OFFLINE" );

	addToModule( pModule, BW_COMPRESSION_NONE, "COMPRESSION_NONE" );
	addToModule( pModule, BW_COMPRESSION_ZIP_1, "COMPRESSION_ZIP_1" );
	addToModule( pModule, BW_COMPRESSION_ZIP_2, "COMPRESSION_ZIP_2" );
	addToModule( pModule, BW_COMPRESSION_ZIP_3, "COMPRESSION_ZIP_3" );
	addToModule( pModule, BW_COMPRESSION_ZIP_4, "COMPRESSION_ZIP_4" );
	addToModule( pModule, BW_COMPRESSION_ZIP_5, "COMPRESSION_ZIP_5" );
	addToModule( pModule, BW_COMPRESSION_ZIP_6, "COMPRESSION_ZIP_6" );
	addToModule( pModule, BW_COMPRESSION_ZIP_7, "COMPRESSION_ZIP_7" );
	addToModule( pModule, BW_COMPRESSION_ZIP_8, "COMPRESSION_ZIP_8" );
	addToModule( pModule, BW_COMPRESSION_ZIP_9, "COMPRESSION_ZIP_9" );
	addToModule( pModule, BW_COMPRESSION_ZIP_BEST_SPEED,
		"COMPRESSION_ZIP_BEST_SPEED" );
	addToModule( pModule, BW_COMPRESSION_ZIP_BEST_COMPRESSION,
		"COMPRESSION_ZIP_BEST_COMPRESSION" );

	PyObjectPtr pBases( new PyBases( bases ), PyObjectPtr::STEAL_REFERENCE );
	if (PyObject_SetAttrString( pModule, "entities", pBases.get() ) == -1)
	{
		return false;
	}

	if (pServices)
	{
		PyObjectPtr pBases( new PyBases( *pServices ),
				PyObjectPtr::STEAL_REFERENCE );

		if (PyObject_SetAttrString( pModule,
					"localServices", pBases.get() ) == -1)
		{
			return false;
		}
	}

	if (!BaseApp::instance().initPersonality())
	{
		return false;
	}

	// Initialise the entity types
	if (!EntityType::init( false, isServiceApp ))
	{
		return false;
	}

	return true;
}

} // namespace BigWorldBaseAppScript

BW_END_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Python documentation
// -----------------------------------------------------------------------------

/* The following comments reside here because it doesn't seem right to have
 * declaration comments in executable code.
 */
/*~ attribute BigWorld entities
 *  @components{ base }
 *  entities contains a list of all the entities which currently reside on the base.
 *  @type PyBases
 */
/*~ attribute BigWorld services                                                                
 *  @components{ base }                                                                             
 *  services is a map that keys a service name to a corresponding service
 *  fragment on one of the ServiceApps that offer it.
 *  @type PyBases                                                                                   
 */
/*~ attribute BigWorld localServices                                                                
 *  @components{ base }                                                                             
 *  localServices contains a list of all the service fragments which currently reside on the ServiceApp. 
 *  @type PyBases                                                                                   
 */
/*~ attribute BigWorld globalBases
 *	@components{ base }
 *	globalBases is a dictionary mapping strings to bases or base mailboxes.
 *	@type GlobalBases
 */
/*~ attribute BigWorld isServiceApp
 *	@components{ base }
 *	This value indicates whether this process is running as a ServiceApp or a
 *	BaseApp. A ServiceApp is a variation on a BaseApp. It does not accept new
 *	player logins or entities from BigWorld.createBaseAnywhere calls. It is used
 *	to run fragments of a Service.
 *	@type bool
 */
/*~ attribute BigWorld NEXT_ONLY
 *	@components{ base }
 *	This constant is used with the Entity.shouldAutoBackup and
 *	Entity.shouldAutoArchive property. This value indicates that the entity
 *	should be backed up next time it is considered and then this property is
 *	automatically set to False (0).
 */

// script_bigworld.cpp
