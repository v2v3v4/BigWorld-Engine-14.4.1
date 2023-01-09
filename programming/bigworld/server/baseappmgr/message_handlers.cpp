#include "baseapp.hpp"
#include "baseappmgr.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "network/channel_sender.hpp"
#include "network/common_message_handlers.hpp"
#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This specialisation allows messages directed for a local BaseApp object
 *	to be delivered.
 */
template <>
class MessageHandlerFinder< BaseApp >
{
public:
	static BaseApp * find( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		return ServerApp::getApp< BaseAppMgr >( header ).findBaseApp( srcAddr );
	}
};

BW_END_NAMESPACE

#define DEFINE_SERVER_HERE
#include "baseappmgr_interface.hpp"

// message_handlers.cpp
