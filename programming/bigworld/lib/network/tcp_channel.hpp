#ifndef TCP_CHANNEL_HPP
#define TCP_CHANNEL_HPP

#include "cstdmf/smartpointer.hpp"

#include "network/channel.hpp"
#include "network/interfaces.hpp"
#include "network/stream_filter.hpp"

BW_BEGIN_NAMESPACE

class Endpoint;
class MemoryOStream;

namespace Mercury
{

class TCPChannel;
typedef SmartPointer< TCPChannel > TCPChannelPtr;


/**
 *	This class represents a Mercury channel over TCP.
 */
class TCPChannel : public Channel,
		public InputNotificationHandler
{
public:
	TCPChannel( NetworkInterface & networkInterface, Endpoint & endpoint,
		bool isServer );

	virtual ~TCPChannel();

	// Override from InputNotificationHandler
	virtual int handleInputNotification( int fd );

	// Overrides from Channel
#if ENABLE_WATCHERS
	virtual WatcherPtr getWatcher();
#endif // ENABLE_WATCHERS

	virtual const char * c_str() const;
	virtual Bundle * newBundle();
	virtual bool hasUnsentData() const;

	/* Override from Channel */
	virtual bool isExternal() const { return true; }

	/* Override from Channel */
	virtual bool isTCP() const { return true; }

	/* Override from Channel */
	virtual bool isConnected() const 
	{ 
		return !isShuttingDown_ && !this->isDestroyed(); 
	}

	virtual void setEncryption( Mercury::BlockCipherPtr pBlockCipher );

	virtual void shutDown();

	virtual double roundTripTimeInSeconds() const;

	virtual void startInactivityDetection( float period,
		float checkPeriod = 1.f );
	virtual void onChannelInactivityTimeout();

	// Own methods
	const char * asString() const;
	int readInto( BinaryOStream & output );
	bool writeFrom( BinaryIStream & input, bool shouldCork );

	/**
	 *	This method sets/clears the stream filter for this channel.
	 *
	 *	@param pStreamFilter 	The stream filter to set, or NULL if the stream
	 *							filter is to be cleared.
	 */
	void pStreamFilter( StreamFilter * pStreamFilter )
	{
		pStreamFilter_ = pStreamFilter; 
	}

	/**
	 *	This method returns the stream filter for this channel, or NULL if none
	 *	is attached.
	 *
	 *	@return 	The stream filter, or NULL.
	 */
	StreamFilter * pStreamFilter()
	{
		return pStreamFilter_.get();
	}


	int maxSegmentSize() const;

	int numBytesAvailableForReading() const;

	bool sendBufferedData();

protected:
	// Overrides from Channel
	virtual void doSend( Bundle & pBundle );
	virtual void doDestroy();

private:

	void handleInput();
	bool readHeader();
	void readSmallFrameLength();
	void readLargeFrameLength();
	void processReceivedFrame();
	void cleanUpAfterFrameReceive();

	int basicSend( const void * data, int size, bool shouldCork = false );

	void doShutDown();
	void handlePeerDisconnect();

	enum HeaderState
	{
		HEADER_STATE_NONE,
		HEADER_STATE_SERVER_WAIT_FOR_HEADER,
		HEADER_STATE_CLIENT_SEND_HEADER
	};


	StreamFilterPtr 			pStreamFilter_;
	Endpoint * 					pEndpoint_;
	HeaderState 				headerState_;
	bool						isShuttingDown_;
	MemoryOStream * 			pFrameData_;
	uint 						frameLength_;
	uint 						frameHeaderLength_;
	MemoryOStream * 			pSendBuffer_;
	InputNotificationHandler *	pSendWaiter_;
};


} // end namespace Mercury

BW_END_NAMESPACE

#endif // TCP_CHANNEL_HPP

