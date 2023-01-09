#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef DB_APP_INTERFACE_HPP
#endif

#ifndef DB_APP_INTERFACE_HPP
#define DB_APP_INTERFACE_HPP

// -----------------------------------------------------------------------------
// Section: Includes
// -----------------------------------------------------------------------------
#undef INTERFACE_NAME
#define INTERFACE_NAME DBAppInterface
#include "network/common_interface_macros.hpp"

#include "server/common.hpp"
#include "server/reviver_subject.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DBApp Interface
// -----------------------------------------------------------------------------

BEGIN_MERCURY_INTERFACE( DBAppInterface )

	MF_REVIVER_PING_MSG()

	BW_BEGIN_STRUCT_MSG( DBApp, handleBaseAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBApp, handleDBAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBApp, handleDBAppMgrDeath )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG( DBApp, handleBaseAppDeath )

	BW_BEGIN_STRUCT_MSG( DBApp, shutDown )
		// none
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBApp, controlledShutDown )
		ShutDownStage stage;
	END_STRUCT_MESSAGE()

	// TODO: Scalable DB: Move this to DBAppMgr
	BW_BEGIN_STRUCT_MSG( DBApp, cellAppOverloadStatus )
		bool hasOverloadedCellApps;
	END_STRUCT_MESSAGE()


	BW_STREAM_MSG_EX( DBApp, logOn )
		// BW::string logOnName
		// BW::string password
		// Mercury::Address addrForProxy
		// MD5::Digest digest

	BW_STREAM_MSG_EX( DBApp, authenticateAccount )
		// BW::string username
		// BW::string password

	BW_STREAM_MSG_EX( DBApp, loadEntity )
		// EntityTypeID	entityTypeID;
		// EntityID entityID;
		// bool nameNotID;
		// nameNotID ? (BW::string name) : (DatabaseID id );

	BW_BIG_STREAM_MSG_EX( DBApp, writeEntity )
		// int16 flags; (cell? base? log off?)
		// EntityTypeID entityTypeID;
		// DatabaseID	databaseID;
		// properties

	BW_BEGIN_STRUCT_MSG_EX( DBApp, deleteEntity )
		EntityTypeID	entityTypeID;
		DatabaseID		dbid;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG_EX( DBApp, lookupEntity )
		EntityTypeID	entityTypeID;
		DatabaseID		dbid;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG_EX( DBApp, lookupEntityByName )
		// EntityTypeID		entityTypeID;
		// BW::string 		name;

	BW_STREAM_MSG_EX( DBApp, lookupDBIDByName )
		// EntityTypeID	entityTypeID;
		// BW::string name;

	BW_STREAM_MSG_EX( DBApp, lookupEntities )
		// EntityTypeID entityTypeID
		// BW::string property
		// BW::string value

	BW_BIG_STREAM_MSG_EX( DBApp, executeRawCommand )
		// char[] command;

	BW_STREAM_MSG_EX( DBApp, putIDs )
		// EntityID ids[];

	BW_STREAM_MSG_EX( DBApp, getIDs )
		// int numIDs;

	BW_BIG_STREAM_MSG_EX( DBApp, writeSpaces )

	BW_BEGIN_STRUCT_MSG( DBApp, writeGameTime )
		GameTime gameTime;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG_EX( DBApp, checkStatus )

	BW_STREAM_MSG_EX( DBApp, secondaryDBRegistration );
	BW_STREAM_MSG_EX( DBApp, updateSecondaryDBs );
	BW_STREAM_MSG_EX( DBApp, getSecondaryDBDetails );

	BW_STREAM_MSG( DBApp, updateDBAppHash );

END_MERCURY_INTERFACE()

BW_END_NAMESPACE

#endif // DB_APP_INTERFACE_HPP
