#include "dbappmgr.hpp"
#include "dbapp.hpp"

#include "network/common_message_handlers.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This specialisation allows messages directed for a local DBApp object
 *	to be delivered.
 */
template <>
class MessageHandlerFinder< DBApp >
{
public:
	static DBApp * find( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		return ServerApp::getApp< DBAppMgr >( header ).findApp( srcAddr );
	}
};

BW_END_NAMESPACE

#define DEFINE_SERVER_HERE
#include "db/dbappmgr_interface.hpp"

// message_handlers.cpp
