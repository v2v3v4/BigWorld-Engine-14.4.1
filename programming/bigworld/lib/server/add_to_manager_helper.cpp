#include "add_to_manager_helper.hpp"

#include "cstdmf/binary_stream.hpp"

#include "network/event_dispatcher.hpp"
#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE

/**
 *  Constructor.
 */
AddToManagerHelper::AddToManagerHelper(
		Mercury::EventDispatcher & dispatcher ) :
	dispatcher_( dispatcher )
{}


AddToManagerHelper::~AddToManagerHelper()
{
	resendTimerHandle_.cancel();
	fatalTimerHandle_.cancel();
}


/**
 *  This method handles the reply from the manager process.  Zero-length replies
 *  mean that the manager is not ready to add child apps at the moment and we
 *  should wait and try again later.
 */
void AddToManagerHelper::handleMessage(
	const Mercury::Address & source,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	void * arg )
{
	if (data.remainingLength())
	{
		if (!this->finishInit( data ))
		{
			ERROR_MSG( "AddToManagerHelper::handleMessage: "
				"finishInit() failed, aborting\n" );

			dispatcher_.breakProcessing();
		}

		fatalTimerHandle_.cancel();

		delete this;
	}
	else
	{
		MF_ASSERT( !resendTimerHandle_.isSet() );

		resendTimerHandle_ = dispatcher_.addOnceOffTimer( 1000000, this,
			reinterpret_cast< void *>( TIMEOUT_RESEND ) );
	}
}


/**
 *  This method handles a reply timeout, which means that this app couldn't add
 *  itself to the manager and should bail out.
 */
void AddToManagerHelper::handleException(
	const Mercury::NubException & exception,
	void * arg )
{
	ERROR_MSG( "AddToManagerHelper::handleException: "
		"Failed to add ourselves to the manager (%s)\n",
		Mercury::reasonToString( exception.reason() ) );

	dispatcher_.breakProcessing();
	delete this;
}


/**
 *  This method handles a callback timeout, which means it's time to send
 *  another add message to the manager.
 */
void AddToManagerHelper::handleTimeout( TimerHandle handle, void * arg )
{
	switch (reinterpret_cast< uintptr >( arg ))
	{
		case TIMEOUT_RESEND:
			this->send();
			resendTimerHandle_.cancel();
			break;

		case TIMEOUT_FATAL:
			this->handleFatalTimeout();
			fatalTimerHandle_.cancel();
			break;

		default:
			MF_ASSERT( !"Invalid TIMEOUT condition" );
	}
}


void AddToManagerHelper::send()
{
	fatalTimerHandle_.cancel();
	fatalTimerHandle_ = dispatcher_.addOnceOffTimer( 5000000, this,
				reinterpret_cast< void *>( TIMEOUT_FATAL ) );

	this->doSend();
}

BW_END_NAMESPACE

// add_to_manager_helper.cpp
