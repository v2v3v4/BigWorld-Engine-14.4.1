#include "pch.hpp"

#include "entity.hpp"
#include "entity_type.hpp"
#include "py_entities.hpp"

#include "connection/condemned_interfaces.hpp"
#include "connection/log_on_status.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection/server_connection_handler.hpp"

#include "connection_model/bw_connection_helper.hpp"
#include "connection_model/bw_entity_factory.hpp"
#include "connection_model/bw_server_connection.hpp"
#include "connection_model/simple_space_data_storage.hpp"

#include "entitydef/constants.hpp"
#include "entitydef/script_data_source.hpp"

#include "pyscript/py_import_paths.hpp"
#include "pyscript/script.hpp"

#include "resmgr/bwresource.hpp"

#include "cstdmf/singleton.hpp"

#include <iostream>
#include <memory>

#ifdef _WIN32
#include <Winsock2.h>
#include <conio.h>
#else // ! ifdef _WIN32
#include <signal.h>
#endif // ifdef _WIN32

DECLARE_DEBUG_COMPONENT2( "Eg", 0 )




/**
 *	This class provides a very simple implementation of the required
 *	BWEntityFactory interface. This factory is used by the BWConnection
 *	instance to create entity instances.
 */
class MyEntityFactory : public BW::BWEntityFactory
{
public:
	MyEntityFactory(): BWEntityFactory() {}

	virtual ~MyEntityFactory() {}

	// Override from BWEntityFactory.
	virtual BW::BWEntity * doCreate( BW::EntityTypeID entityTypeID,
			BW::BWConnection * pBWConnection );
};


/**
 *	This method is an override from BWEntityFactory. It simply creates a new
 *	MyEntity instance.
 *
 *	@param entityTypeID 	The entity type ID.
 */
BW::BWEntity * MyEntityFactory::doCreate( BW::EntityTypeID entityTypeID,
		BW::BWConnection * pBWConnection )
{
	EntityType * pEntityType = EntityType::find( entityTypeID );

	if (pEntityType == NULL)
	{
		return NULL;
	}

	Entity * pEntity = new Entity( *pEntityType, pBWConnection );

	INFO_MSG( "MyEntityFactory::doCreate: %s\n",
		pEntity->entityTypeName().c_str() );

	return pEntity;
}


/**
 *	This class implements the ServerConnectionHandler interface, for receiving
 *	logon and logoff events.
 */
class MyConnectionHandler : public BW::ServerConnectionHandler
{
public:
	MyConnectionHandler() :
		ServerConnectionHandler(),
		errorMessage_()
	{}

	virtual ~MyConnectionHandler() {}

	const BW::string & logOnFailureMessage() const { return errorMessage_; }

	// Overrides from ServerConnectionHandler.
	virtual void onLoggedOff()
	{}


	virtual void onLoggedOn()
	{
		errorMessage_.clear();
	}

	virtual void onLogOnFailure( const BW::LogOnStatus & status,
		const BW::string & message )
	{
		errorMessage_ = message;
	}


private:
	BW::string errorMessage_;
};


BW_USE_NAMESPACE

/**
 *	This class implements a simple wrapper for a BW::ServerConnection to
 *	simplify the server connection's creation and destruction.
 */
class SimplePythonClient : public Singleton< SimplePythonClient >
{
public:
	SimplePythonClient( const BW::string & serverName,
			const BW::string & userName, const BW::string & password,
			bool autoDisconnect = false ) :
		pSpaceDataStorage_( new SimpleSpaceDataStorage(
				new SpaceDataMappings() ) ),
		pConnectionHandler_( new MyConnectionHandler() ),
		pConnectionHelper_( NULL ),
		pConnection_( NULL ),
		serverName_( serverName ),
		userName_( userName ),
		password_( password ),
		autoDisconnect_( autoDisconnect ),
		wasOnline_( false ),
		shouldExit_( false )
	{
	}

	~SimplePythonClient()
	{
		delete pConnection_;
		delete pConnectionHelper_;
		delete pSpaceDataStorage_;
		delete pConnectionHandler_;
	}

	bool login()
	{
		return pConnection_->logOnTo( serverName_, userName_, password_ );
	}

	void logout()
	{
		pConnection_->logOff();
	}

	void exit()
	{
		shouldExit_ = true;
	}

	void run()
	{
#if defined( _WIN32 ) && !defined( _XBOX )
			std::cout << "Press 'q' to quit" << std::endl;
#else
			std::cout << "Press Ctrl-C to quit" << std::endl;
#endif
		
			std::cout << "Server is " << serverName_ << std::endl;
			std::cout << "Username is " << userName_ << std::endl;

	
#if ! defined( _WIN32 )
			signal( SIGINT, &SimplePythonClient::onSigInt );
#endif // ! defined( _WIN32 )
		
		while (!this->exitCondition())
		{
			this->tick();
		}
	}

