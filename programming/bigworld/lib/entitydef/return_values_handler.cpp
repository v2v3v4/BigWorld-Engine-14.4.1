#include "pch.hpp"

#include "return_values_handler.hpp"

#include "network/basictypes.hpp"
#include "network/nub_exception.hpp"

#include "pyscript/script.hpp"

#include "data_type.hpp"
#include "method_description.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ReturnValuesHandler
// -----------------------------------------------------------------------------

/**
 *	This method initialises the static state used by ReturnValuesHandler.
 */
bool ReturnValuesHandler::staticInit()
{
	bool isTwoWayOkay = MethodDescription::staticInitTwoWay();
	bool isDeferredOkay = PyDeferred::staticInit();

	return isDeferredOkay && isTwoWayOkay;
}


/**
 *	Constructor.
 */
ReturnValuesHandler::ReturnValuesHandler(
		const MethodDescription & methodDescription ) :
	methodDescription_( methodDescription ),
	deferred_()
{
}


/*
 *	This method overrides the ReplyMessageHandler method. It receives a response
 *	from the remote process.
 */
void ReturnValuesHandler::handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data, void * arg )
{
	ScriptObject pValues;

	uint8 isError = methodDescription_.createReturnValuesOrFailureFromStream(
			data, pValues );

	bool isOkay = isError ?
		deferred_.errback( pValues ) :
		deferred_.callback( pValues );

	if (!isOkay)
	{
		WARNING_MSG( "ReturnValuesHandler::handleMessage: "
					"Response handler for method %s failed.\n",
				methodDescription_.name().c_str() );
	}

	delete this;
}


/*
 *	This method overrides the ReplyMessageHandler method. It is called if no
 *	response was received for the request.
 */
void ReturnValuesHandler::handleException(
	const Mercury::NubException & exception, void * arg )
{
	if (!deferred_.mercuryErrback( exception ))
	{
		WARNING_MSG( "ReturnValuesHandler::handleException: "
					"Response handler for method %s failed.\n",
				methodDescription_.name().c_str() );
	}

	delete this;
}

BW_END_NAMESPACE

// return_values_handler.cpp
