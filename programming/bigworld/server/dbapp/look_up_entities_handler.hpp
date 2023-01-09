#ifndef LOOK_UP_ENTITIES_HANDLER_HPP
#define LOOK_UP_ENTITIES_HANDLER_HPP

#include "db_storage/idatabase.hpp"

#include "network/udp_bundle.hpp"


BW_BEGIN_NAMESPACE

class DBApp;

/**
 *	This class processes a request to retrieve the base mailbox of a
 *	checked-out entity from the database.
 */
class LookUpEntitiesHandler : public IDatabase::ILookUpEntitiesHandler
{
public:
	LookUpEntitiesHandler( DBApp & dbApp, 
			const Mercury::Address & srcAddr,
			Mercury::ReplyID replyID );

	virtual ~LookUpEntitiesHandler() {}

	void lookUpEntities( EntityTypeID entityTypeID, 
		const LookUpEntitiesCriteria & criteria );

	// IDatabase::ILookUpEntitiesHandler overrides

	virtual void onLookUpEntitiesStart( size_t numEntities );
	virtual void onLookedUpEntity( DatabaseID databaseID,
			const EntityMailBoxRef & baseEntityLocation );

	virtual void onLookUpEntitiesEnd( bool hasError = false );

private:
	void sendReply();

	DBApp & 			dbApp_;
	Mercury::UDPBundle	replyBundle_;
	Mercury::Address	srcAddr_;
};

BW_END_NAMESPACE

#endif // LOOK_UP_ENTITIES_HANDLER_HPP
