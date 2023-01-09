#ifndef PROXY_HPP
#define PROXY_HPP

#include "base.hpp"
#include "baseapp_int_interface.hpp"
#include "data_downloads.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/client_interface.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/concurrency.hpp"

#include "math/ema.hpp"

#include "network/channel.hpp"

#include "pyscript/stl_to_py.hpp"

#include "mailbox.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class DeferredBundle;
} // end namespace Mercury

class BaseEntityMailBox;
class BufferedMessage;
class ClientEntityMailBox;
class DownloadStreamer;
class EventHistoryStats;
class MemoryOStream;
class PendingReLogOn;
class ProxyPusher;
class RateLimitMessageFilter;

typedef SmartPointer< RateLimitMessageFilter > RateLimitMessageFilterPtr;

/**
 *	Reason codes for client disconnection.
 */
enum ClientDisconnectReason
{
	CLIENT_DISCONNECT_CLIENT_REQUESTED,
	CLIENT_DISCONNECT_GIVEN_TO_OTHER_PROXY,
	CLIENT_DISCONNECT_RATE_LIMITS_EXCEEDED,
	CLIENT_DISCONNECT_TIMEOUT,
	CLIENT_DISCONNECT_BASE_RESTORE,
	CLIENT_DISCONNECT_SHUTDOWN,
	CLIENT_DISCONNECT_CELL_RESTORE_FAILED,

	// If changing this enum, also update
	// clientDisconnectReasonToString() in proxy.cpp

	NUM_CLIENT_DISCONNECT_REASONS
};

/*~ class BigWorld.Proxy
 *	@components{ base }
 *
 *	The Proxy is a special type of Base that has an associated Client. As such,
 *	it handles all the server updates for that Client. There is no direct script
 *	call to create a Proxy specifically.
 *
 */

/**
 *	This class is used to represent a proxy. A proxy is a special type of base.
 *	It has an associated client.
 */
class Proxy: public Base
{
	Py_Header( Proxy, Base )

public:
	Proxy( EntityID id, DatabaseID dbID, EntityType * pType );
	~Proxy();

 	void onClientDeath( ClientDisconnectReason reason,
		bool shouldExpectClient = true );

	void onDestroy( bool isOffload = false );

	void restoreClient();

	void writeBackupData( BinaryOStream & stream );
	void offload( const Mercury::Address & dstAddr );

	void transferClient( const Mercury::Address & dstAddr, bool shouldReset,
		Mercury::ReplyMessageHandler * pHandler = NULL );



	Mercury::Address readBackupData( BinaryIStream & stream, bool hasChannel );
	void onRestored( bool hasChannel, const Mercury::Address & clientAddr );

	void proxyRestoreTo();
	void onEnableWitnessAck();

	/// @name Accessors
	//@{
	bool hasClient() const			
	{ 
		return (pClientChannel_ != NULL) || 
			(pBufferedClientBundle_.get() != NULL); 
	}

	Mercury::ChannelPtr pClientChannel() const { return pClientChannel_; }

	bool isClientChannel( Mercury::Channel * pClientChannel ) const	
	{ return (pClientChannel_ != NULL) && (pClientChannel == pClientChannel_); }
	bool isGivingClientAway() const { return isGivingClientAway_; }
	bool isClientConnected() const { return pClientChannel() != NULL && pClientChannel()->isConnected(); }

	bool entitiesEnabled() const	{ return entitiesEnabled_; }

	const Mercury::Address & clientAddr() const;
	SessionKey sessionKey() const				{ return sessionKey_; }

	PyEntityMailBox * pClientEntityMailBox() const
									{ return pClientEntityMailBox_; }

	const char * c_str() const;
	//@}

	// Script related methods
	PY_AUTO_METHOD_DECLARE( RETOK, giveClientTo, ARG( PyObjectPtr, END ) )
	bool giveClientTo( PyObjectPtr pDestProxy );
	bool giveClientLocally( Proxy * pLocalDest );

	void onGiveClientToCompleted( bool success = true );

	bool attachToClient( const Mercury::Address & clientAddr,
		Mercury::ReplyID loginReplyID,
		Mercury::Channel * pChannel );

	void detachFromClient( bool shouldCondemn );
	void logOffClient( bool shouldCondemnChannel );

	PyObject * pyGet_ownClient();
	PY_RO_ATTRIBUTE_SET( ownClient )
	PY_RO_ATTRIBUTE_DECLARE( hasClient(), hasClient )
	PY_RO_ATTRIBUTE_DECLARE( isGivingClientAway(), isGivingClientAway )
	PY_RO_ATTRIBUTE_DECLARE( clientAddr(), clientAddr )

	PY_RO_ATTRIBUTE_DECLARE( entitiesEnabled_, entitiesEnabled )

	PY_RO_ATTRIBUTE_DECLARE( wardsHolder_, wards )

