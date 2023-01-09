#ifndef DELETE_ENTITY_HANDLER_HPP
#define DELETE_ENTITY_HANDLER_HPP

#include "get_entity_handler.hpp"

#include "network/udp_bundle.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class processes a request to delete an entity from the database.
 */
class DeleteEntityHandler : public GetEntityHandler,
							public IDatabase::IDelEntityHandler
{
public:
	DeleteEntityHandler( EntityTypeID typeID, DatabaseID dbID,
		const Mercury::Address& srcAddr, Mercury::ReplyID replyID );
	virtual ~DeleteEntityHandler() {}

	void deleteEntity();

	// GetEntityHandler override
	virtual void onGetEntityCompleted( bool isOK,
		const EntityDBKey & entityKey,
		const EntityMailBoxRef * pBaseEntityLocation );

	// IDatabase::IDelEntityHandler override
	virtual void onDelEntityComplete( bool isOK );

private:
	void sendReply();
	void addFailure();

	Mercury::UDPBundle	replyBundle_;
	Mercury::Address	srcAddr_;
	EntityDBKey			ekey_;
};

BW_END_NAMESPACE

#endif // DELETE_ENTITY_HANDLER_HPP
