#ifndef DB_APP_HPP
#define DB_APP_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/watcher.hpp"

#include "db/dbapp_interface.hpp"

#include "network/channel_owner.hpp"


BW_BEGIN_NAMESPACE

class DBApp;
typedef SmartPointer< DBApp > DBAppPtr;

/**
 *	This class represents the DBAppMgr's view of a DBApp.
 */
class DBApp: public ReferenceCount
{
public:
	DBApp( Mercury::NetworkInterface & interface, 
		const Mercury::Address & addr, DBAppID id,
		Mercury::ReplyID addReplyID );

	DBAppID id() const	{ return id_; }
	void id( int id )	{ id_ = id; }

	const Mercury::Address & address() const { return channelOwner_.addr(); }
	Mercury::ChannelOwner & channelOwner() { return channelOwner_; }

	template< typename HASH >
	void updateDBAppHash( const HASH & dbApps,
		bool shouldAlphaResetGameServerState );

	void controlledShutDown( ShutDownStage stage = SHUTDOWN_REQUEST );

	static WatcherPtr pWatcher();

	// Message handlers
	void retireApp();

private:
	DBAppID					id_;
	Mercury::ChannelOwner	channelOwner_;
	Mercury::ReplyID 		pendingReplyID_;
};


/**
 *	This method sends an update for the given DBApp hash to this DBApp.
 *
 *	@param hash 		The hash to send.
 *	@param shouldAlphaResetGameServerState
 *						For the DBApp-Alpha, if this is true, then it should
 *						reset the game server state, otherwise it has no
 *						effect if false or for non-Alpha DBApps.
 */
template< typename HASH >
void DBApp::updateDBAppHash( const HASH & hash,
		bool shouldAlphaResetGameServerState )
{
	Mercury::Bundle & bundle = channelOwner_.channel().bundle();

	if (pendingReplyID_ != Mercury::REPLY_ID_NONE)
	{
		bundle.startReply( pendingReplyID_ );
		pendingReplyID_ = Mercury::REPLY_ID_NONE;
		bundle << id_;
	}
	else
	{
		bundle.startMessage( DBAppInterface::updateDBAppHash );
	}

	bundle << uint8( shouldAlphaResetGameServerState );
	bundle << hash;

	channelOwner_.channel().send();
}


BW_END_NAMESPACE

#endif // DB_APP_HPP
