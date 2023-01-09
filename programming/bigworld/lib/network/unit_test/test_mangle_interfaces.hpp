#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef TEST_MANGLE_INTERFACE_HPP
#endif

#ifndef TEST_MANGLE_INTERFACE_HPP
#define TEST_MANGLE_INTERFACE_HPP

#include "network/interface_macros.hpp"
#include "network/udp_channel.hpp"

#define BW_COMMON_MSG( NAME, TYPE )										\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,									\
		TYPE##MangleStructMessageHandler<								\
			Mangle##TYPE##Interface::NAME##Args >,						\
		&Mangle##TYPE##App::NAME )										\

#define BW_SERVER_MSG( NAME ) BW_COMMON_MSG( NAME, Server )
#define BW_CLIENT_MSG( NAME ) BW_COMMON_MSG( NAME, Client )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interior interface
// -----------------------------------------------------------------------------

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( MangleServerInterface )

	BW_SERVER_MSG( connect )
		Mercury::UDPChannel::Traits traits;
		uint32 magic;
	END_STRUCT_MESSAGE()

	BW_SERVER_MSG( msg1 )
		uint32	magic;
	END_STRUCT_MESSAGE()

	BW_SERVER_MSG( msg2 )
		uint8	magic;
	END_STRUCT_MESSAGE()

END_MERCURY_INTERFACE()

BEGIN_MERCURY_INTERFACE( MangleClientInterface )

	BW_CLIENT_MSG( ackConnect )
		uint32 magic;
	END_STRUCT_MESSAGE()

END_MERCURY_INTERFACE()


#pragma pack(pop)

BW_END_NAMESPACE

#undef BW_COMMON_MSG
#undef BW_SERVER_MSG
#undef BW_CLIENT_MSG

#endif // TEST_MANGLE_INTERFACE_HPP
