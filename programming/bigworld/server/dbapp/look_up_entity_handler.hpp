#ifndef LOOK_UP_ENTITY_HANDLER_HPP
#define LOOK_UP_ENTITY_HANDLER_HPP

#include "get_entity_handler.hpp"

#include "network/udp_bundle.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class processes a request to retrieve the base mailbox of a
 *	checked-out entity from the database.
 */
class LookUpEntityHandler : public GetEntityHandler, IDatabase::IGetDbIDHandler
{
public:
	LookUpEntityHandler( const Mercury::Address& srcAddr,
			Mercury::ReplyID replyID );
	virtual ~LookUpEntityHandler() {}

	void lookUpEntity( EntityTypeID typeID, const BW::string & name )
	{
		EntityDBKey entityKey( typeID, 0, name );
		this->lookUpEntity( entityKey );
	}

	void lookUpEntity( EntityTypeID typeID, DatabaseID databaseID )
	{
		EntityDBKey entityKey( typeID, databaseID );
		this->lookUpEntity( entityKey );
	}

	// IDatabase::IGetEntityHandler/GetEntityHandler overrides
	virtual void onGetEntityCompleted( bool isOK,
		const EntityDBKey & entityKey,
		const EntityMailBoxRef * pBaseEntityLocation );

	// IDatabase::IGetDbIDHandler overrides
        virtual void onGetDbIDComplete( bool isOK, const EntityDBKey & entityKey );

private:
	void lookUpEntity( const EntityDBKey & entityKey );

	void sendReply( bool hasError,
			const EntityMailBoxRef * pBaseEntityLocation );

	void writeReply( bool hasError,
			const EntityMailBoxRef * pBaseEntityLocation,
			Mercury::Bundle & bundle );

	Mercury::ReplyID	replyID_;
	Mercury::Address	srcAddr_;
};

BW_END_NAMESPACE

#endif // LOOK_UP_ENTITY_HANDLER_HPP
