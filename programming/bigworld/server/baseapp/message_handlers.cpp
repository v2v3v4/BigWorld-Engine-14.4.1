#include "script/first_include.hpp"

#include "message_handlers.hpp"

#include "base.hpp"
#include "baseapp.hpp"
#include "proxy.hpp"

#include "cstdmf/debug.hpp"

#include "network/common_message_handlers.hpp"
#include "network/interfaces.hpp"
#include "network/smart_bundles.hpp"
#include "network/unpacked_message_header.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Interface stuff
// -----------------------------------------------------------------------------

/**
 *	This class is used to handle the finishing on bundles on the internal
 *	interface.
 */
class InternalBundleEventHandler : public Mercury::BundleEventHandler
{
protected:

	virtual void onBundleStarted( Mercury::Channel * pChannelUncast )
	{
		BaseApp & baseApp = BaseApp::instance();

		Mercury::UDPChannel * pChannel = 
			static_cast< Mercury::UDPChannel * >( pChannelUncast );

		if (pChannel && pChannel->isIndexed())
		{
			Base * pBase = baseApp.bases().findEntity( pChannel->id() );

			if (pBase)
			{
				baseApp.setBaseForCall( pBase, /* isExternalCall */ false );
			}
			else
			{
				ERROR_MSG( "InternalBundleEventHandler::onBundleStarted: "
						"Couldn't find base with ID %u\n", pChannel->id() );
			}
		}
	}

	virtual void onBundleFinished( Mercury::Channel * pChannel )
	{
		BaseApp::instance().clearProxyForCall();
	}
};


/**
 *	This class is used to handle the finishing on bundles on the external
 *	interface.
 */
class ExternalBundleEventHandler : public Mercury::BundleEventHandler
{
protected:
	virtual void onBundleFinished( Mercury::Channel * pChannel )
	{
		ProxyPtr pProxy = BaseApp::instance().clearProxyForCall();

		if (pProxy)
		{
			if (pProxy->hasCellEntity())
			{
				pProxy->sendToCell();
			}
		}
	}
};


namespace InternalInterfaceHandlers
{
void init( Mercury::InterfaceTable & interfaceTable )
{
	static InternalBundleEventHandler s_bundleEventHandler;
	interfaceTable.pBundleEventHandler( &s_bundleEventHandler );
}

} // namespace InternalInterfaceHandlers


namespace ExternalInterfaceHandlers
{

void init( Mercury::InterfaceTable & interfaceTable )
{
	static ExternalBundleEventHandler s_bundleEventHandler;
	interfaceTable.pBundleEventHandler( &s_bundleEventHandler );
}

} // namespace ExternalInterfaceHandlers


/**
 *	Objects of this type are used to handle proxy messages
 */
template <class ARGS_TYPE>
class ProxyMessageHandler : public Mercury::InputMessageHandler
{
public:
	typedef void (Proxy::*Handler)( const ARGS_TYPE & args );

	// Constructors
	ProxyMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ARGS_TYPE * args;
		args = (ARGS_TYPE*)data.retrieve(sizeof(ARGS_TYPE));

		BaseApp & app = ServerApp::getApp< BaseApp >( header );
		ProxyPtr pProxy = app.getAndCheckProxyForCall( header, srcAddr );

		if (pProxy)
		{
			if (!pProxy->isRestoringClient())
			{
				(pProxy.get()->*handler_)( *args );
			}
			/*
			else
			{
				INFO_MSG( "ProxyMessageHandler::handleMessage: "
						"Client %lu restoring. Skipping message %lu\n",
								pProxy->id(),
								header.identifier );
			}
			*/
		}
		else
		{
			// warning message displayed by getAndCheckProxyForCall
			data.finish();
		}
	}

	Handler handler_;
};


/**
 *	Objects of this type are used to handle proxy messages
 */
