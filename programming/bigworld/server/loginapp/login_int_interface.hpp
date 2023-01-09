#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef LOGIN_INT_INTERFACE_HPP
#endif

#ifndef LOGIN_INT_INTERFACE_HPP
#define LOGIN_INT_INTERFACE_HPP

#include "network/basictypes.hpp"

#undef INTERFACE_NAME
#define INTERFACE_NAME LoginIntInterface
#include "network/common_interface_macros.hpp"

#include "server/common.hpp"
#include "server/anonymous_channel_client.hpp"

#ifdef MF_SERVER
#include "server/reviver_subject.hpp"
#else
#define MF_REVIVER_PING_MSG()
#endif


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interior interface
// -----------------------------------------------------------------------------

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( LoginIntInterface )

	BW_ANONYMOUS_CHANNEL_CLIENT_MSG( DBAppMgrInterface )

	MERCURY_EMPTY_MESSAGE( controlledShutDown, &gShutDownHandler )

	BW_BEGIN_STRUCT_MSG( LoginApp, handleDBAppMgrBirth )
		Mercury::Address	addr;
	END_STRUCT_MESSAGE()

	BW_BEGIN_STRUCT_MSG( LoginApp, notifyDBAppAlpha )
		Mercury::Address addr;
	END_STRUCT_MESSAGE()

	MF_REVIVER_PING_MSG()

END_MERCURY_INTERFACE()
#pragma pack(pop)

BW_END_NAMESPACE

#endif // LOGIN__INT_INTERFACE_HPP

