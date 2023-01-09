#ifndef WRITE_ENTITY_HANDLER_HPP
#define WRITE_ENTITY_HANDLER_HPP

#include "db_storage/idatabase.hpp"

#include "network/misc.hpp"
#include "server/writedb.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used by DBApp::writeEntity() to write entities into
 *	the database and wait for the result.
 */
class WriteEntityHandler : public IDatabase::IPutEntityHandler,
                           public IDatabase::IDelEntityHandler
{

public:
	WriteEntityHandler( const EntityDBKey ekey, EntityID entityID,
			WriteDBFlags flags, Mercury::ReplyID replyID,
			const Mercury::Address & srcAddr );

	void writeEntity( BinaryIStream & data, EntityID entityID );

	void deleteEntity();

	// IDatabase::IPutEntityHandler override
	virtual void onPutEntityComplete( bool isOK, DatabaseID );

	// IDatabase::IDelEntityHandler override
	virtual void onDelEntityComplete( bool isOK );

private:
	void putEntity( BinaryIStream * pStream,
			UpdateAutoLoad updateAutoLoad,
			EntityMailBoxRef * pBaseMailbox = NULL,
			bool removeBaseMailbox = false );

	void finalise( bool isOK, bool isFirstWrite = false );

private:
	EntityDBKey				ekey_;
	EntityMailBoxRef		baseRef_;
	EntityID				entityID_;
	WriteDBFlags			flags_;
	bool					shouldReply_;
	Mercury::ReplyID		replyID_;
	const Mercury::Address	srcAddr_;
};

BW_END_NAMESPACE

#endif // WRITE_ENTITY_HANDLER_HPP
