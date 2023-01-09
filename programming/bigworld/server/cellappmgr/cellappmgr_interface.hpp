#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef CELL_APP_MGR_INTERFACE_HPP
#endif

#ifndef CELL_APP_MGR_INTERFACE_HPP
#define CELL_APP_MGR_INTERFACE_HPP


#include "network/basictypes.hpp"

#undef INTERFACE_NAME
#define INTERFACE_NAME CellAppMgrInterface
#include "network/common_interface_macros.hpp"

#include "server/anonymous_channel_client.hpp"
#include "server/common.hpp"
#include "server/reviver_subject.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Cell App Manager interface
// -----------------------------------------------------------------------------

#pragma pack(push, 1)
BEGIN_MERCURY_INTERFACE( CellAppMgrInterface )

	// The arguments are as follows:
	//	SpaceID		The space to create the entity in
	//	A stream suitable for CellAppInterface::createEntity
	//		We rely on being able to read the struct StreamHelper::AddEntityData
	BW_STREAM_MSG_EX( CellAppMgr, createEntity )
	// The arguments are as follows:
	//	bool		true if it is preferred that the new space
	//				be created on a CellApp on the same machine
	//				as the BaseApp of this entity
	//	A stream suitable for CellAppInterface::createEntity
	//		We rely on being able to read the struct StreamHelper::AddEntityData
	BW_STREAM_MSG_EX( CellAppMgr, createEntityInNewSpace )

	BW_STREAM_MSG_EX( CellAppMgr, prepareForRestoreFromDB )

	BW_STREAM_MSG_EX( CellAppMgr, startup )

	BW_BEGIN_STRUCT_MSG( CellAppMgr, shutDown )
		bool isSigInt;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, controlledShutDown )
		ShutDownStage stage;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, shouldOffload )
		bool enable;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG_EX( CellAppMgr, addApp );

	BW_STREAM_MSG( CellAppMgr, recoverCellApp );

	BW_BEGIN_STRUCT_MSG( CellAppMgr, delApp )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, setBaseApp )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, handleCellAppMgrBirth )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, handleBaseAppMgrBirth )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG_EX( CellAppMgr, handleCellAppDeath )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG( CellAppMgr, handleBaseAppDeath )

	BW_BEGIN_STRUCT_MSG_EX( CellAppMgr, ackCellAppDeath )
		Mercury::Address deadAddr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, handleDBAppMgrBirth )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, setDBAppAlpha )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG_EX( CellAppMgr, gameTimeReading )
		double				gameTimeReadingContribution;
	END_STRUCT_MESSAGE()	// double is good for ~100 000 years

	// These could be a space messages
	BW_STREAM_MSG_EX( CellAppMgr, updateSpaceData )

	BW_BEGIN_STRUCT_MSG( CellAppMgr, shutDownSpace )
		SpaceID spaceID;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( CellAppMgr, ackBaseAppsShutDown )
		ShutDownStage stage;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG_EX( CellAppMgr, checkStatus )

	// ---- Cell App messages ----
	BW_BEGIN_STRUCT_MSG( CellApp, informOfLoad )
		float load;
		int	numEntities;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG( CellApp, updateBounds );

	BW_EMPTY_MSG( CellApp, retireApp )

	BW_BEGIN_STRUCT_MSG( CellApp, ackCellAppShutDown )
		ShutDownStage stage;
	END_STRUCT_MESSAGE()

	MF_REVIVER_PING_MSG()

	BW_STREAM_MSG_EX( CellAppMgr, setSharedData )
	BW_STREAM_MSG_EX( CellAppMgr, delSharedData )

	BW_STREAM_MSG( CellAppMgr, addServiceFragment )
	BW_STREAM_MSG( CellAppMgr, delServiceFragment )

END_MERCURY_INTERFACE()
#pragma pack(pop)

BW_END_NAMESPACE

#endif // CELL_APP_MGR_INTERFACE_HPP
