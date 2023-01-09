#include "status_check_watcher.hpp"

#include "loginapp.hpp"

#include "db/dbapp_interface.hpp"

#include "network/bundle.hpp"


BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: ReplyHandler
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
StatusCheckWatcher::ReplyHandler::ReplyHandler( StatusCheckWatcher & rWatcher,
		WatcherPathRequestV2 & pathRequest ) :
	rWatcher_( rWatcher ),
	pathRequest_( pathRequest )
{
}


/**
 *	This method handles the reply from the DBApp from the checkStatus request.
 */
void StatusCheckWatcher::ReplyHandler::handleMessage(
		const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg )
{
	// TODO: Should check that there are not too many arguments.

	bool value;
	BW::stringstream outputStream;

	data >> value;

	// Convert from a sequence of strings to one output string.
	while (!data.error() && (data.remainingLength() > 0))
	{
		BW::string line;
		data >> line;

		outputStream << line << std::endl;
	}

	const BW::string & output = outputStream.str();
	this->sendResult( value, output );

	delete this;
}


/**
 *	This method handles the failure case for the checkStatus request to DBApp.
 */
void StatusCheckWatcher::ReplyHandler::handleException(
		const Mercury::NubException & ne, void * arg )
{
	WARNING_MSG( "StatusCheckWatcher::ReplyHandler::handleException:\n" );
	this->sendResult( false, "No reply from DBApp\n" );
	delete this;
}


/**
 *	This method sends the reply to the watcher query.
 */
void StatusCheckWatcher::ReplyHandler::sendResult( bool status,
		const BW::string & output )
{
	BinaryOStream & resultStream = rWatcher_.startResultStream( pathRequest_,
		output, sizeof( bool ) );

	watcherValueToStream( resultStream, status, WT_READ_ONLY );

	rWatcher_.endResultStream( pathRequest_, NULL );
}


// -----------------------------------------------------------------------------
// Section: StatusCheckWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
StatusCheckWatcher::StatusCheckWatcher() :
	CallableWatcher( LOCAL_ONLY, "Check the status of the server" )
{
}


/**
 *	This method handles a set call on this watcher.
 */
bool StatusCheckWatcher::setFromStream( void * base,
		const char * path,
		WatcherPathRequestV2 & pathRequest )
{
	MF_ASSERT( base == NULL );

	pathRequest.getValueStream()->finish();

	LoginApp & app = LoginApp::instance();

	// TODO: Scalable DB: Should probably move overload status etc. to DBAppMgr.
	if (app.dbAppAlpha().addr() == Mercury::Address::NONE)
	{
		BinaryOStream & resultStream = 
			this->startResultStream( pathRequest, "No DBApp Alpha yet",
				sizeof( bool ) );
		watcherValueToStream( resultStream, false, WT_READ_ONLY );
		this->endResultStream( pathRequest, NULL );
		return true;
	}

	Mercury::Bundle & bundle = app.dbAppAlpha().bundle();
	bundle.startRequest( DBAppInterface::checkStatus,
		   new ReplyHandler( *this, pathRequest ) );
	app.dbAppAlpha().send();

	return true;
}

BW_END_NAMESPACE

// status_check_watcher.cpp
