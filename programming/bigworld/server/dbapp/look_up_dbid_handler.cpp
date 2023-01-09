#include "script/first_include.hpp"

#include "look_up_dbid_handler.hpp"

#include "dbapp.hpp"

#include "db_storage/db_entitydefs.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Starts the process of looking up the DBID.
 */
void LookUpDBIDHandler::lookUpDBID( EntityTypeID typeID,
		const BW::string & name )
{
	if (DBApp::instance().getEntityDefs().isValidEntityType( typeID ))
	{
		EntityDBKey entityKey( typeID, 0, name );
		DBApp::instance().getIDatabase().getDatabaseIDFromName(
			entityKey, *this );
	}
	else
	{
		ERROR_MSG( "LookUpDBIDHandler::lookUpDBID: Invalid entity type %d\n",
				typeID );
		replyBundle_ << DatabaseID( 0 );
		DBApp::getChannel( srcAddr_ ).send( &replyBundle_ );
		delete this;
	}
}


/**
 *	GetEntityHandler override.
 */
void LookUpDBIDHandler::onGetDbIDComplete( bool isOK,
		const EntityDBKey & entityKey )
{
	replyBundle_ << entityKey.dbID;
	DBApp::getChannel( srcAddr_ ).send( &replyBundle_ );

	delete this;
}

BW_END_NAMESPACE

// look_up_dbid_handler.cpp
