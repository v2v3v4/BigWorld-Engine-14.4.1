#include "script/first_include.hpp"

#include "del_tagged_bots_callable_watcher.hpp"

#include "main_app.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DelTaggedBotsCallableWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
DelTaggedBotsCallableWatcher::DelTaggedBotsCallableWatcher() :
	SimpleCallableWatcher( LOCAL_ONLY,
		"Delete all bots from this process with the given tag" )
{
	this->addArg( WATCHER_TYPE_STRING, "Tag of bots to delete" );
}


/**
 *	This method checks in case we got a set message with a string instead
 *	of a call message with a tuple, for backwards compatibility with
 *	WebConsole and PyCommon in pre-BW2.6 releases.
 */
bool DelTaggedBotsCallableWatcher::setFromStream( void * base,
	const char * path, WatcherPathRequestV2 & pathRequest )
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
	BW::string tagToDelete;
	if (!watcherStreamToValue( *pInput, tagToDelete ))
	{
		return false;
	}

	MainApp::instance().delTaggedEntities( tagToDelete );

	// push the result into the reply stream
	Watcher::Mode mode = Watcher::WT_READ_WRITE;
	watcherValueToStream( pathRequest.getResultStream(), tagToDelete, mode );
	pathRequest.setResult( "", mode, this, base );

	return true;
}


bool DelTaggedBotsCallableWatcher::onCall( BW::string & output,
	BW::string & value, int parameterCount, BinaryIStream & parameters )
{
	if (parameterCount != 1)
	{
		ERROR_MSG( "DelTaggedBotsCallableWatcher:onCall: "
				"Got %d parameters, expecting 1\n", parameterCount );
		return false;
	}

	BW::string tagToDelete;

	if (!watcherStreamToValue( parameters, tagToDelete ))
	{
		ERROR_MSG( "DelTaggedBotsCallableWatcher:onCall: "
				"Failed to read parameter\n" );
		return false;
	}

	MainApp::instance().delTaggedEntities( tagToDelete );

	return true;
}


BW_END_NAMESPACE

// add_bots_callable_watcher.cpp
