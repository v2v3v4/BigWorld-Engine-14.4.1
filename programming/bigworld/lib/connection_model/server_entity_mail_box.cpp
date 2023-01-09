#include "pch.hpp"

#include "server_entity_mail_box.hpp"

#include "bw_entity.hpp"
#include "bw_connection.hpp"

#include "connection/server_connection.hpp"

BW_BEGIN_NAMESPACE

int ServerEntityMailBox_Token = 0;

/**
 *	Constructor.
 */
ServerEntityMailBox::ServerEntityMailBox( BWEntity & entity ) :
		entity_( entity )
{
}


/**
 *	This method starts a message with the given message ID for either the cell
 *	entity or the base entity.
 *
 *	@param methodID			The numeric method ID.
 *	@param isForBaseEntity 	If true, this message is destined for the base
 *							entity, otherwise, it is destined for the cell
 *							entity.
 */
BinaryOStream * ServerEntityMailBox::startMessage( int methodID,
		bool isForBaseEntity, bool & shouldDrop ) const
{
	shouldDrop = false;

	BWConnection * pBWConnection = entity_.pBWConnection();

	if (pBWConnection == NULL)
	{
		WARNING_MSG( "ServerEntityMailBox::startMessage: "
				"Trying to start message %d on entity %d of type %s that "
				"does not have a connection.\n",
			methodID, entity_.entityID(), entity_.entityTypeName().c_str() );

		return NULL;
	}

	return pBWConnection->addServerMessage( entity_.entityID(), methodID,
		isForBaseEntity, shouldDrop );
}

BW_END_NAMESPACE

// server_entity_mail_box.cpp

