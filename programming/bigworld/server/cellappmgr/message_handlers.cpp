#include "cellappmgr.hpp"
#include "cellapp.hpp"

#include "network/common_message_handlers.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This specialisation allows messages directed for a CellApp to be delivered.
 */
template <>
class MessageHandlerFinder< CellApp >
{
public:
	static CellApp * find( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		return ServerApp::getApp< CellAppMgr >( header ).findApp( srcAddr );
	}
};

BW_END_NAMESPACE


#define DEFINE_SERVER_HERE
#include "cellappmgr_interface.hpp"

// message_handlers.cpp
