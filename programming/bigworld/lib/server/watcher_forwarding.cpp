#include "watcher_forwarding.hpp"

#include "cstdmf/watcher_path_request.hpp"
#include "cstdmf/watcher.hpp"
#include <limits.h>


BW_BEGIN_NAMESPACE

// ForwardingWatcher static member initialisation
const BW::string ForwardingWatcher::TARGET_CELL_APPS = "cellApps";
const BW::string ForwardingWatcher::TARGET_BASE_APPS = "baseApps";
const BW::string ForwardingWatcher::TARGET_SERVICE_APPS = "serviceApps";
const BW::string ForwardingWatcher::TARGET_BASE_SERVICE_APPS = "baseServiceApps";
const BW::string ForwardingWatcher::TARGET_LEAST_LOADED = "leastLoaded";


/**
 * Extracts a list of component ID numbers from a provided string of comma
 * seperated IDs.
 *
 * @param targetInfo The string of comma sperated component ID's to extract.
 *
 * @returns A list of component ID's that have been extracted.
 */
ComponentIDList ForwardingWatcher::getComponentIDList(
	const BW::string & targetInfo )
{
	char *targetDup = strdup( targetInfo.c_str() );
	ComponentIDList components;
	const char *TOKEN_SEP = ",";

	if (targetDup != NULL)
	{

		char *endPtr;
		long int result;
		char *curr = strtok(targetDup, TOKEN_SEP);
		while (curr != NULL)
		{
			result = strtol(curr, &endPtr, 10);
			if ((result != LONG_MIN) && (result != LONG_MAX) &&
				(*endPtr == '\0'))
			{
				// Conversion worked so add the component ID to the list.
				components.push_back( (int32)result );
			}
			else
			{
				ERROR_MSG( "ForwardingWatcher::getComponentList: strtol "
							"failed to convert component ID '%s'.\n", curr );
			}

			curr = strtok(NULL, TOKEN_SEP);
		}

		free( targetDup );
	}

	return components;
}


// Overridden from Watcher
bool ForwardingWatcher::setFromStream( void * base, const char * path,
		WatcherPathRequestV2 & pathRequest )
{
	// Our watcher path should now look like;
	// 'all/command/addBot'
	// 'leastLoaded/command/addBot'
	// '1,2,45,87/command/addBot'
	// 
	// Separate on the first '/'
	BW::string pathStr = path;
	size_t pos = pathStr.find( '/' );
	if (pos == (size_t)-1)
	{
		ERROR_MSG( "ForwardingWatcher::setFromStream: Appear to have received "
					"a bad destination path '%s'.\n", path );
		return false;
	}

	// If the separator was the final thing in the string then there is no
	// remaining watcher path to use on the receiving component side.
	if ((pos+1) >= pathStr.size())
	{
		ERROR_MSG( "ForwardingWatcher::setFromStream: No destination watcher "
					"path provided '%s'.\n", path );
		return false;
	}

	// Now generate a new string containing the destination target(s)
	BW::string targetInfo = pathStr.substr( 0, pos );
	BW::string destWatcher = pathStr.substr( pos+1, pathStr.size() );

	// Specialised collector initialisation
	ForwardingCollector *collector = this->newCollector(
		pathRequest, destWatcher, targetInfo );

	bool status = false;
	if (collector)
	{
		status = collector->start();

		// If the collector didn't start correctly, it won't be receiving
		// any responses which will enable it to clean itself up, so do it now
		if (!status)
			delete collector;
	}

	return status;
}


// Overridden from Watcher
bool ForwardingWatcher::getAsStream( const void * base, const char * path,
		WatcherPathRequestV2 & pathRequest ) const
{
	if (isEmptyPath(path))
	{
		Watcher::Mode mode = Watcher::WT_READ_ONLY;
		watcherValueToStream( pathRequest.getResultStream(),
							  "Forwarding Watcher", mode );
		pathRequest.setResult( comment_, mode, this, base );
		return true;
	}
	else if (isDocPath( path ))
	{
		Watcher::Mode mode = Watcher::WT_READ_ONLY;
		watcherValueToStream( pathRequest.getResultStream(), comment_, mode );
		pathRequest.setResult( comment_, mode, this, base );
		return true;
	}

	return false;
}

BW_END_NAMESPACE

// watcher_forwarder.cpp
