#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef DB_APP_MGR_INTERFACE_HPP
#endif

#ifndef DB_APP_MGR_INTERFACE_HPP
#define DB_APP_MGR_INTERFACE_HPP


#include "network/basictypes.hpp"

#undef INTERFACE_NAME
#define INTERFACE_NAME DBAppMgrInterface
#include "network/common_interface_macros.hpp"

#include "server/common.hpp"
#include "server/reviver_subject.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DB App Manager interface
// -----------------------------------------------------------------------------

#pragma pack(push, 1)
BEGIN_MERCURY_INTERFACE( DBAppMgrInterface )

	BW_BEGIN_STRUCT_MSG_EX( DBAppMgr, handleDBAppDeath )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBAppMgr, handleLoginAppDeath )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBAppMgr, handleDBAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBAppMgr, handleBaseAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBAppMgr, handleCellAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( DBAppMgr, controlledShutDown )
		ShutDownStage stage;
	END_STRUCT_MESSAGE()

	BW_EMPTY_MSG_EX( DBAppMgr, addDBApp )

	BW_BEGIN_STRUCT_MSG_EX( DBAppMgr, recoverDBApp )
		DBAppID id;
		// More to come in later phases.
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG_EX( DBAppMgr, addLoginApp );

	BW_BEGIN_STRUCT_MSG_EX( DBAppMgr, recoverLoginApp );
		LoginAppID	id;
	END_STRUCT_MESSAGE()

	BW_STREAM_MSG( DBAppMgr, handleBaseAppDeath )

	BW_EMPTY_MSG( DBApp, retireApp )

	BW_EMPTY_MSG( DBAppMgr, serverHasStarted )

	MF_REVIVER_PING_MSG()

END_MERCURY_INTERFACE()
#pragma pack(pop)

BW_END_NAMESPACE

#endif // DB_APP_MGR_INTERFACE_HPP
