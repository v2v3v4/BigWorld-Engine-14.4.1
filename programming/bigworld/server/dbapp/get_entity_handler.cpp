#include "script/first_include.hpp"

#include "get_entity_handler.hpp"

#include "dbapp.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This method intercepts the result of IDatabase::getEntity() operations and
 *	mucks around with it before passing it to onGetEntityCompleted().
 */
void GetEntityHandler::onGetEntityComplete( bool isOK,
					const EntityDBKey & entityKey,
					const EntityMailBoxRef * pBaseEntityLocation )
{
	// Update mailbox for dead BaseApps.
	if (DBApp::instance().hasMailboxRemapping() &&
			pBaseEntityLocation)
	{
		EntityMailBoxRef remappedLocation = *pBaseEntityLocation;

		DBApp::instance().remapMailbox( remappedLocation );

		this->onGetEntityCompleted( isOK, entityKey, &remappedLocation );
	}
	else
	{
		this->onGetEntityCompleted( isOK, entityKey, pBaseEntityLocation );
	}
}


/**
 *	This method checks Checks that pBaseRef is fully checked out i.e. not in
 *	"pending base creation" state.
 */
bool GetEntityHandler::isActiveMailBox( const EntityMailBoxRef * pBaseRef )
{
	if (pBaseRef && pBaseRef->id != 0)
	{
		return (pBaseRef->addr.ip != 0);
	}

	return false;
}

BW_END_NAMESPACE

// get_entity_handler.cpp
