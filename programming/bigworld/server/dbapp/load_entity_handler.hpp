#ifndef LOAD_ENTITY_HANDLER_HPP
#define LOAD_ENTITY_HANDLER_HPP

#include "dbapp.hpp" // For ICheckoutCompletionListener
#include "get_entity_handler.hpp"

#include "network/udp_bundle.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class is used by DBApp::loadEntity() to load an entity from
 *	the database and wait for the results.
 */
class LoadEntityHandler : public GetEntityHandler,
                          public IDatabase::IPutEntityHandler,
                          public DBApp::ICheckoutCompletionListener
{
public:
	LoadEntityHandler( const EntityDBKey & ekey,
			const Mercury::Address & srcAddr, EntityID entityID,
			Mercury::ReplyID replyID ) :
		ekey_( ekey ),
		baseRef_(),
		srcAddr_( srcAddr ),
		entityID_( entityID ),
		replyID_( replyID ),
		replyBundle_(),
		pStrmDbID_( NULL )
	{}
	virtual ~LoadEntityHandler() {}

	void loadEntity();

	// IDatabase::IGetEntityHandler/GetEntityHandler overrides
	virtual void onGetEntityCompleted( bool isOK,
		const EntityDBKey & entityKey,
		const EntityMailBoxRef * pBaseEntityLocation );

	// IDatabase::IPutEntityHandler override
	virtual void onPutEntityComplete( bool isOK, DatabaseID );

	// DBApp::ICheckoutCompletionListener override
	virtual void onCheckoutCompleted( const EntityMailBoxRef* pBaseRef );

private:
	void sendAlreadyCheckedOutReply( const EntityMailBoxRef& baseRef );

	EntityDBKey			ekey_;
	EntityMailBoxRef	baseRef_;

	Mercury::Address	srcAddr_;
	EntityID			entityID_;
	Mercury::ReplyID	replyID_;

	Mercury::UDPBundle	replyBundle_;

	DatabaseID*			pStrmDbID_;
};

BW_END_NAMESPACE

#endif // LOAD_ENTITY_HANDLER_HPP
