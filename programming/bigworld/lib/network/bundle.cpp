#include "pch.hpp"

#include "bundle.hpp"

#include "network/nub_exception.hpp"
#include "network/request_manager.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "bundle.ipp"
#endif


namespace Mercury
{


/**
 *	Constructor.
 */
Bundle::Bundle( Channel * pChannel ) : 
	BinaryOStream(),
	pChannel_( pChannel ),
	isFinalised_( false ),
	numMessages_( 0 ),
	replyOrders_()
{}


/**
 *	Destructor.
 */
Bundle::~Bundle()
{}


/**
 *	This method cancels the requests made on this bundle.
 */
void Bundle::cancelRequests( RequestManager * pRequestManager, Reason reason )
{
	if (!pRequestManager)
	{
		return;
	}

	ReplyOrders::iterator iter = replyOrders_.begin();

	while (iter != replyOrders_.end())
	{
		pRequestManager->cancelRequestsFor( iter->handler, reason );
		++iter;
	}

	replyOrders_.clear();
}


/**
 * 	This method finalises the bundle before it is sent.
 */
void Bundle::finalise()
{ 
	if (isFinalised_)
	{
		return;
	}

	this->doFinalise();

	isFinalised_ = true; 
}


/**
 * 	This method grabs all the reliable data from the source packet, and
 * 	appends it to this bundle. It handles the case where the source packet
 *	contains partial messages from a multi-packet bundle. It does nothing
 *	and returns false if the reliable data cannot all fit into the current
 *	packet.
 *
 *	@param pRequestManager		The request manager to add requests to.
 *	@param pChannel 			The channel receiving replies back. 
 */
void Bundle::addReplyOrdersTo( RequestManager * pRequestManager,
		Channel * pChannel ) const
{
	MF_ASSERT( isFinalised_ );

	if (!pRequestManager)
	{
		return;
	}

	ReplyOrders::const_iterator iter = replyOrders_.begin();

	while (iter != replyOrders_.end())
	{
		pRequestManager->addReplyOrder( *iter, pChannel );

		++iter;
	}
}


/**
 *	This method clears this bundle and readies it for new messages to be sent.
 *
 *	@param newBundle 	Whether this is called for a newly created bundle.
 */
void Bundle::clear( bool newBundle )
{
	isFinalised_ = false;
	numMessages_ = 0;
	replyOrders_.clear();
}


} // end namespace Mercury


BW_END_NAMESPACE

// bundle.cpp