	PY_WO_ATTRIBUTE_GET( bandwidthPerSecond ) // write-only for now
	int pySet_bandwidthPerSecond( PyObject * value );

	PY_READABLE_ATTRIBUTE_GET( inactivityTimeout_, inactivityTimeout )
	int pySet_inactivityTimeout( PyObject * value );

	PY_RO_ATTRIBUTE_DECLARE( roundTripTime(), roundTripTime )
	PY_RO_ATTRIBUTE_DECLARE( timeSinceHeardFromClient(),
			timeSinceHeardFromClient )
	PY_RW_ATTRIBUTE_DECLARE( latencyTriggersHolder_, latencyTriggers )
	PY_RO_ATTRIBUTE_DECLARE( latencyAtLastCheck_, latencyLast )

	PY_AUTO_METHOD_DECLARE( RETOWN, streamStringToClient,
		ARG( PyObjectPtr, OPTARG( PyObjectPtr, NULL, OPTARG( int, -1, END ) ) ) )
	PyObject * streamStringToClient(
		PyObjectPtr pData, PyObjectPtr pDesc, int id );

	PY_AUTO_METHOD_DECLARE( RETOWN, streamFileToClient,
		ARG( BW::string, OPTARG( PyObjectPtr, NULL, OPTARG( int, -1, END ) ) ) )
	PyObject * streamFileToClient( const BW::string & path,
			PyObjectPtr pDesc, int id );

	/* Internal Interface */