template <>
class ProxyMessageHandler< void > : public Mercury::InputMessageHandler
{
public:
	typedef void (Proxy::*Handler)();

	// Constructors
	ProxyMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		BaseApp & app = ServerApp::getApp< BaseApp >( header );
		ProxyPtr pProxy = app.getAndCheckProxyForCall( header, srcAddr );

		if (pProxy)
		{
			if (!pProxy->isRestoringClient())
			{
				(pProxy.get()->*handler_)();
			}
			/*
			else
			{
				INFO_MSG( "ProxyMessageHandler::handleMessage: "
						"Client %lu restoring. Skipping message %lu\n",
								pProxy->id(),
								header.identifier );
			}
			*/
		}
		else
		{
			// warning message displayed by getAndCheckProxyForCall
			data.finish();
		}
	}

	Handler handler_;
};


/**
 *	Objects of this type are used to handle proxy messages. These messages are
 *	not blocked if isRestoringClient is true.
 */
template <class ARGS_TYPE>
class NoBlockProxyMessageHandler : public Mercury::InputMessageHandler
{
public:
	typedef void (Proxy::*Handler)( const ARGS_TYPE & args );

	// Constructors
	NoBlockProxyMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		ARGS_TYPE * args;
		args = (ARGS_TYPE*)data.retrieve(sizeof(ARGS_TYPE));

		BaseApp & app = ServerApp::getApp< BaseApp >( header );
		ProxyPtr pProxy = app.getAndCheckProxyForCall( header, srcAddr );

		if (pProxy)
		{
			(pProxy.get()->*handler_)( *args );
		}
		else
		{
			// warning message displayed by getAndCheckProxyForCall
			data.finish();
		}
	}

	Handler handler_;
};


/**
 *	Objects of this type are used to handle proxy messages. These messages are
 *	not blocked if isRestoringClient is true.
 */
template <>
class NoBlockProxyMessageHandler< void > : public Mercury::InputMessageHandler
{
public:
	typedef void (Proxy::*Handler)();

	// Constructors
	NoBlockProxyMessageHandler( Handler handler ) : handler_( handler ) {}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		BaseApp & app = ServerApp::getApp< BaseApp >( header );
		ProxyPtr pProxy = app.getAndCheckProxyForCall( header, srcAddr );

		if (pProxy)
		{
			(pProxy.get()->*handler_)();
		}
		else
		{
			// warning message displayed by getAndCheckProxyForCall
			data.finish();
		}
	}

	Handler handler_;
};


/**
 *	Objects of this type are used to handle variable length messages destined
 *	for a proxy.
 */
template <bool SHOULD_BLOCK>
class ProxyVarLenMessageHandler : public Mercury::InputMessageHandler
{
public:
	typedef void (Proxy::*Handler)( const Mercury::Address & srcAddr,
									Mercury::UnpackedMessageHeader & header,
									BinaryIStream & data );

	// Constructors
	ProxyVarLenMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		BaseApp & app = ServerApp::getApp< BaseApp >( header );
		ProxyPtr pProxy = app.getAndCheckProxyForCall( header, srcAddr );

		if (!pProxy)
		{
			// warning message displayed by getAndCheckProxyForCall
			data.finish();
			return;
		}

		if (SHOULD_BLOCK && pProxy->isRestoringClient())
		{
			/*
			INFO_MSG( "ProxyVarLenMessageHandler::handleMessage: "
					"Client %lu restoring. Skipping message %lu\n",
							pProxy->id(),
							header.identifier );
			*/
			data.finish();
			return;
		}

		(pProxy.get()->*handler_)( srcAddr, header, data );
	}

	Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: Base Message Handlers
// -----------------------------------------------------------------------------

/**
 *	Objects of this type are used to handle base messages
 */