	bool init( int argc, char ** argv )
	{
		// 1. Set up the resource directories so we can find the LoginApp
		// public key.
		BWResource bwResource;
		BWResource::init( argc, (const char**)argv );
	
		PyImportPaths importPaths;
	
		importPaths.addResPath( EntityDef::Constants::entitiesClientPath() );
	
		if (!Script::init( importPaths ))
		{
			std::cerr << "Could not initialise Python." << std::endl;
			return false;
		}
	
		// 2. Initialise entity types 
		if (!EntityType::init( EntityDef::Constants::entitiesFile() ))
		{
			return false;
		}
	
		// 3. Create connection helper and server connection
		pConnectionHelper_ = new BWConnectionHelper( entityFactory_,
			* pSpaceDataStorage_, EntityType::entityDefConstants() );

		pConnection_ = pConnectionHelper_->createServerConnection( 0.0 );

		MF_ASSERT( pConnection_ );
		pConnection_->setHandler( pConnectionHandler_ );

		// 4. Setup the public key to encrypt client / server communication
		#if defined( PLAYSTATION3 ) || defined( _XBOX360 )
		if (!pConnection_->initWithKeyString( g_loginAppPublicKey ))
		#else

		BW::string loginAppPublicKeyPath = "loginapp.pubkey";

		if ((IFileSystem::FT_FILE != 
				bwResource.resolveToAbsolutePath( loginAppPublicKeyPath )) ||
			(!pConnection_->setLoginAppPublicKeyPath( loginAppPublicKeyPath )))
		#endif
		{
			std::cerr << "Failed to initialise LoginApp public key." <<
				std::endl;
			return false;
		}

		ScriptModule bigWorld = ScriptModule::import( "BigWorld",
			ScriptErrorPrint( "Failed to import BigWorld module" ) );

		if (!bigWorld)
		{
			return false;
		}

		bigWorld.setAttribute( "entities",
			ScriptObject( new PyEntities( pConnection_->entities() ),
				ScriptObject::FROM_NEW_REFERENCE ),
			ScriptErrorPrint() );

		return true;
	}

	BWServerConnection * pConnection()
	{
		return pConnection_;
	}

private:

	void tick()
	{
		const float TICK_TIME = 1 / 20.f;

		this->sleep( TICK_TIME );
		pConnectionHelper_->update( pConnection_, TICK_TIME );

		if (!pConnectionHandler_->logOnFailureMessage().empty())
		{
			std::cout << "Login failed: " << 
				pConnectionHandler_->logOnFailureMessage() <<
				std::endl;
			return;
		}

		if (!wasOnline_ && pConnection_->isOnline())
		{
			wasOnline_ = true;
			std::cout << "Connected to " <<
				pConnection_->serverAddress().c_str() <<
				std::endl;

			if (autoDisconnect_)
			{
				shouldExit_ = true;
			}
		}

		if (wasOnline_ && !pConnection_->isOnline())
		{
			std::cout << "Disconnected." << std::endl;
			shouldExit_ = true;
		}

		pConnectionHelper_->updateServer( pConnection_ );
	}

	/**
	 *	This function sleeps for a given amount of time.
	 *
	 *	@param seconds	Duration to sleep in seconds.
	 */
	void sleep( double seconds )
	{
#ifdef _WIN32
		Sleep( int( seconds * 1000.0 ) );
#else
		usleep( int( seconds * 1000000.0 ) );
#endif
	}

	bool exitCondition()
	{
#if defined( _WIN32 ) && !defined( _XBOX )
		while (_kbhit() && !shouldExit_)
		{
			char key = _getch();
			if (key == 'q' || key == 'Q')
			{
				this->exit();
			}
		}
#endif
		return shouldExit_;
	}

	static void onSigInt( int signum )
	{
		SimplePythonClient::instance().exit();
	}



	MyEntityFactory entityFactory_;
	SimpleSpaceDataStorage * pSpaceDataStorage_;
	MyConnectionHandler * pConnectionHandler_;
	BWConnectionHelper * pConnectionHelper_;
	BWServerConnection * pConnection_;

	const BW::string & serverName_;
	const BW::string & userName_;
	const BW::string & password_;
	const bool & autoDisconnect_;
	bool wasOnline_;
	bool shouldExit_;
};

BW_SINGLETON_STORAGE( SimplePythonClient )

BW_BEGIN_NAMESPACE
/*~ function BigWorld createEntity
 *  Creates a new entity on the client and places it in the world. The
 *	resulting entity will have no base or cell part.
 *
 *  @param className	The string name of the entity to instantiate.
 *	@param spaceID		The id of the space in which to place the entity.
 *	@param vehicleID	The id of the vehicle on which to place the entity
 *						(0 for no vehicle).
 *  @param position		A Vector3 containing the position at which the new
 *						entity is to be spawned.
 *  @param direction	A Vector3 containing the initial orientation of the new
 *						entity (roll, pitch, yaw).
 *  @param state		A dictionary describing the initial states of the
 *						entity's properties (as described in the entity's .def
 *						file). A property will take on it's default value if it
 *						is not listed here.
 *  @return				The ID of the new entity, as an integer.
 *
 *  Example:
 *  @{
 *  # create an arrow style Info entity at the same position as the player
 *  p = BigWorld.player()
 *  direction = ( 0, 0, p.yaw )
 *  properties = { 'modelType':2, 'text':'Created Info Entity'}
 *  BigWorld.createEntity( 'Info', p.spaceID, 0, p.position, direction, properties )
 *  @}
 */
