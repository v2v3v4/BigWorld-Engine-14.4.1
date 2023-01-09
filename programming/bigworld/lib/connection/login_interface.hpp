#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef LOGIN_INTERFACE_HPP
#endif

#ifndef LOGIN_INTERFACE_HPP
#define LOGIN_INTERFACE_HPP

// -----------------------------------------------------------------------------
// Section: Includes
// -----------------------------------------------------------------------------

// Everything in this section is only parsed once.

#ifndef LOGIN_INTERFACE_HPP_ONCE_ONLY
#define LOGIN_INTERFACE_HPP_ONCE_ONLY

#include "cstdmf/stdmf.hpp"

#include "network/interface_macros.hpp"

// Probe reply is a list of pairs of strings
// Some strings can be interpreted as integers
#define PROBE_KEY_HOST_NAME			"hostName"
#define PROBE_KEY_OWNER_NAME		"ownerName"
#define PROBE_KEY_USERS_COUNT		"usersCount"
// The following strings are not used,
// and comment them for documentation purpose (BWT-8115)
//#define PROBE_KEY_UNIVERSE_NAME	"universeName"
//#define PROBE_KEY_SPACE_NAME		"spaceName"
//#define PROBE_KEY_BINARY_ID		"binaryID"

#endif // LOGIN_INTERFACE_HPP_ONCE_ONLY

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Login Interface
// -----------------------------------------------------------------------------


// gLoginHandler and gProbeHandler below are only defined in 
// loginapp/messsage_handlers.cpp:

// class LoginAppRawMessageHandler;
// extern LoginAppRawMessageHandler gLoginHandler;
// extern LoginAppRawMessageHandler gProbeHandler;

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( LoginInterface )

	// uint32 version
	// bool encrypted
	// LogOnParams
	MERCURY_VARIABLE_MESSAGE( login, 2, &gLoginHandler )

	MERCURY_EMPTY_MESSAGE( probe, &gProbeHandler )

	MERCURY_VARIABLE_MESSAGE( challengeResponse, 2, 
		&gChallengeResponseHandler )

END_MERCURY_INTERFACE()

#pragma pack(pop)

BW_END_NAMESPACE

#endif // LOGIN_INTERFACE_HPP

