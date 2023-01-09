#ifndef TCP_CHANNEL_STREAM_ADAPTOR
#define TCP_CHANNEL_STREAM_ADAPTOR

#include "stream_filter.hpp"
#include "tcp_channel.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{


/**
 *	This class is an adaptor class between a TCPChannel and NetworkStream. 
 *
 *	This is required because TCPChannel derives already from ReferenceCount,
 *	which NetworkStream derives from. 
 *
 *	Note instances of this class hold a reference to the channel, and the
 *	channel holds a reference to the top of the stream filter stack. Breaking
 *	this circular reference is required when shutting down the channel.
 */
class TCPChannelStreamAdaptor : public NetworkStream
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param channel 	The TCP channel to wrap as a NetworkStream.
	 */
	TCPChannelStreamAdaptor( TCPChannel & channel ) :
		NetworkStream(),
		pChannel_( &channel ),
		channelAddress_( pChannel_->addr() )
	{
	}

	
	/**
	 *	Destructor.
	 */
	virtual ~TCPChannelStreamAdaptor()
	{
	}


	/*
	 *	Override from NetworkStream.
	 */
	virtual bool writeFrom( BinaryIStream & input, bool shouldCork )
	{
		return pChannel_->writeFrom( input, shouldCork );
	}


	/*
	 *	Override from NetworkStream.
	 */
	virtual int readInto( BinaryOStream & output )
	{
		return pChannel_->readInto( output );
	}


	/*
	 *	Override from NetworkStream.
	 */
	virtual BW::string streamToString() const
	{
		BW::ostringstream out;
		out << "tcp:";
		if (pChannel_.exists())
		{
			out << pChannel_->addr().c_str();
		}
		else
		{
			out << channelAddress_.c_str() << "!";
		}

		return out.str();
	}


	/*
	 *	Override from NetworkStream.
	 */
	virtual void shutDown()
	{
		// Save this away so we can still print it out.
		channelAddress_ = pChannel_->addr();

		pChannel_->shutDown();

		// Break the circular reference.
		pChannel_ = NULL;
	}


	/*
	 *	Override from NetworkStream.
	 */
	virtual bool isConnected() const
	{
		return pChannel_ && pChannel_->isConnected();
	}


	/*
	 *	Override from NetworkStream.
	 */
	virtual bool hasUnsentData() const
	{
		// Do not call TCPChannel::hasUnsentData(), otherwise we recurse.
		return (pChannel_->bundle().numMessages() > 0);
	}


private:
	
	TCPChannelPtr pChannel_;
	Mercury::Address channelAddress_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // TCP_CHANNEL_STREAM_ADAPTOR

