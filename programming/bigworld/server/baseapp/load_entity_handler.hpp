#ifndef LOAD_ENTITY_HANDLER_HPP
#define LOAD_ENTITY_HANDLER_HPP

#include "network/basictypes.hpp"
#include "network/interfaces.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

class Base;

// -----------------------------------------------------------------------------
// Section: Load from DB status
// -----------------------------------------------------------------------------
static const uint8 LOAD_FROM_DB_FAILED			= 0;
static const uint8 LOAD_FROM_DB_SUCCEEDED		= 1;
static const uint8 LOAD_FROM_DB_FOUND_EXISTING = 2;


/**
 *	This class is used to handle reply to a loadEntity message is received from
 *	DBApp.
 */
class LoadEntityHandler : public Mercury::ReplyMessageHandler
{
public:
	LoadEntityHandler( EntityTypeID entityTypeID, EntityID entityID );
	virtual ~LoadEntityHandler() {}

private:
	void handleMessage(const Mercury::Address& srcAddr,
		Mercury::UnpackedMessageHeader& /*header*/,
		BinaryIStream& data, void * /*arg*/);

	void handleException(const Mercury::NubException& /*ne*/, void* /*arg*/);
	void handleShuttingDown(const Mercury::NubException& /*ne*/, void* /*arg*/);

	// Derived classes can override this method to do some action on
	// completion. If pBase is non-NULL then the entity was successfully
	// loaded from database. If pMailBox is non-NULL then the entity was
	// already checked out and the mailbox points to the existing entity.
	// dbID is the database ID of the entity.
	// If both pBase and pMailbox is NULL then load operation failed and
	// dbID will be 0.
	// Only one of pBase or pMailbox can be non-NULL.
	virtual void onLoadedFromDB( Base * pBase = NULL,
			EntityMailBoxRef * pMailbox = NULL, DatabaseID dbID = 0 ) {}

	void unCheckoutEntity( DatabaseID databaseID );

	EntityTypeID		entityTypeID_;
	EntityID			entityID_;
};

/**
 *	This class is a LoadEntityHandler but calls a Python callback function
 *  on completion.
 */
class LoadEntityHandlerWithCallback : public LoadEntityHandler
{
public:
	LoadEntityHandlerWithCallback( PyObjectPtr pResultHandler,
			EntityTypeID entityTypeID, EntityID entityID );

	// LoadEntityHandler overrides
	virtual void onLoadedFromDB( Base* pBase = NULL,
			EntityMailBoxRef* pMailbox = NULL, DatabaseID dbID = 0 );

private:
	PyObjectPtr	pResultHandler_;
};


/**
 *	This class is a LoadEntityHandler but replies to a request on completion.
 */
class LoadEntityHandlerWithReply : public LoadEntityHandler
{
public:
	LoadEntityHandlerWithReply( Mercury::ReplyID replyID,
			const Mercury::Address& srcAddr,
			EntityTypeID entityTypeID, EntityID entityID );

	// LoadEntityHandler overrides
	virtual void onLoadedFromDB( Base* pBase = NULL,
		EntityMailBoxRef* pMailbox = NULL, DatabaseID dbID = 0 );

private:
	Mercury::ReplyID		replyID_;
	const Mercury::Address  srcAddr_;
};

BW_END_NAMESPACE

#endif // LOAD_ENTITY_HANDLER_HPP