	void cellEntityCreated();
	void cellEntityDestroyed( const Mercury::Address * pSrcAddr );
	void acceptClient( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	// Start of messages forwarded from cell ...

	void sendToClient();
	void createCellPlayer( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,BinaryIStream & data );
	void spaceData( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,BinaryIStream & data );
	void enterAoI( const BaseAppIntInterface::enterAoIArgs & args );
	void enterAoIOnVehicle( const BaseAppIntInterface::enterAoIOnVehicleArgs & args );
	void leaveAoI( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data );
	void createEntity( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data );
	void createEntityDetailed( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data );
	void updateEntity( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	// The following bit of unattractive macros is used to declare many of
	// the message handlers for messages sent from the cell that are to be
	// forwarded to the client. This includes the handlers for all of the
	// avatarUpdate messages.
#define MF_EMPTY_COMMON_RELIABLE_MSG( MESSAGE )								\
	void MESSAGE();

#define MF_BEGIN_COMMON_RELIABLE_MSG( MESSAGE )								\
	void MESSAGE( const BaseAppIntInterface::MESSAGE##Args & args );

#define MF_BEGIN_COMMON_PASSENGER_MSG MF_BEGIN_COMMON_RELIABLE_MSG
#define MF_BEGIN_COMMON_UNRELIABLE_MSG MF_BEGIN_COMMON_RELIABLE_MSG

#define MF_COMMON_ARGS( ARGS )
#define MF_END_COMMON_MSG()
#define MF_COMMON_ISTREAM( NAME, XSTREAM )
#define MF_COMMON_OSTREAM( NAME, XSTREAM )

#include "connection/common_client_interface.hpp"

#undef MF_EMPTY_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_UNRELIABLE_MSG
#undef MF_COMMON_ARGS
#undef MF_END_COMMON_MSG
#undef MF_COMMON_ISTREAM
#undef MF_COMMON_OSTREAM

	void modWard( const BaseAppIntInterface::modWardArgs & args );

	// ... end of messages forwarded from cell.

	bool sendBundleToClient( bool expectData = false );

	/* External Interface */

	// receive an update from the client for our position
	void avatarUpdateImplicit(
		const BaseAppExtInterface::avatarUpdateImplicitArgs & args );
	void avatarUpdateExplicit(
		const BaseAppExtInterface::avatarUpdateExplicitArgs & args );
	void avatarUpdateWardImplicit(
		const BaseAppExtInterface::avatarUpdateWardImplicitArgs & args );
	void avatarUpdateWardExplicit(
		const BaseAppExtInterface::avatarUpdateWardExplicitArgs & args );

	void ackPhysicsCorrection();
	void ackWardPhysicsCorrection(
		const BaseAppExtInterface::ackWardPhysicsCorrectionArgs & args );

	void requestEntityUpdate( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void enableEntities();

	void restoreClientAck(
		const BaseAppExtInterface::restoreClientAckArgs & args );

	void disconnectClient(
			const BaseAppExtInterface::disconnectClientArgs & args );

	void cellEntityMethod( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data );
	void baseEntityMethod( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	static WatcherPtr pWatcher();

	double roundTripTime() const;
	double timeSinceHeardFromClient() const;
	float avgClientBundleDataUnits() const
	{ 
		return avgClientBundleDataUnits_.average();
	}

	bool isRestoringClient() const	{ return isRestoringClient_; }

	typedef BW::vector< EntityID > Wards;
	typedef BW::vector< float > LatencyTriggers;

	void callClientMethod( const Mercury::Address & srcAddr,
						   Mercury::UnpackedMessageHeader & header, 
						   BinaryIStream &data );

	void sendMessageToClient( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void sendMessageToClientUnreliable( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	BinaryOStream * getStreamForEntityMessage( Mercury::MessageID msgID,
		int methodStreamSize, bool isReliable = true );

	const BW::string & encryptionKey() const { return encryptionKey_; }
	void encryptionKey( const BW::string & data ) { encryptionKey_ = data; }

	RateLimitMessageFilterPtr pRateLimiter();

	static EventHistoryStats & privateClientStats();

	void cellBackupHasWitness( bool v ) 	{ cellBackupHasWitness_ = v; }

	void regenerateSessionKey();

	void onFilterLimitsExceeded( const Mercury::Address & srcAddr,
			BufferedMessage * pMessage );

	SessionKey prepareForLogin( const Mercury::Address & clientAddress );
	void prepareForReLogOnAfterGiveClientToSuccess(
		const Mercury::Address & clientAddress, Mercury::ReplyID replyID,
		const BW::string & encryptionKey );
	void discardReLogOnAttempt();
	void completeReLogOnAttempt( const Mercury::Address & clientAddress,
		Mercury::ReplyID replyID,
		const BW::string & encryptionKey );

private:
	static DownloadStreamer & downloadStreamer();

	// -------------------------------------------------------------------------
	// Section: Client channel stuff
	// -------------------------------------------------------------------------

	class ClientBundlePrimer : public Mercury::BundlePrimer
	{
	public:
		ClientBundlePrimer( Proxy & proxy ) : proxy_( proxy ) {}
		void primeBundle( Mercury::Bundle & bundle );
		int numUnreliableMessages() const;

	private:
		Proxy & proxy_;
	};

	friend class ClientBundlePrimer;

	bool hasOutstandingEnableWitness() const
		{ return numOutstandingEnableWitness_ != 0; }

	/// The bundle to use for client-entity messages
	Mercury::Bundle & clientBundle();

	void setClientChannel( Mercury::Channel * pChannel );
	int addOpportunisticData( Mercury::Bundle * b );
	void sendEnableDisableWitness( bool enable = true,
			bool isRestore = false );
	void logBadWardWarning( EntityID ward );

	void pRateLimiter( RateLimitMessageFilterPtr pRateLimiter );

	void sendMessageToClientHelper( BinaryIStream & data,
			bool isReliable = true );

	void addCreateBasePlayerToChannelBundle();

	Mercury::ChannelPtr	pClientChannel_;
	ClientBundlePrimer	clientBundlePrimer_;

	BW::string			encryptionKey_;
	SessionKey			sessionKey_;

	PyEntityMailBox * pClientEntityMailBox_;
	friend class ClientEntityMailBox;

	// Does the client want a witness now?
	bool		entitiesEnabled_;
	// Has the client asked for a witness since the last full reset?
	bool		basePlayerCreatedOnClient_;
	// Do we need to call the onEntitiesEnabled script callback?
	// i.e. have we sent a new BasePlayer since the last entitiesEnabled?
	bool		shouldRunCallbackOnEntitiesEnabled_;

	Wards			wards_;
	PySTLSequenceHolder< Wards >			wardsHolder_;
	GameTime		lastModWardTime_;

	LatencyTriggers	latencyTriggers_;
	PySTLSequenceHolder< LatencyTriggers >	latencyTriggersHolder_;

	float	latencyAtLastCheck_;

	bool	isRestoringClient_;

	DataDownloads dataDownloads_;

	// The amount of bandwidth that this proxy would like have to stream
	// downloads to the attached client, in bytes/tick.
	int 				downloadRate_;

	// The actual streaming rate that this proxy was at the last time it had to
	// throttle back due to packet loss.
	int					apparentStreamingLimit_;

	// The average earliest unacked packet age for this client, used as a
	// baseline for calculating when packet loss has occurred.
	float				avgUnackedPacketAge_;

	// The number of packets sent to this client in the previous tick
	int 				prevPacketsSent_;

	// The total number of bytes downloaded to this client
	uint64				totalBytesDownloaded_;

	void modifyDownloadRate( int delta );
	int scaledDownloadRate() const;

	ProxyPusher *		pProxyPusher_;

	GameTime			lastLatencyCheckTime_;

	std::auto_ptr< Mercury::DeferredBundle > pBufferedClientBundle_;

	RateLimitMessageFilterPtr		pRateLimiter_;

	bool			cellHasWitness_;
	bool			cellBackupHasWitness_;
	int				numOutstandingEnableWitness_;

	bool			isGivingClientAway_;

	EMA				avgClientBundleDataUnits_;

	std::auto_ptr< PendingReLogOn > pPendingReLogOn_;

	// The override for baseApp/inactivityTimeout config option
	float inactivityTimeout_;
};

typedef SmartPointer< Proxy > ProxyPtr;

BW_END_NAMESPACE


#endif // PROXY_HPP
