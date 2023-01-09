#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include "cstdmf/bw_string.hpp"

#include "network/interfaces.hpp"
#include "network/endpoint.hpp"



BW_BEGIN_NAMESPACE

namespace Mercury
{

class EventDispatcher;
class NetworkInterface;
class StreamFilterFactory;
class TCPChannel;


/**
 *	This class represents a server socket that listens for new TCPChannel
 *	connections.
 */
class TCPServer : public InputNotificationHandler
{
public:
	// This is the default for Apache httpd.
	static const uint DEFAULT_BACKLOG = 511;

	TCPServer( NetworkInterface & networkInterface, 
		uint backlog = DEFAULT_BACKLOG );

	bool init();
	bool initWithPort( uint16 port );
	virtual ~TCPServer();

	// Override from InputNotificationHandler
	virtual int handleInputNotification( int fd );

	// Own methods

	/**
	 *	This method returns the address the socket is bound on, or NONE if
	 *	not bound.
	 */
	const Mercury::Address address() const
	{
		return this->isGood() ? serverSocket_.getLocalAddress() : 
			Mercury::Address::NONE;
	}

	/** This method returns if the server socket is good. */
	bool isGood() const { return serverSocket_.good(); }

	/**
	 *	This method returns the stream filter factory used for new connections.
	 */
	StreamFilterFactory * pStreamFilterFactory()
	{
		return pStreamFilterFactory_;
	}


	/**
	 *	This method sets the stream filter factory that will be used to create
	 *	stream filters for new connections when they are created.
	 */
	void pStreamFilterFactory( StreamFilterFactory * pStreamFilterFactory )
	{ 
		pStreamFilterFactory_ = pStreamFilterFactory; 
	}

private:
	EventDispatcher & dispatcher();

	NetworkInterface & 		networkInterface_;
	uint					backlog_;
	Endpoint 				serverSocket_;
	StreamFilterFactory * 	pStreamFilterFactory_;
};


} // end namespace Mercury

BW_END_NAMESPACE

#endif // TCP_SERVER_HPP