/**
 *	Creates a new client-side entity
 */
static PyObject * createEntity( const BW::string & className,
		SpaceID spaceID,
		EntityID vehicleID,
		const Position3D & position,
		const Vector3 & vecDirection,
		const ScriptDict & dict )
{
	Direction3D direction( vecDirection );
	// Now find the index of this class
	EntityType * pType = EntityType::find( className );
	if (pType == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.createEntity: "
			"Class %s is not an entity class", className.c_str() );
		return NULL;
	}

	// fake up a creation stream of tagged cell data
	// This is also how the CellApp does it.
	// See EntityCache::addChangedProperties
	MemoryOStream	stream( 4096 );
	uint8 * pNumProps = (uint8*)stream.reserve( sizeof(uint8) );
	*pNumProps = 0;

	uint8 nprops = (uint8)pType->description().clientServerPropertyCount();
	for (uint8 i = 0; i < nprops; i++)
	{
		const DataDescription * pDD = pType->description().clientServerProperty( i );
		// CellApp doesn't check this...
		if (!pDD->isOtherClientData()) continue;

		ScriptObject pVal;
		if (dict)
		{
			pVal = dict.getItem( pDD->name().c_str(), ScriptErrorClear() );
		}

		if (!pVal)
		{
			pVal = pDD->pInitialValue();
		}

		stream << i;
		ScriptDataSource source ( pVal );
		pDD->addToStream( source, stream, /* isPersistentOnly */ false );

		(*pNumProps)++;
	}

	// Tell the EntityManager all we know about this entity
	BWServerConnection * pConn = SimplePythonClient::instance().pConnection();
	BWEntity * pEntity = pConn->createLocalEntity(
			pType->description().clientIndex(), spaceID, vehicleID, position,
			direction, stream );

	if (pEntity == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.createEntity: "
			"Could not create entity of type %s", className.c_str() );
		return NULL;
	}

	return Script::getData( pEntity->entityID() );
}

PY_AUTO_MODULE_FUNCTION( RETOWN, createEntity,
		ARG( BW::string, ARG( SpaceID, ARG( EntityID, ARG( Position3D, ARG( Vector3,
				OPTARG( ScriptDict, ScriptDict(), END ) ) ) ) ) ),
				BigWorld );


/*~ function BigWorld destroyEntity
 *  Destroys an exiting client-side entity.
 *  @param id The id of the entity to destroy.
 *
 *  Example:
 *  @{
 *  id = BigWorld.target().id # get current target ID
 *  BigWorld.destroyEntity( id )
 *  @}
 */
/**
 *	Destroys an existing client-side entity
 */
static bool destroyEntity( EntityID id )
{
	BW_GUARD;

	BWServerConnection * pConn = SimplePythonClient::instance().pConnection();
	return pConn->destroyLocalEntity( id );
}
PY_AUTO_MODULE_FUNCTION( RETDATA, destroyEntity,
		ARG( EntityID,
			END ), BigWorld );
BW_END_NAMESPACE


void printUsage( const char * argv0 )
{
	static const char USAGE_STRING[] = "Usage: %s -server <server> "
		"-user <user> -password <password> [-autoDisconnect]\n";

	printf( USAGE_STRING, argv0 );
}



/**
 *	Main function.
 */
int main( int argc, char * argv[] )
{
	BW::string serverName;
	BW::string userName;
	BW::string password = "pass";
	bool autoDisconnect = false;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp( "-server", argv[i] ) == 0)
		{
			serverName = argv[ ++i ];
		}
		else if (strcmp( "-user", argv[i] ) == 0)
		{
			userName = argv[ ++i ];
		}
		else if (strcmp( "-password", argv[i] ) == 0)
		{
			password = argv[ ++i ];
		}
		else if (strcmp( "-autoDisconnect", argv[i] ) == 0)
		{
			autoDisconnect = true;
		}
		else if (strcmp( "-help", argv[i] ) == 0)
		{
			printUsage( argv[0] );
			return EXIT_SUCCESS;
		}
	}

	if (serverName.empty())
	{
		std::cout << "No server name specified" << std::endl;
		return EXIT_FAILURE;
	}

	if (userName.empty())
	{
		std::cout << "No username specified" << std::endl;
		return EXIT_FAILURE;
	}

	SimplePythonClient client( serverName, userName, password,
			autoDisconnect );

	if (!client.init( argc, argv ))
	{
		return EXIT_FAILURE;
	}

	if (!client.login())
	{
		std::cout << "Login failed, check your server name." << std::endl;
		return EXIT_FAILURE;
	}

	client.run();

	client.logout();

	return EXIT_SUCCESS;
}


// main.cpp

