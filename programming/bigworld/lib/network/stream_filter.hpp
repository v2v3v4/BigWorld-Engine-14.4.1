#ifndef STREAM_FILTER_HPP
#define STREAM_FILTER_HPP


#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class Endpoint;
class BinaryIStream;
class BinaryOStream;


namespace Mercury
{
class TCPChannel;


/**
 *	This class is the base class for network streams.  
 */
class NetworkStream : public ReferenceCount
{
public:
	static const bool WRITE_SHOULD_CORK = true;
	static const bool WRITE_SHOULD_NOT_CORK = false;

	/**
	 *	Destructor.
	 */
	virtual ~NetworkStream() {}

	// Own methods

	/**
	 *	This method writes the given input stream data through this filter onto
	 *	the underlying stream.
	 *
	 *	@param input 		The input data stream.
	 *	@param shouldCork 	Whether to send any buffered data and the given
	 *						input data immediately, or buffer the send.
	 *
	 *	@return 			True on success, false on error.
	 */
	virtual bool writeFrom( BinaryIStream & input, bool shouldCork ) = 0;


	/**
	 *	This method reads from the underlying stream through this filter and
	 *	into the given output stream.
	 *
	 *	@param output 		The output stream.
	 */
	virtual int readInto( BinaryOStream & output ) = 0;


	/**
	 *	This method returns a string representation for debugging.
	 */
	virtual BW::string streamToString() const = 0;


	/**
	 *	This method shuts the stream down cleanly.
	 */
	virtual void shutDown() = 0;


	/**
	 *	This method returns whether this stream has any buffered send data.
	 */
	virtual bool hasUnsentData() const 
	{ return false; }


	/**
	 *	This is called by the channel when it receives notification of
	 *	inactivity.
	 *
	 *	@return 		True if the filter has handled the timeout and consumed
	 *					the event, otherwise if false, the channel should do
	 *					its own handling.
	 */
	virtual bool didHandleChannelInactivityTimeout()
	{
		return false;
	}


	/**
	 *	This is called by the channel when it is shutting down gracefully.
	 *
	 *	Subclasses should start any shutdown procedures relevant to their
	 *	level, and should return false if they need the keep the channel open
	 *	to complete shutdown e.g. shutdown handshakes. 
	 *
	 *	When shutdown procedures are complete at this level, the lower-level
	 *	stream will have shutDown() called and this method should then return
	 *	true. Eventually the lowest level stream will call through to
	 *	Channel::shutDown() and all filters will have returned true in
	 *	didFinishShuttingDown(), enabling the TCPChannel to begin its own
	 *	shutdown handshake.
	 *
	 *	@return 	True if this stream filter has finished shutting down,
	 *				false otherwise.
	 */
	virtual bool didFinishShuttingDown()
	{
		return true;
	}


	/**
	 *	This method returns whether the underlying stream has been
	 *	disconnected.
	 */
	virtual bool isConnected() const = 0;

protected:
	/**
	 *	Constructor.
	 */
	NetworkStream() :
		ReferenceCount() {}
};

typedef SmartPointer< NetworkStream > NetworkStreamPtr;


/**
 *	This is the base class for stream filters. Stream filters can be attached
 *	to TCPChannels, and implement some sort of transformation between the
 *	high-level application-level and the transport-level, for example,
 *	encryption.
 */
class StreamFilter : public NetworkStream
{
public:

	/**
 	 *	Destructor.
	 */
	virtual ~StreamFilter() {}

	// Overrides from NetworkStream
	virtual bool writeFrom( BinaryIStream & input, bool shouldCork ) = 0;

	virtual int readInto( BinaryOStream & output ) = 0;


	/* Override from NetworkStream. */
	virtual BW::string streamToString() const
	{ 
		return pStream_->streamToString(); 
	}


	/* Override from NetworkStream. */
	virtual bool isConnected() const
	{ 
		return pStream_->isConnected(); 
	}


	/* Override from NetworkStream. */
	virtual bool didHandleChannelInactivityTimeout()
	{
		return pStream_->didHandleChannelInactivityTimeout();	
	}


	/**
	 *	Override from NetworkStream.
	 *
	 *	This should NOT be overridden by StreamFilter subclasses. The method
	 *	didFinishShuttingDown() will be called in response to a shutdown request
	 *	from TCPChannel, and should be overridden to implement shutdown
	 *	initiation.
	 */
	virtual void shutDown()
	{
		return pStream_->shutDown();
	}

	/* Override from NetworkStream. */
	virtual bool didFinishShuttingDown()
	{
		return pStream_->didFinishShuttingDown();
	}

	
	/* Override from NetworkStream. */
	virtual bool hasUnsentData() const
	{
		return pStream_->hasUnsentData();
	}

	// Own methods

	/**
	 *	This method returns the underlying stream.
	 */
	NetworkStream & stream() { return *pStream_; }

	/**
	 *	This method returns the underlying stream.
	 */
	const NetworkStream & stream() const { return *pStream_; }

protected:

	/**
	 *	Constructor.
	 *
	 *	@param stream 	The underlying network stream.
	 */
	StreamFilter( NetworkStream & stream ) :
			NetworkStream(),
			pStream_( &stream )
	{}

private:
	NetworkStreamPtr pStream_;
};


typedef SmartPointer< StreamFilter > StreamFilterPtr;


} // end namespace Mercury


BW_END_NAMESPACE

#endif // STREAM_FILTER_HPP
