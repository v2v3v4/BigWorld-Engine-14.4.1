
#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef TEST_TCP_CHANNELS_INTERFACES_HPP
#endif

#ifndef TEST_TCP_CHANNELS_INTERFACES_HPP
#define TEST_TCP_CHANNELS_INTERFACES_HPP

#include "network/interface_macros.hpp"

#define BW_STRUCT_MSG( NAME, TYPE )										\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,									\
		TCPChannels##TYPE##StructMessageHandler< 									\
			TCPChannels##TYPE##Interface::NAME##Args >,					\
		&TCPChannels##TYPE##App::NAME )									\

#define BW_VARLEN_MSG( NAME, TYPE ) 									\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,							\
			TCPChannels##TYPE##VarLenMessageHandler,					\
			&TCPChannels##TYPE##App::NAME )
#define BW_SERVER_STRUCT_MSG( NAME ) BW_STRUCT_MSG( NAME, Server )
#define BW_CLIENT_STRUCT_MSG( NAME ) BW_STRUCT_MSG( NAME, Client )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interior interface
// -----------------------------------------------------------------------------

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( TCPChannelsServerInterface )

	BW_SERVER_STRUCT_MSG( msg1 )
		uint32	seq;
		uint32	data;
	END_STRUCT_MESSAGE()

	BW_VARLEN_MSG( msg2, Server )

	BW_SERVER_STRUCT_MSG( disconnect )
		uint32 seq;
	END_STRUCT_MESSAGE()

END_MERCURY_INTERFACE()

BEGIN_MERCURY_INTERFACE( TCPChannelsClientInterface )

	BW_CLIENT_STRUCT_MSG( msg1 )
		uint32	seq;
		uint32	data;
	END_STRUCT_MESSAGE()

END_MERCURY_INTERFACE()


#pragma pack(pop)

#undef BW_COMMON_MSG
#undef BW_SERVER_STRUCT_MSG
#undef BW_CLIENT_STRUCT_MSG

BW_END_NAMESPACE

#endif // TEST_TCP_CHANNELS_INTERFACES_HPP
