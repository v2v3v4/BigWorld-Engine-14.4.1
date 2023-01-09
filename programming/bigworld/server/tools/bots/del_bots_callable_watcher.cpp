#include "script/first_include.hpp"

#include "del_bots_callable_watcher.hpp"

#include "main_app.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DelBotsCallableWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
DelBotsCallableWatcher::DelBotsCallableWatcher() :
	SimpleCallableWatcher( LOCAL_ONLY, "Delete bots from this process" )
{
	this->addArg( WATCHER_TYPE_INT, "Number of bots to delete" );
}


/**
 *	This method checks in case we got a set message with an int instead
 *	of a call message with a tuple, for backwards compatibility with
 *	WebConsole and PyCommon in pre-BW2.6 releases.
 */
bool DelBotsCallableWatcher::setFromStream( void * base, const char * path,
	WatcherPathRequestV2 & pathRequest )
{
	BinaryIStream * pInput = pathRequest.getValueStream();

	if (!Watcher::isEmptyPath( path ) || !pInput || pInput->error())
	{
		return this->SimpleCallableWatcher::setFromStream( base, path,
			pathRequest );
	}

	char type = pInput->peek();

	if ((WatcherDataType)type == WATCHER_TYPE_TUPLE)
	{
		return this->SimpleCallableWatcher::setFromStream( base, path,
			pathRequest );
	}

	// It's not a tuple, so it must be an older caller.
	// The below code is derived from MemberWatcher::setFromStream
	int botsToDelete;
	if (!watcherStreamToValue( *pInput, botsToDelete ))
	{
		return false;
	}

	MainApp::instance().delBots( botsToDelete );

	// push the result into the reply stream
	Watcher::Mode mode = Watcher::WT_READ_WRITE;
	watcherValueToStream( pathRequest.getResultStream(), botsToDelete, mode );
	pathRequest.setResult( "", mode, this, base );

	return true;
}


bool DelBotsCallableWatcher::onCall( BW::string & output, BW::string & value,
	int parameterCount, BinaryIStream & parameters )
{
	if (parameterCount != 1)
	{
		ERROR_MSG( "DelBotsCallableWatcher:onCall: "
				"Got %d parameters, expecting 1\n", parameterCount );
		return false;
	}

	int botsToDelete;

	if (!watcherStreamToValue( parameters, botsToDelete ))
	{
		ERROR_MSG( "DelBotsCallableWatcher:onCall: "
				"Failed to read parameter\n" );
		return false;
	}

	MainApp::instance().delBots( botsToDelete );

	return true;
}


BW_END_NAMESPACE

// add_bots_callable_watcher.cpp
