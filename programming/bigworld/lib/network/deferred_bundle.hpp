#ifndef DEFERRED_BUNDLE_HPP
#define DEFERRED_BUNDLE_HPP


#include "bundle.hpp"

#include "cstdmf/bw_list.hpp"

#include <memory>


BW_BEGIN_NAMESPACE


class MemoryOStream;

namespace Mercury
{


/**
 *	This class is used to stream messages on to a bundle before an actual
 *	channel has been created. Messages streamed onto a DeferredBundle can then
 *	be applied to another bundle.
 */
class DeferredBundle : public Bundle
{
public:
	DeferredBundle();
	virtual ~DeferredBundle();

	void applyToBundle( Bundle & bundle );

	// Overrides from Bundle

	virtual void startMessage( const InterfaceElement & ie,
		ReliableType reliable = RELIABLE_DRIVER );
	virtual void startRequest( const InterfaceElement & ie,
		ReplyMessageHandler * handler,
		void * arg = NULL,
		int timeout = DEFAULT_REQUEST_TIMEOUT,
		ReliableType reliable = RELIABLE_DRIVER );
	virtual void startReply( ReplyID id, 
		ReliableType reliable = RELIABLE_DRIVER );
	virtual void clear( bool newBundle = false );
	virtual int freeBytesInLastDataUnit() const;
	virtual int numDataUnits() const;
	virtual int numMessages() const;
	virtual void finalise();


	// Overrides from BinaryOStream
	virtual void * reserve( int nBytes );
	virtual int size() const;

private:

	/**
	 *	A deferred message.
	 */
	class DeferredMessage
	{
	public:
		DeferredMessage( int offset, 
			const InterfaceElement & ie, ReliableType reliable,
			ReplyMessageHandler * pRequestHandler = NULL, void * arg = 0,
			int timeout = DEFAULT_REQUEST_TIMEOUT );

		void finish( int newOffset );
		void apply( Bundle & bundle, BinaryIStream & stream );
	private:
		int offset_;
		int length_;

		InterfaceElement ie_;
		ReliableType reliable_;
		ReplyMessageHandler * pRequestHandler_;
		void * arg_;
		int timeout_;
	};

	typedef BW::list< DeferredMessage > DeferredMessages;

	DeferredMessages messages_;
	std::auto_ptr< MemoryOStream > pData_;
};


} // end namespace Mercury


BW_END_NAMESPACE

#endif // DEFERRED_BUNDLE_HPP

