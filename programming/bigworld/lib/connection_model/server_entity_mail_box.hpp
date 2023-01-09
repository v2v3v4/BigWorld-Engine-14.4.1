#ifndef SERVER_ENTITY_MAIL_BOX_HPP
#define SERVER_ENTITY_MAIL_BOX_HPP

#include "bwentity/bwentity_api.hpp"
#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class BWEntity;
class ServerConnection;


/**
 *	This class is the base class for all client-side mailboxes referring to
 *	server-side entities.
 */
class BWENTITY_API ServerEntityMailBox 
{
public:
	ServerEntityMailBox( BWEntity & entity );

protected:
	BinaryOStream * startMessage( int methodID, bool isForBaseEntity,
		bool & shouldDrop ) const;

	BWEntity & entity_;
};

BW_END_NAMESPACE

#endif // SERVER_ENTITY_MAIL_BOX_HPP

