#ifndef TCP_BUNDLE_HPP
#define TCP_BUNDLE_HPP


#include "bundle.hpp"

#include "network/interface_element.hpp"

#include <memory>


BW_BEGIN_NAMESPACE


class MemoryOStream;

namespace Mercury
{
class TCPChannel;


/**
 *	This class represents a bundle sent out on a TCPChannel.
 */
class TCPBundle : public Bundle
{
public:
	typedef uint8 Flags;
	static const int FLAG_HAS_REQUESTS = 0x01;
	static const uint8 FLAG_MASK = 0x01;

	typedef uint32 Offset;

	TCPBundle( TCPChannel & channel );
	virtual ~TCPBundle();


	// Overrides from Bundle.
	virtual void startMessage( const InterfaceElement & ie,
		ReliableType reliable = RELIABLE_DRIVER );

	virtual void startRequest( const InterfaceElement & ie,
		ReplyMessageHandler * pHandler,
		void * arg = NULL,
		int timeout = DEFAULT_REQUEST_TIMEOUT,
		ReliableType reliable = RELIABLE_DRIVER );

	virtual void startReply( ReplyID id, 
		ReliableType reliable = RELIABLE_DRIVER );

	virtual void clear( bool newBundle = false );

	virtual int freeBytesInLastDataUnit() const;

	virtual int numDataUnits() const;

	virtual void doFinalise();

	// Overrides from BinaryOStream.
	virtual void * reserve( int nBytes );
	virtual int size() const;

	// Own methods
	void * data();


private:

	TCPChannel & tcpChannel();
	const TCPChannel & tcpChannel() const 
	{
		return const_cast< TCPBundle * >( this )->tcpChannel();
	}

	Flags & flags();

	void newMessage( const InterfaceElement & ie ); 
	void finaliseCurrentMessage();
	void setNextRequestOffset( Offset offset );

	int currentMessagePayloadLength();

	Offset					frameStartOffset_;
	MemoryOStream * 		pFrameData_;
	MemoryOStream * 		pMessageData_;
	std::auto_ptr< InterfaceElement >
							pCurrentMessage_;

	bool					isCurrentMessageRequest_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // TCP_BUNDLE_HPP

