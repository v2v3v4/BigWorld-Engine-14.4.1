#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef TEST_INTERFACE_HPP
#endif

#ifndef TEST_INTERFACE_HPP
#define TEST_INTERFACE_HPP

#include "network/udp_channel.hpp"
#include "network/interface_macros.hpp"

#define BW_COMMON_MSG( NAME, TYPE )										\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,									\
		TYPE##StructMessageHandler< TYPE##Interface::NAME##Args >,		\
		&Channel##TYPE##App::NAME )										\

#define BW_SERVER_MSG( NAME ) BW_COMMON_MSG( NAME, Server )
#define BW_CLIENT_MSG( NAME ) BW_COMMON_MSG( NAME, Client )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interior interface
// -----------------------------------------------------------------------------

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( ServerInterface )

	BW_SERVER_MSG( msg1 )
		Mercury::UDPChannel::Traits traits;
		uint32	seq;
		uint32	data;
	END_STRUCT_MESSAGE()

	BW_SERVER_MSG( disconnect )
		uint32 seq;
	END_STRUCT_MESSAGE()

END_MERCURY_INTERFACE()

BEGIN_MERCURY_INTERFACE( ClientInterface )

	BW_CLIENT_MSG( msg1 )
		uint32	seq;
		uint32	data;
	END_STRUCT_MESSAGE()

END_MERCURY_INTERFACE()


#pragma pack(pop)

#undef BW_COMMON_MSG
#undef BW_SERVER_MSG
#undef BW_CLIENT_MSG

BW_END_NAMESPACE

#endif // TEST_INTERFACE_HPP
