#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef TEST_FLOOD_INTERFACE_HPP
#endif

#ifndef TEST_FLOOD_INTERFACE_HPP
#define TEST_FLOOD_INTERFACE_HPP

#include "network/channel.hpp"
#include "network/interface_macros.hpp"

#define BW_COMMON_MSG( NAME, TYPE )										\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,									\
		TYPE##FloodStructMessageHandler< 								\
			Flood##TYPE##Interface::NAME##Args >,						\
		&Flood##TYPE##App::NAME )										\

#define BW_VARLEN_MSG( NAME, TYPE ) 									\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, 2,							\
			VarLenMessageHandler< Flood##TYPE##App >,					\
			&Flood##TYPE##App::NAME )

#define BW_SERVER_STRUCT_MSG( NAME ) BW_COMMON_MSG( NAME, Server )
#define BW_CLIENT_STRUCT_MSG( NAME ) BW_COMMON_MSG( NAME, Client )
#define BW_SERVER_VARLEN_MSG( NAME ) BW_VARLEN_MSG( NAME, Server )
#define BW_CLIENT_VARLEN_MSG( NAME ) BW_VARLEN_MSG( NAME, Client )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interior interface
// -----------------------------------------------------------------------------

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( FloodServerInterface )

	BW_SERVER_STRUCT_MSG( connect )
		Mercury::UDPChannel::Traits traits;
	END_STRUCT_MESSAGE()

	BW_SERVER_VARLEN_MSG( disconnect )

	BW_SERVER_VARLEN_MSG( msg1 )

END_MERCURY_INTERFACE()

BEGIN_MERCURY_INTERFACE( FloodClientInterface )
	BW_CLIENT_VARLEN_MSG( connectAck )

	BW_CLIENT_VARLEN_MSG( msg1Ack )

	BW_CLIENT_VARLEN_MSG( msg2 )
END_MERCURY_INTERFACE()


#pragma pack(pop)

BW_END_NAMESPACE

#undef BW_COMMON_MSG
#undef BW_SERVER_STRUCT_MSG
#undef BW_CLIENT_STRUCT_MSG
#undef BW_SERVER_VARLEN_MSG
#undef BW_CLIENT_VARLEN_MSG
#undef BW_SERVER_VARLEN_MSG
#undef BW_CLIENT_VARLEN_MSG
#endif // TEST_FLOOD_INTERFACE_HPP
