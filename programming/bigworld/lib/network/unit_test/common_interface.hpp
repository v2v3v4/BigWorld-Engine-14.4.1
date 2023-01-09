#if defined( DEFINE_INTERFACE_HERE ) || defined( DEFINE_SERVER_HERE )
	#undef TEST_COMMON_INTERFACE_HPP
#endif

#ifndef TEST_COMMON_INTERFACE_HPP
#define TEST_COMMON_INTERFACE_HPP

#include "network/udp_channel.hpp"
#include "network/interface_macros.hpp"

#define BW_COMMON_MSG( NAME )							\
	BEGIN_HANDLED_STRUCT_MESSAGE( NAME,					\
		CommonStructMessageHandler< CommonInterface::NAME##Args >,	\
		&CommonHandler::NAME )						\

#define BW_COMMON_VARIABLE_MSG( NAME, SIZEBYTES )				\
	MERCURY_HANDLED_VARIABLE_MESSAGE( NAME, SIZEBYTES,			\
		CommonStreamMessageHandler,					\
		&CommonHandler::NAME )						\


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interior interface
// -----------------------------------------------------------------------------

#pragma pack(push,1)
BEGIN_MERCURY_INTERFACE( CommonInterface )

	BW_COMMON_MSG( msg1 )
		Mercury::UDPChannel::Traits traits;
		uint32	seq;
		uint32	data;
	END_STRUCT_MESSAGE()

	BW_COMMON_VARIABLE_MSG( msgVar1, 1 )

	BW_COMMON_VARIABLE_MSG( msgVar2, 4 )

	BW_COMMON_MSG( disconnect )
		uint32 seq;
	END_STRUCT_MESSAGE()

END_MERCURY_INTERFACE()

#pragma pack(pop)

// -----------------------------------------------------------------------------
// Section: CommonHandler
// -----------------------------------------------------------------------------

#ifndef BW_COMMON_HANDLER
#define BW_COMMON_HANDLER

#define BW_MSG_HANDLER( NAME ) 						\
public:									\
	void NAME( const Mercury::Address & srcAddr, 			\
			const CommonInterface::NAME##Args & args )	\
	{								\
		this->on_##NAME( srcAddr, args );			\
	}								\
									\
protected:								\
	virtual void on_##NAME( const Mercury::Address & srcAddr, 	\
			const CommonInterface::NAME##Args & args ) {}	\

#define BW_VAR_MSG_HANDLER( NAME ) 					\
public:									\
	void NAME( const Mercury::Address & srcAddr, 			\
			BinaryIStream & data )				\
	{								\
		this->on_##NAME( srcAddr, data );			\
	}								\
									\
protected:								\
	virtual void on_##NAME( const Mercury::Address & srcAddr, 	\
			BinaryIStream & data ) {}			\


/**
 *	This class is a base class for handlers of CommonInterface. To support this
 *	interface, you need to do the following:
 *
 * class LocalHandler : public CommonHandler
 * {
 * 	protected:
 * 		// Implement any messages that you want to handle
 * 		void on_msg1( const Mercury::Address & srcAddr,
 * 			const CommonInterface::msg1Args & args )
 * 		{
 * 			...
 * 		}
 *
 * 		void on_msgVar1( const Mercury::Address & srcAddr,
 * 			BinaryIStream & data )
 * 		{
 * 			...
 * 		}
 * };
 *
 *	NetworkInterface networkInterface( &dispatcher );
 *	LocalHandler handler;
 *	networkInterface.pExtensionData( &handler );
 */
class CommonHandler
{
	BW_MSG_HANDLER( msg1 )
	BW_VAR_MSG_HANDLER( msgVar1 )
	BW_VAR_MSG_HANDLER( msgVar2 )
	BW_MSG_HANDLER( disconnect )
};

#undef BW_MSG_HANDLER

#endif // BW_COMMON_HANDLER

#undef BW_COMMON_MSG
#undef BW_MSG

BW_END_NAMESPACE


#endif // TEST_COMMON_INTERFACE_HPP
