#ifndef BUNDLE_HPP
#define BUNDLE_HPP

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bw_vector.hpp"

#include "network/interface_element.hpp"
#include "network/reply_order.hpp"

BW_BEGIN_NAMESPACE


namespace Mercury
{

class Channel;
class InterfaceElement;
class RequestManager;


/**
 *	@internal
 *	This is the default request timeout in microseconds.
 */
const int DEFAULT_REQUEST_TIMEOUT = 5000000;

/**
 *	There are three types of reliability. RELIABLE_PASSENGER messages will
 *	only be sent so long as there is at least one RELIABLE_DRIVER in the
 *	same UDPBundle.  RELIABLE_CRITICAL means the same as RELIABLE_DRIVER,
 *	however, starting a message of this type also marks the UDPBundle as
 *	critical.
 */
enum ReliableTypeEnum
{
	RELIABLE_NO = 0,
	RELIABLE_DRIVER = 1,
	RELIABLE_PASSENGER = 2,
	RELIABLE_CRITICAL = 3
};


/**
 *	This struct wraps a @see ReliableTypeEnum value.
 */
class ReliableType
{
public:
	ReliableType( ReliableTypeEnum e ) : e_( e ) { }

	bool isReliable() const	{ return e_ != RELIABLE_NO; }

	// Leveraging the fact that only RELIABLE_DRIVER and RELIABLE_CRITICAL share
	// the 0x1 bit.
	bool isDriver() const	{ return e_ & RELIABLE_DRIVER; }

	bool operator== (const ReliableTypeEnum e) { return e == e_; }

private:
	ReliableTypeEnum e_;
};


/**
 *	A bundle is a sequence of messages. You stream or otherwise add your
 *	messages onto the bundle. When you want to send a group of messages
 *	(possibly just one), you tell a network interface to send the bundle on a
 *	channel, if there is one. Bundles can be sent multiple times to different
 *	hosts, but beware that any requests inside will also be made multiple
 *	times.
 *
 *	@ingroup mercury
 */
class Bundle : public BinaryOStream
{
public:
	virtual ~Bundle();

	// Overrides for subclasses
	virtual void startMessage( const InterfaceElement & ie,
		ReliableType reliable = RELIABLE_DRIVER ) = 0;

	virtual void startRequest( const InterfaceElement & ie,
		ReplyMessageHandler * handler,
		void * arg = NULL,
		int timeout = DEFAULT_REQUEST_TIMEOUT,
		ReliableType reliable = RELIABLE_DRIVER ) = 0;

	virtual void startReply( ReplyID id, 
		ReliableType reliable = RELIABLE_DRIVER ) = 0;

	virtual void clear( bool newBundle = false );

	/**
	 *	This method returns the number of bytes free in the last data unit to
	 *	be sent for this bundle, that is, the number of bytes that can be sent
	 *	without incurring an extra segment/packet.
	 */
	virtual int freeBytesInLastDataUnit() const = 0;

	/**
	 *	This method returns the number of data units to be sent for this
	 *	bundle.
	 */
	virtual int numDataUnits() const = 0;

	/**
	 *	This method is called to finalise this bundle for sending.
	 */
	virtual void doFinalise() {}

	// General methods
	Channel * pChannel() { return pChannel_; }

	int numMessages() const		{ return numMessages_; }

	void * startStructMessage( const InterfaceElement & ie,
		ReliableType reliable = RELIABLE_DRIVER );

	void * startStructRequest( const InterfaceElement & ie,
		ReplyMessageHandler * handler,
		void * arg = NULL,
		int timeout = DEFAULT_REQUEST_TIMEOUT,
		ReliableType reliable = RELIABLE_DRIVER );

	template<typename ArgsType>
	void sendMessage( const ArgsType & args,
		ReliableType reliable = RELIABLE_DRIVER )
	{
		this->startMessage( ArgsType::interfaceElement(), reliable );
		static_cast<BinaryOStream&>(*this) << args;
	}

	template<typename ArgsType>
	void sendRequest( const ArgsType & args,
		ReplyMessageHandler * handler,
		void * arg = NULL,
		int timeout = DEFAULT_REQUEST_TIMEOUT,
		ReliableType reliable = RELIABLE_DRIVER )
	{
		this->startRequest( ArgsType::interfaceElement(),
			handler, arg, timeout, reliable );
		static_cast<BinaryOStream&>(*this) << args;
	}

	void addReplyOrdersTo( RequestManager * pRequestManager,
			Channel * pChannel ) const;

	void cancelRequests( RequestManager * pRequestManager, Reason reason );

	
	/**
	 *	This method returns whether we have more than one data unit to be sent
	 *	for this bundle.
	 */
	bool hasMultipleDataUnits() const { return this->numDataUnits() > 1; }

	bool isFinalised() const { return isFinalised_; }
	void finalise();

protected:
	Bundle( Channel * pChannel = NULL );


	// Member data

	Channel * pChannel_;
	bool isFinalised_;

	uint numMessages_;

	/// This vector stores all the requests for this bundle.
	typedef BW::vector< ReplyOrder >	ReplyOrders;
	ReplyOrders	replyOrders_;
};

} // end namespace Mercury

#ifdef CODE_INLINE
#include "bundle.ipp"
#endif

BW_END_NAMESPACE

#endif // BUNDLE_HPP
