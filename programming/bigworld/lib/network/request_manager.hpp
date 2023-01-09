#ifndef REQUEST_MANAGER_HPP
#define REQUEST_MANAGER_HPP

#include "interfaces.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class Channel;
class EventDispatcher;
class InterfaceTable;
class NetworkInterface;
class ReplyOrder;
class Request;
class RequestManager;


/**
 *
 */
class RequestManager : public InputMessageHandler
{
public:
	RequestManager( bool isExternal,
		EventDispatcher & dispatcher );
	~RequestManager();

	void cancelRequestsFor( Channel * pChannel );
	void cancelRequestsFor( ReplyMessageHandler * pHandler,
			Reason reason = REASON_CHANNEL_LOST );

	void addReplyOrder( const ReplyOrder & replyOrder, Channel * pChannel );

	void failRequest( Request & request, Reason reason );

private:
	ReplyID getNextReplyID()
	{
		if (nextReplyID_ > REPLY_ID_MAX)
			nextReplyID_ = 1;

		return nextReplyID_++;
	}

	virtual void handleMessage( const Address & source,
		UnpackedMessageHeader & header,
		BinaryIStream & data );

	// -------------------------------------------------------------------------
	// Section: Properties
	// -------------------------------------------------------------------------

	typedef BW::map< int, Request * > RequestMap;
	RequestMap requestMap_;

	ReplyID nextReplyID_;

	EventDispatcher & dispatcher_;

	const bool isExternal_; 
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // REQUEST_MANAGER_HPP
