#ifndef UNPACKED_MESSAGE_HEADER_HPP
#define UNPACKED_MESSAGE_HEADER_HPP

#include "misc.hpp"
#include "channel.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class NetworkInterface;

/**
 * 	This structure is returned by Mercury when delivering
 * 	messages to a client.
 *
 * 	@ingroup mercury
 */
class UnpackedMessageHeader
{
public:
	static const ReplyID NO_REPLY = REPLY_ID_NONE;

	MessageID		identifier;		///< The message identifier.
	ReplyID			replyID;		///< A unique ID, used for replying.
	int				length;			///< The number of bytes in this message.
	bool * 			pBreakLoop;		///< Used to break bundle processing.
	const InterfaceElement *
		pInterfaceElement;	///< The interface element for this message.
	ChannelPtr		pChannel;		///< The channel that received this message.
	NetworkInterface * pInterface;  ///< The interface this message came on.

	UnpackedMessageHeader() :
		identifier( 0 ),
		replyID( REPLY_ID_NONE ), length( 0 ),
		pBreakLoop( NULL ), pInterfaceElement( NULL ), pChannel( NULL ),
		pInterface( NULL )
	{}

	const char * msgName() const;

	void breakBundleLoop() const
	{
		if (pBreakLoop)
		{
			*pBreakLoop = true;
		}
	}

private:
};

} // namespace Mercury

BW_END_NAMESPACE

#endif // UNPACKED_MESSAGE_HEADER_HPP
