#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef BASEAPP_INT_INTERFACE_HPP
#endif

#ifndef BASEAPP_INT_INTERFACE_HPP
#define BASEAPP_INT_INTERFACE_HPP

#include "network/basictypes.hpp"
#include "network/network_interface.hpp"

#include "server/anonymous_channel_client.hpp"

#undef INTERFACE_NAME
#define INTERFACE_NAME BaseAppIntInterface
#include "connection/common_baseapp_interface.hpp"


// For Base entity
#define MF_BEGIN_BASE_STRUCT_MSG_EX( NAME ) 								\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME, 									\
			BaseMessageStructHandlerEx< INTERFACE_NAME::NAME##Args >, 		\
			&Base::NAME )

#define MF_BASE_STREAM_MSG( NAME ) 											\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,								\
			BaseMessageStreamHandler, 										\
			&Base::NAME )

#define MF_BASE_STREAM_MSG_EX( NAME ) 										\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,								\
			BaseMessageStreamHandlerEx, 									\
			&Base::NAME )


// -----------------------------------------------------------------------------
// Section: Includes
// -----------------------------------------------------------------------------

#include "server/common.hpp"
#include "network/msgtypes.hpp"


BW_BEGIN_NAMESPACE

#ifndef BASEAPP_INT_INTERFACE_HPP_ONCE
#define BASEAPP_INT_INTERFACE_HPP_ONCE

// These constants apply to the BaseAppIntInterface::logOnAttempt message.
namespace BaseAppIntInterface
{
	const uint8 LOG_ON_ATTEMPT_REJECTED = 0;
	const uint8 LOG_ON_ATTEMPT_TOOK_CONTROL = 1;
	const uint8 LOG_ON_ATTEMPT_WAIT_FOR_DESTROY = 3;
}

#endif // BASEAPP_INT_INTERFACE_HPP_ONCE

// -----------------------------------------------------------------------------
// Section: Proxy Internal Interface
// -----------------------------------------------------------------------------

