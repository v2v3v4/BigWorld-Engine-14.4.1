#ifndef LOOK_UP_DBID_HANDLER_HPP
#define LOOK_UP_DBID_HANDLER_HPP

#include "db_storage/idatabase.hpp"

#include "network/udp_bundle.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class processes a request to retrieve the DBID of an entity from the
 * 	database.
 */
class LookUpDBIDHandler : public IDatabase::IGetDbIDHandler
{
public:
	LookUpDBIDHandler( const Mercury::Address& srcAddr,
			Mercury::ReplyID replyID ) :
		replyBundle_(),
		srcAddr_( srcAddr )
	{
		replyBundle_.startReply( replyID );
	}

	virtual ~LookUpDBIDHandler() {}

	void lookUpDBID( EntityTypeID typeID, const BW::string & name );

	// IDatabase::IGetDbIDHandler overrides
	virtual void onGetDbIDComplete( bool isOK, const EntityDBKey & entityKey );

private:
	Mercury::UDPBundle	replyBundle_;
	Mercury::Address	srcAddr_;
};

BW_END_NAMESPACE

#endif // LOOK_UP_DBID_HANDLER_HPP
