#include "script/first_include.hpp"

#include "update_movement_callable_watcher.hpp"

#include "main_app.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: UpdateMovementCallableWatcher
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
UpdateMovementCallableWatcher::UpdateMovementCallableWatcher() :
	SimpleCallableWatcher( LOCAL_ONLY,
		"Update the movement controller of all bots int this process with "
			"the given tag" )
{
	this->addArg( WATCHER_TYPE_STRING,
		"Tag of bots to update, or empty for all" );
}


/**
 *	This method checks in case we got a set message with a string instead
 *	of a call message with a tuple, for backwards compatibility with
 *	WebConsole and PyCommon in pre-BW2.6 releases.
 */
bool UpdateMovementCallableWatcher::setFromStream( void * base,
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
	BW::string tagToUpdate;
	if (!watcherStreamToValue( *pInput, tagToUpdate ))
	{
		return false;
	}

	MainApp::instance().updateMovement( tagToUpdate );

	// push the result into the reply stream
	Watcher::Mode mode = Watcher::WT_READ_WRITE;
	watcherValueToStream( pathRequest.getResultStream(), tagToUpdate, mode );
	pathRequest.setResult( "", mode, this, base );

	return true;
}


bool UpdateMovementCallableWatcher::onCall( BW::string & output,
	BW::string & value, int parameterCount, BinaryIStream & parameters )
{
	if (parameterCount != 1)
	{
		ERROR_MSG( "UpdateMovementCallableWatcher:onCall: "
				"Got %d parameters, expecting 1\n", parameterCount );
		return false;
	}

	BW::string tagToUpdate;

	if (!watcherStreamToValue( parameters, tagToUpdate ))
	{
		ERROR_MSG( "UpdateMovementCallableWatcher:onCall: "
				"Failed to read parameter\n" );
		return false;
	}

	MainApp::instance().updateMovement( tagToUpdate );

	return true;
}


BW_END_NAMESPACE

// add_bots_callable_watcher.cpp