#pragma pack(push, 1)
BEGIN_MERCURY_INTERFACE( BaseAppIntInterface )

	BW_STREAM_MSG_EX( BaseApp, createBaseWithCellData )

	BW_STREAM_MSG_EX( BaseApp, createBaseFromDB )

	BW_STREAM_MSG_EX( BaseApp, createBaseFromTemplate )
	// EntityID			id
	// EntityTypeID		typeID
	// BW::string		templateID

	BW_STREAM_MSG_EX( BaseApp, logOnAttempt )

	BW_STREAM_MSG( BaseApp, addGlobalBase )
	BW_STREAM_MSG( BaseApp, delGlobalBase )

	BW_STREAM_MSG( BaseApp, addServiceFragment )
	BW_STREAM_MSG( BaseApp, delServiceFragment )

	BW_BEGIN_STRUCT_MSG( BaseApp, handleCellAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( BaseApp, handleBaseAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG( BaseApp, handleCellAppDeath )

	BW_BEGIN_STRUCT_MSG( BaseApp, startup )
		bool				bootstrap;
		bool				didAutoLoadEntitiesFromDB;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( BaseApp, shutDown )
		bool	isSigInt;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG_EX( BaseApp, controlledShutDown )

	BW_STREAM_MSG( BaseApp, startOffloading )

	BW_STREAM_MSG( BaseApp, setCreateBaseInfo )

	// *** messages for the new style BaseApp backup. Base entities are now
	// backed up individually ***
	BW_BEGIN_STRUCT_MSG( BaseApp, startBaseEntitiesBackup )
		Mercury::Address	realBaseAppAddr;
		uint32				index;
		uint32				hashSize;
		uint32				prime;
		bool				isInitial;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( BaseApp, stopBaseEntitiesBackup )
		Mercury::Address	realBaseAppAddr;
		uint32				index;
		uint32				hashSize;
		uint32				prime;
		bool				isPending;
	END_STRUCT_MESSAGE()

	BW_BIG_STREAM_MSG_EX( BaseApp, backupBaseEntity )
	BW_STREAM_MSG( BaseApp, ackOffloadBackup )

	BW_BEGIN_STRUCT_MSG_EX( BaseApp, stopBaseEntityBackup )
		EntityID entityID;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG_EX( BaseApp, handleBaseAppBirth )
	BW_STREAM_MSG_EX( BaseApp, handleBaseAppDeath )

	// Concerned with backing up this app. Other messages are concerned with
	// backing up others.
	BW_STREAM_MSG_EX( BaseApp, setBackupBaseApps )

	BW_STREAM_MSG( BaseApp, updateDBAppHash )

	// *** messages related to shared data ***
	BW_STREAM_MSG( BaseApp, setSharedData )

	BW_STREAM_MSG( BaseApp, delSharedData )

	// *** messages from the cell concerning the client ***

	// identify the client that future messages
	//  (in this bundle) are destined for.
	BW_BEGIN_STRUCT_MSG( BaseApp, setClient )
		EntityID			id;
	END_STRUCT_MESSAGE()

	// set the cell that owns this base
	MF_BEGIN_BASE_STRUCT_MSG_EX( currentCell )
		SpaceID				newSpaceID;
		Mercury::Address	newCellAddr;
	END_STRUCT_MESSAGE()

	MF_BEGIN_BASE_STRUCT_MSG_EX( teleportOther )
		EntityMailBoxRef cellMailBox;
	END_STRUCT_MESSAGE()

	// set the cell that owns this base, in an emergency. This will be called by
	// an old ghost.
	BW_STREAM_MSG_EX( BaseApp, emergencySetCurrentCell )
	//	SpaceID				newSpaceID;
	//	Mercury::Address	newCellAddr;

	BW_STREAM_MSG( BaseApp, forwardedBaseMessage )

	MF_EMPTY_PROXY_MSG( sendToClient )

	// Start of messages to forward from cell to client...
	MF_VARLEN_PROXY_MSG( createCellPlayer )

	MF_VARLEN_PROXY_MSG( spaceData )
	//	EntityID		spaceID
	//	SpaceEntryID	entryID
	//	uint16			key;
	//	char[]			value;		// rest of message

	MF_BEGIN_PROXY_MSG( enterAoI )
		EntityID			id;
		IDAlias				idAlias;
	END_STRUCT_MESSAGE()

	MF_BEGIN_PROXY_MSG( enterAoIOnVehicle )
		EntityID			id;
		EntityID			vehicleID;
		IDAlias				idAlias;
	END_STRUCT_MESSAGE()

	MF_VARLEN_PROXY_MSG( leaveAoI )
	//	EntityID		id;
	//	EventNumber[]	lastEventNumbers;	// rest

	MF_VARLEN_PROXY_MSG( createEntity )

	MF_VARLEN_PROXY_MSG( createEntityDetailed )

	MF_VARLEN_PROXY_MSG( updateEntity )


	// The interface that is shared between this interface and the client
	// interface. This includes messages such as all of the avatarUpdate
	// messages.
#define MF_EMPTY_COMMON_RELIABLE_MSG MF_EMPTY_PROXY_MSG
#define MF_BEGIN_COMMON_RELIABLE_MSG MF_BEGIN_PROXY_MSG
#define MF_BEGIN_COMMON_PASSENGER_MSG MF_BEGIN_PROXY_MSG
#define MF_BEGIN_COMMON_UNRELIABLE_MSG MF_BEGIN_PROXY_MSG
#include "connection/common_client_interface.hpp"
#undef MF_EMPTY_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_PASSENGER_MSG
#undef MF_BEGIN_COMMON_UNRELIABLE_MSG

	BW_BEGIN_STRUCT_MSG( BaseApp, makeNextMessageReliable )
		bool			isReliable;
	END_STRUCT_MESSAGE()

	MF_BEGIN_PROXY_MSG( modWard )
		EntityID		id;
		bool			on;
	END_STRUCT_MESSAGE()


	MF_VARLEN_PROXY_MSG( callClientMethod )
	// ... end of messages to forward from cell to client.

	BW_STREAM_MSG_EX( BaseApp, acceptClient )

	// *** Message from the cell not concerning the client ***

	// Messages for proxies

	MF_BASE_STREAM_MSG( backupCellEntity )

	MF_BASE_STREAM_MSG( writeToDB )
	MF_BASE_STREAM_MSG_EX( cellEntityLost )

	/**
	 *	This message is used to signal to the base that it should be kept alive
	 *	for at least the specified interval, even if the client disconnects or
	 *	is not connected.
	 */
	MF_BEGIN_BASE_STRUCT_MSG_EX( startKeepAlive )
		uint32			interval; // in seconds
	END_STRUCT_MESSAGE()

	MF_BASE_STREAM_MSG_EX( callBaseMethod )

	MF_BASE_STREAM_MSG_EX( callCellMethod )

	MF_BASE_STREAM_MSG_EX( getCellAddr )

	MF_VARLEN_PROXY_MSG( sendMessageToClient )
	MF_VARLEN_PROXY_MSG( sendMessageToClientUnreliable )

	// -------------------------------------------------------------------------
	// Watcher messages
	// -------------------------------------------------------------------------

	// Message to forward watcher requests via
	BW_STREAM_MSG_EX( BaseApp, callWatcher )

END_MERCURY_INTERFACE()

#pragma pack( pop )

BW_END_NAMESPACE

#endif // BASEAPP_INT_INTERFACE_HPP
