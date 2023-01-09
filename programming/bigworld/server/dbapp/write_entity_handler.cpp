#include "script/first_include.hpp"

#include "write_entity_handler.hpp"

#include "dbapp.hpp"

#include "network/channel_sender.hpp"
#include "server/writedb.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
WriteEntityHandler::WriteEntityHandler( const EntityDBKey ekey,
			EntityID entityID, WriteDBFlags flags, Mercury::ReplyID replyID,
			const Mercury::Address & srcAddr ) :
	ekey_( ekey ),
	entityID_( entityID ),
	flags_( flags ),
	shouldReply_( replyID != Mercury::REPLY_ID_NONE ),
	replyID_( replyID ),
	srcAddr_( srcAddr )
{
}


/**
 *	This method writes the entity data into the database.
 *
 *	@param	data	Stream should be currently at the start of the entity's
 *	data.
 *	@param	entityID	The entity's base mailbox object ID.
 */
void WriteEntityHandler::writeEntity( BinaryIStream & data, EntityID entityID )
{
	BinaryIStream * pStream = NULL;

	UpdateAutoLoad updateAutoLoad = 
		(flags_ & WRITE_AUTO_LOAD_YES) 	? 	UPDATE_AUTO_LOAD_TRUE :
		(flags_ & WRITE_AUTO_LOAD_NO) 	? 	UPDATE_AUTO_LOAD_FALSE:
											UPDATE_AUTO_LOAD_RETAIN;

	if (flags_ & WRITE_BASE_CELL_DATA)
	{
		pStream = &data;
	}

	if (flags_ & WRITE_LOG_OFF)
	{
		this->putEntity( pStream, updateAutoLoad,
			/* pBaseMailbox: */ NULL,
			/* removeBaseMailbox: */ true );
	}
	else if (ekey_.dbID == 0 ||(flags_ & WRITE_EXPLICIT_DBID))
	{
		// New entity is checked out straight away
		baseRef_.init( entityID, srcAddr_, EntityMailBoxRef::BASE,
			ekey_.typeID );
		this->putEntity( pStream, updateAutoLoad, &baseRef_ );
	}
	else
	{
		this->putEntity( pStream, updateAutoLoad );
	}
	// When putEntity() completes onPutEntityComplete() is called.
}


/**
 *	IDatabase::IPutEntityHandler override
 */
void WriteEntityHandler::onPutEntityComplete( bool isOK, DatabaseID dbID )
{
	bool isFirstWrite = (ekey_.dbID == 0 || (flags_ & WRITE_EXPLICIT_DBID));
	ekey_.dbID = dbID;

	if (!isOK)
	{
		ERROR_MSG( "WriteEntityHandler::writeEntity: "
						"Failed to update entity %" FMT_DBID " of type %d.\n",
					dbID, ekey_.typeID );
	}

	this->finalise( isOK, isFirstWrite );
}


/**
 *	Deletes the entity from the database.
 */
void WriteEntityHandler::deleteEntity()
{
	MF_ASSERT( flags_ & WRITE_DELETE_FROM_DB );
	DBApp::instance().delEntity( ekey_, entityID_, *this );
	// When delEntity() completes, onDelEntityComplete() is called.
}


/**
 *	IDatabase::IDelEntityHandler override
 */
void WriteEntityHandler::onDelEntityComplete( bool isOK )
{
	if (!isOK)
	{
		ERROR_MSG( "WriteEntityHandler::writeEntity: "
						"Failed to delete entity %" FMT_DBID " of type %d.\n",
					ekey_.dbID, ekey_.typeID );
	}

	this->finalise(isOK);
}


/**
 *	This method is invoked by WriteEntityHandler::writeEntity to pass through
 *	a putEntity request to the database implementation.
 */
void WriteEntityHandler::putEntity( BinaryIStream * pStream,
			UpdateAutoLoad updateAutoLoad,
			EntityMailBoxRef * pBaseMailbox,
			bool removeBaseMailbox )
{
	DBApp::instance().putEntity( ekey_, entityID_,
			pStream, pBaseMailbox, removeBaseMailbox, 
			flags_ & WRITE_EXPLICIT_DBID, 
			updateAutoLoad, *this );
}


/**
 *	This function does some common stuff at the end of the operation.
 */
void WriteEntityHandler::finalise( bool isOK, bool isFirstWrite )
{
	if (shouldReply_)
	{
		Mercury::ChannelSender sender( DBApp::getChannel( srcAddr_ ) );
		sender.bundle().startReply( replyID_ );
		sender.bundle() << isOK << ekey_.dbID;
	}

	if (isOK)
	{
		DBApp & dbApp = DBApp::instance();

		if (flags_ & WRITE_LOG_OFF)
		{
			dbApp.onEntityLogOff( ekey_.typeID, ekey_.dbID );
		}

		if (dbApp.shouldCacheLogOnRecords())
		{
			if (flags_ & WRITE_LOG_OFF)
			{
				dbApp.logOnRecordsCache().erase( ekey_ );
			}
			else if (isFirstWrite)
			{
				dbApp.logOnRecordsCache().insert( ekey_, baseRef_ );
			}
		}
	}

	delete this;
}

BW_END_NAMESPACE

// write_entity_handler.cpp