class CommonBaseMessageHandler : public Mercury::InputMessageHandler
{
public:
	virtual void handleMessage( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		BaseApp & app = ServerApp::getApp< BaseApp >( header );

		if (app.forwardBaseMessageIfNecessary( 0, srcAddr, header, data ))
		{
			return;
		}

		Base * pBase = app.getBaseForCall( true /*okIfNull*/ );

		if (pBase != NULL)
		{
			AUTO_SCOPED_ENTITY_PROFILE( pBase );
			this->callHandler( srcAddr, header, data, pBase );
		}
		else
		{
			WARNING_MSG( "CommonBaseMessageHandler::handleMessage(%s): "
					"%s (id %d). Could not find entity %d: %s\n",
					srcAddr.c_str(),
					header.msgName(),
					header.identifier,
					app.lastMissedBaseForCall(),
					data.remainingBytesAsDebugString().c_str() );

			if (header.replyID != Mercury::REPLY_ID_NONE)
			{
				const Mercury::MessageID id = header.identifier;

				if ((id == BaseAppIntInterface::callBaseMethod.id() ) ||
					(id == BaseAppIntInterface::callCellMethod.id() ))
				{
					MethodDescription::sendReturnValuesError(
							"BWNoSuchEntityError", "No entity found",
							header.replyID, srcAddr, app.intInterface() );
				}
				else
				{
					// Send empty reply to indicate failure for non-script calls
					Mercury::OnChannelBundle bundle( app.intInterface(), srcAddr );
					bundle->startReply( header.replyID );
				}
			}
		}
	}

protected:
	virtual void callHandler( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			Base * pBase ) = 0;
};


/**
 *	Objects of this type are used to handle base messages
 */
template <class ARGS_TYPE>
class BaseMessageStructHandlerEx : public CommonBaseMessageHandler
{
public:
	typedef void (Base::*Handler)( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			const ARGS_TYPE & args );

	// Constructors
	BaseMessageStructHandlerEx( Handler handler ) : handler_( handler ) {}

private:
	virtual void callHandler( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			Base * pBase )
	{
		ARGS_TYPE * args;
		args = (ARGS_TYPE*)data.retrieve(sizeof(ARGS_TYPE));
		(pBase->*handler_)( srcAddr, header, *args );
	}

	Handler handler_;
};


/**
 *	Objects of this type are used to handle variable length messages destined
 *	for a base.
 */
class BaseMessageStreamHandler : public CommonBaseMessageHandler
{
public:
	typedef void (Base::*Handler)( BinaryIStream & stream );

	// Constructors
	BaseMessageStreamHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void callHandler( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			Base * pBase )
	{
		(pBase->*handler_)( data );
	}

	Handler handler_;
};


/**
 *	Objects of this type are used to handle variable length messages destined
 *	for a base.
 */
class BaseMessageStreamHandlerEx : public CommonBaseMessageHandler
{
public:
	typedef void (Base::*Handler)( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	// Constructors
	BaseMessageStreamHandlerEx( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void callHandler( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			Base * pBase )
	{
		(pBase->*handler_)( srcAddr, header, data );
	}

	Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: Global initialisers
// -----------------------------------------------------------------------------

/**
 *	This specialisation allows messages directed for a Base entity to be
 *	delivered.
 */
template <>
class MessageHandlerFinder< Base >
{
public:
	static Base * find( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data )
	{
		BaseApp & app = ServerApp::getApp< BaseApp >( header );
		Base * pBase = app.getBaseForCall();

		if (!pBase)
		{
			WARNING_MSG( "MessageHandlerFinder::find: "
						"From %s: %s (id %d). Failed to find entity %d\n",
				srcAddr.c_str(),
				header.msgName(),
				header.identifier,
				app.lastMissedBaseForCall() );
		}

		return pBase;
	}
};

BW_END_NAMESPACE


#define DEFINE_SERVER_HERE
#include "baseapp_int_interface.hpp"

#define DEFINE_SERVER_HERE
#include "connection/baseapp_ext_interface.hpp"

// message_handlers.cpp
