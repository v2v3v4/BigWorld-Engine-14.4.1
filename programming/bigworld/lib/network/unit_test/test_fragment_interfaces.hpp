#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef TEST_FRAGMENT_INTERFACE_HPP
#endif

#ifndef TEST_FRAGMENT_INTERFACE_HPP
#define TEST_FRAGMENT_INTERFACE_HPP

#include "network/channel.hpp"
#include "network/interface_macros.hpp"

#define BW_VARLEN_MSG( NAME )											\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,							\
		VarLenMessageHandler< FragmentServerApp >,						\
		&FragmentServerApp::NAME )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interior interface
// -----------------------------------------------------------------------------

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( FragmentServerInterface )

	BW_VARLEN_MSG( connect )

	BW_VARLEN_MSG( disconnect )

	BW_VARLEN_MSG( channelMsg )

	BW_VARLEN_MSG( onceOffMsg )

END_MERCURY_INTERFACE()

#pragma pack(pop)

BW_END_NAMESPACE

#undef BW_VARLEN_MSG
#endif // TEST_FRAGMENT_INTERFACE_HPP
