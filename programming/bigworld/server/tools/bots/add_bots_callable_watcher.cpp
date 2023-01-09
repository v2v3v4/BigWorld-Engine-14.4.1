#include "script/first_include.hpp"

#include "add_bots_callable_watcher.hpp"

#include "main_app.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: AddBotsCallableWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
AddBotsCallableWatcher::AddBotsCallableWatcher() :
	SimpleCallableWatcher( LOCAL_ONLY, "Add bots to this process" )
{
	this->addArg( WATCHER_TYPE_INT, "Number of bots to add" );
}


/**
 *	This method checks in case we got a set message with an int instead
 *	of a call message with a tuple, for backwards compatibility with
 *	WebConsole and PyCommon in pre-BW2.6 releases.
 */
bool AddBotsCallableWatcher::setFromStream( void * base, const char * path,
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
	int botsToAdd;
	if (!watcherStreamToValue( *pInput, botsToAdd ))
	{
		return false;
	}

	MainApp::instance().addBots( botsToAdd );

	// push the result into the reply stream
	Watcher::Mode mode = Watcher::WT_READ_WRITE;
	watcherValueToStream( pathRequest.getResultStream(), botsToAdd, mode );
	pathRequest.setResult( "", mode, this, base );

	return true;
}


bool AddBotsCallableWatcher::onCall( BW::string & output, BW::string & value,
	int parameterCount, BinaryIStream & parameters )
{
	if (parameterCount != 1)
	{
		ERROR_MSG( "AddBotsCallableWatcher:onCall: "
				"Got %d parameters, expecting 1\n", parameterCount );
		return false;
	}

	int botsToAdd;

	if (!watcherStreamToValue( parameters, botsToAdd ))
	{
		ERROR_MSG( "AddBotsCallableWatcher:onCall: "
				"Failed to read parameter\n" );
		return false;
	}

	MainApp::instance().addBots( botsToAdd );

	return true;
}


BW_END_NAMESPACE

// add_bots_callable_watcher.cpp
