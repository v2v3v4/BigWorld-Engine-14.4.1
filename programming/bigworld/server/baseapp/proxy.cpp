#include "script/first_include.hpp"

#include "pyscript/script.hpp"

#include "cstdmf/memory_stream.hpp"
#include "cstdmf/timestamp.hpp"

#include "baseapp.hpp"
#include "baseapp_config.hpp"
#include "client_entity_mailbox.hpp"
#include "download_streamer.hpp"
#include "proxy.hpp"
#include "rate_limit_message_filter.hpp"
#include "worker_thread.hpp"

#include "cellapp/cellapp_interface.hpp"
#include "cellappmgr/cellappmgr_interface.hpp"

#include "pyscript/pyobject_base.hpp"

#include "server/event_history_stats.hpp"
#include "server/nat_config.hpp"
#include "server/stream_helper.hpp"
#include "server/util.hpp"

#include "network/channel_sender.hpp"
#include "network/deferred_bundle.hpp"
#include "network/encryption_filter.hpp"
#include "network/encryption_stream_filter.hpp"
#include "network/event_dispatcher.hpp"
#include "network/msgtypes.hpp"
#include "network/symmetric_block_cipher.hpp"
#include "network/tcp_channel.hpp"
#include "network/tcp_channel_stream_adaptor.hpp"
#include "network/udp_bundle.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/xml_section.hpp"


#ifdef _WIN32
#pragma warning( disable: 4355 )  // 'this' used in base member initializer list
#endif

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE


/**
 *	This class represents a pending re-log-on attempt.
 */
class PendingReLogOn
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param clientAddress 	The pending re-log-on client address.
	 *	@param replyID			The reply ID.
	 *	@param encryptionKey 	The encryption key for the new connection.
	 */
	PendingReLogOn( const Mercury::Address & clientAddress,
			Mercury::ReplyID replyID,
			const BW::string & encryptionKey ) :
		clientAddress_( clientAddress ),
		replyID_( replyID ),
		encryptionKey_( encryptionKey )
	{}

	/** This method returns the client address. */
	const Mercury::Address & clientAddress() const { return clientAddress_; }
	/** This method returns the reply ID. */
	Mercury::ReplyID replyID() const { return replyID_; }
	/** This method returns the encryption key. */
	const BW::string & encryptionKey() const { return encryptionKey_; }

private:
	Mercury::Address 	clientAddress_;
	Mercury::ReplyID 	replyID_;
	BW::string 			encryptionKey_;
};


namespace  // anon
{

const char * clientDisconnectReasonToString( ClientDisconnectReason reason )
{
	static const char * REASON_STRINGS[] = {
		"Requested by client",
		"Given to other proxy",
		"Rate limit exceeded",
		"Connection timeout",
		"Proxy entity was restored",
		"BaseApp shutdown",
		"Cell entity restoration failure"
	};

	if (reason < 0 || reason >= NUM_CLIENT_DISCONNECT_REASONS)
	{
		return "Invalid reason";
	}

	return REASON_STRINGS[reason];
}


const float MIN_CLIENT_INACTIVITY_RESEND_DELAY = 1.f;

const float CLIENT_BUNDLE_MOVING_AVERAGE_SAMPLES = 100.f;
const float CLIENT_BUNDLE_DATA_UNITS_THRESHOLD = 1.5f;

bool				g_privateClientStatsInited = false;
EventHistoryStats 	g_privateClientStats;

class DownloadCallbacks : public IFinishedDownloads
{
public:
	/**
	 * 	Override of IFinishedDownloads::onFinished
	 */
	void onFinished( uint16 id, bool successful )
	{
		finishedIDs_.push_back( std::make_pair( id, successful ) );
	}

	void triggerCallbacks( PyObject * pObj )
	{
		// Trigger callbacks for any complete/aborted downloads
		for (uint i = 0; i < finishedIDs_.size(); ++i)
		{
			CompleteRec &rec = finishedIDs_[i];

			Script::call(
				PyObject_GetAttrString( pObj,
					"onStreamComplete" ),
				Py_BuildValue( "(ib)", rec.first, rec.second ),
				"DownloadCallbacks::triggerCallbacks: ",
				/*okIfFunctionNull:*/true );
		}
	}
private:
	typedef std::pair< uint16, bool >	CompleteRec;
	BW::vector< CompleteRec >		finishedIDs_;
};

} // anonymous namespace


EventHistoryStats & Proxy::privateClientStats()
{
	return g_privateClientStats;
}


PY_BASETYPEOBJECT( Proxy )

PY_BEGIN_METHODS( Proxy )
	/*~ function Proxy.giveClientTo
	 *	@components{ base }
	 *
	 *	This method transfers the client associated with this proxy to
	 *	another proxy.  The current proxy must have a client and have
	 *	enabled entities, and the destination must not have a client.
	 *  @see Proxy.onGiveClientToSuccess
	 *  @see Proxy.onGiveClientToFailure
	 *
	 *	@param proxy the proxy to transfer the client to.
	 */
	PY_METHOD( giveClientTo )

	/*~ function Proxy.streamStringToClient
	 *	@components{ base }
	 *
	 *	This method adds some data to send to the currently attached
	 *	client. Data is cleared if the client detaches from this entity
	 *	or this BaseApp, and this method may only be called when a
	 *	client is attached. The management of the 16-bit id is entirely
	 *	up to the caller. If the ID is not specified by the caller then
	 *	an unused id is allocated.
	 *
	 *	You can define a callback in your Proxy-derived class
	 *	(onStreamComplete) that will be called when either all data has
	 *	been sent to the client, or the download has failed or been
	 *	aborted.
	 *
	 *	See Proxy.onStreamComplete in the base Python API documentation, and
	 *	BWPersonality.onStreamComplete in client Python API documentation.
	 *
	 *	@param data The string to be sent.
	 *	@param desc="" An optional string description to be sent.
	 *	@param id=-1 A 16-bit id whose management is entirely up to the
	 *	caller. If -1 is passed in then the next id in sequence that is not
	 *	currently in use is selected.
	 *
	 *	@return The id associated with this download.
	 */
	PY_METHOD( streamStringToClient )

	/*~ function Proxy.streamFileToClient
	 *  @components{ base }
	 *
	 *  This method is similar to streamStringToClient() except that the string
	 *  passed as the first argument is the path to a file to be sent to the
	 *  client, rather than the actual data itself.  This should be used
	 *  whenever data is being read off disk to be sent to the client, as all
	 *  file operations are done in a separate thread and therefore the timing
	 *  of the main thread is not jeopardised.
	 *
	 *  The client treats data sent with streamStringToClient() and
	 *  streamFileToClient() in exactly the same way, i.e. it doesn't try to
	 *  write data sent down with this method to a file.
	 *
	 *  See BWPersonality.onStreamComplete in client Python API documentation.
	 *  @see Proxy.onStreamComplete
	 *
	 *  @param resourceName The name of the resource to be sent.
	 *
	 *  @param desc="" An optional string description to be sent.
	 *
	 *  @param id=-1 A 16-bit id whose management is entirely up to the
	 *  caller. If -1 is passed in then the next id in sequence that is not
	 *  currently in use is selected.
	 *
	 *  @return The id associated with this download.
	 */
 	PY_METHOD( streamFileToClient )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Proxy )

	/*~ attribute Proxy.ownClient
	 *	@components{ base }
	 *
	 *	This is the ClientEntityMailBox that communicates directly with the
	 *	Client entity on the ClientApp associated with this Proxy.
	 *
	 *	If the entity does not have an attached client, a dummy mailbox is
	 *	returned. This will consume the method call but will not cause any
	 *	network data to be sent.
	 *
	 *	Use the hasClient attribute to check if a client is attached.
	 */
	PY_ATTRIBUTE( ownClient )

	/*~ attribute Proxy.client
	 *	@components{ base }
	 *
	 *	This is an alias for Proxy.ownClient.
	 */
	PY_ATTRIBUTE_ALIAS( ownClient, client )

	/*~ attribute Proxy.hasClient
	 *	@components{ base }
	 *
	 *	This is whether or not a client is currently attached to this Proxy.
	 */
	PY_ATTRIBUTE( hasClient )

	/*~ attribute Proxy.isGivingClientAway
	 *	@components{ base }
	 *
	 *	This attribute is True when we are in the process of giving the client
	 *	connection to another Proxy, False at all other times.
	 */
	PY_ATTRIBUTE( isGivingClientAway )

	/*~ attribute Proxy.clientAddr
	 *	@components{ base }
	 *
	 *	This tuple contains the ip and port of the Client machine
	 */
	PY_ATTRIBUTE( clientAddr )

	/*~ attribute Proxy.entitiesEnabled
	 *	@components{ base }
	 *
	 *	This is whether or not entities have been enabled by the Client.
	 *	Scripts cannot communicate with the Client until it enables entities.
	 */
	PY_ATTRIBUTE( entitiesEnabled )

	/*~ attribute Proxy.wards
	 *	@components{ base }
	 *
	 *	This is a list of EntityIDs that the associated Client is allowed to control.  When controlEntity()
	 *	is called on the Client, the newly controlled Entity should be added to this list so the updates will
	 *	be propagated to the Cell, rather than the Cell updating the Client.
	 */
	PY_ATTRIBUTE( wards )

	/*~ attribute Proxy.bandwidthPerSecond
	 *	@components{ base }
	 *
	 *	Set this to limit the amount of bandwidth this Client wishes to receive per second
	 */
	PY_ATTRIBUTE( bandwidthPerSecond )

	/*~ attribute Proxy.inactivityTimeout
	 *	@components{ base }
	 *
	 *	This is an override for baseApp/inactivityTimeout configuration option in bw.xml.
	 *	Equals to baseApp/inactivityTimeout by default. Changing this attribute will
	 *	only affect the given instance of the Proxy. Attribute can be set before or after
	 *	a client is connected through the given Proxy.
	 */
	PY_ATTRIBUTE( inactivityTimeout )

	/*~ attribute Proxy.roundTripTime
	 *	@components{ base }
	 *
	 *  This is the average round trip time in seconds for communication to the
	 *	client.
	 */
	PY_ATTRIBUTE( roundTripTime )

	/*~ attribute Proxy.timeSinceHeardFromClient
	 *	@components{ base }
	 *
	 *	This is the number of seconds since a packet from the client was last
	 *	received.
	 */
	PY_ATTRIBUTE( timeSinceHeardFromClient )

	/*~ attribute Proxy.latencyTriggers
	 *	@components{ base }
	 *
	 *  This is a sequence of latency trigger values. Whenever the latency
	 *  passes this value in either direction, the onLatencyTrigger method
	 *  is called. The call is made with the relevant trigger value and
	 *  the direction (1 = above, 0 = below) as arguments. The latency is
	 *	checked roughly every second, but the calls occur in sorted order,
	 *	as if the latency value moved smoothly through its range. The latency
	 *	value used for this calculation is that of 'timeSinceHeardFromClient'.
	 */
	PY_ATTRIBUTE( latencyTriggers )

	/*~ attribute Proxy.latencyLast
	 *	@components{ base }
	 *
	 *	This is the latency value that was used for the last check of the
	 *	latency triggers. It may be useful for initialising the state of
	 *	trigger call handlers when inserting their trigger value into the list.
	 */
	PY_ATTRIBUTE( latencyLast )
PY_END_ATTRIBUTES()

// Callback
/*~ callback Proxy.onLogOnAttempt
 *	@components{ base }
 *	If present, this method is called if a client attempts to log in and wants
 *	to use this entity.
 *
 *	This callback should return one of the following: BigWorld.LOG_ON_ACCEPT,
 *	BigWorld.LOG_ON_REJECT, BigWorld.LOG_ON_WAIT_FOR_DESTROY.
 *
 *	If this callback returns BigWorld.LOG_ON_REJECT, the log on attempt fails.
 *
 *	If this callback returns BigWorld.LOG_ON_ACCEPT, the client is attached to
 *	this active entity.
 *
 *	If this callback returns BigWorld.LOG_ON_WAIT_FOR_DESTROY or the proxy
 *	destroys itself in this callback, the log on attempt will wait until the
 *	current entity has logged off and then log on as normal. Note: The
 *	calling code is still responsible for ensuring the current entity
 *	is destroyed via Base.destroy. If the current entity has not been
 *	destroyed within 5 seconds, the relogon attempt will be automatically
 *	rejected.
 *
 *	@param ip	The IP address of the client attempting to connect.
 *	@param port	The port number of the client attempting to connect.
 *	@param logOnData	A string provided by the billing system.
 */
/*~	callback Proxy.clientDead
 *	@components{ base }
 *	This callback method is obsolete. It is replaced by onClientDeath.
 */
/*~	callback Proxy.onClientDeath
 *	@components{ base }
 *	If present, this method is called when it has been detected that the
 *	associated client has disconnected. This method has no arguments.
 */
/*~	callback Proxy.onEntitiesEnabled
 *	@components{ base }
 *	If present, this method is called when the client requests that updates
 *	from the cell be enabled. This method has no arguments.
 */
/*~	callback Proxy.onClientGetCell
 *	@components{ base }
 *	If present, this method is called when the message to the client that it now
 *	has a cell is called.
 */
/*~	callback Proxy.onLatencyTrigger
 *	@components{ base }
 *	This method is called when the latency associated with the channel to this
 *	proxy's client, crosses a threshold in the latencyTriggers attribute. To
 *	check the current latency use Proxy.roundTripTime or
 *	Proxy.timeSinceHeardFromClient.
 *	@param latency	The latency trigger value.
 *	@param goneAboveThis	Indicates whether the latency has gone above or
 *		below this.
 */
/*~ callback Proxy.onStreamComplete
 *  @components{ base }
 *  If present, this is called when a download to the attached client started
 *  with either Proxy.streamStringToClient() or Proxy.streamFileToClient() has
 *  terminated.
 *  @param id		The id of the terminated download.
 *  @param success	Boolean indicating whether or not the download completed
 *			successfully.
 */
/*~ callback Proxy.onGiveClientToFailure
 *  @components{ base }
 *  If present, this is called if giveClientTo has been called for a proxy that
 *  could not accept this client connection, for example if it already has a
 *  client connected.
 *  @see Proxy.giveClientTo
 *  @see Proxy.onGiveClientToSuccess
 */
/*~ callback Proxy.onGiveClientToSuccess
 *  @components{ base }
 *  If present, this is called after the proxy has completed its part of
 *  giveClientTo and no longer has a client, except in the case of
 *  Proxy.giveClientTo( None ).
 *  @see Proxy.giveClientTo
 *  @see Proxy.onGiveClientToFailure
 */


/**
 *  Change the download streaming rate for this Proxy by the specified amount.
 */
void Proxy::modifyDownloadRate( int delta )
{
	// Don't let it go negative
	if (downloadRate_ + delta < 0)
		delta = -downloadRate_;

	DownloadStreamer & streamer = Proxy::downloadStreamer();

	int maxRate = streamer.maxClientDownloadRate();

	// Don't let it exceed the hard max per client
	if (downloadRate_ + delta > maxRate)
		delta = maxRate - downloadRate_;

	streamer.modifyDownloadRate( delta );
	downloadRate_ += delta;
}


/**
 *  Returns the rate of data streaming to this client scaled by the bandwidth
 *  limits of the BaseApp.  This is the upper limit for the actual download
 *  rate, since we always round down to packet boundaries.
 */
int Proxy::scaledDownloadRate() const
{
	return int( downloadRate_ * Proxy::downloadStreamer().downloadScaleBack() );
}


/**
 * This class calls sendBundleToClient on the proxy periodically.
 */
class ProxyPusher : public TimerHandler
{
public:
	ProxyPusher( Proxy * pProxy ) : pProxy_( pProxy )
	{
		timerHandle_ = BaseApp::instance().timeQueue().add(
			BaseApp::instance().time() + 1, 1, this, NULL, "ProxyPusher" );
	}

	virtual ~ProxyPusher()
	{
		timerHandle_.cancel();
	}

private:
	virtual void handleTimeout( TimerHandle, void * )
	{
		if (!pProxy_->hasClient())
			return;

		pProxy_->sendBundleToClient( /*expectData:*/ true );
	}

	virtual void onRelease( TimerHandle handle, void  * pUser ) {}

	Proxy * pProxy_;
	TimerHandle timerHandle_;
};


// -----------------------------------------------------------------------------
// Section: Proxy
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Proxy::Proxy( EntityID id, DatabaseID dbID, EntityType * pType ) :
	Base( id, dbID, pType ),
	pClientChannel_( NULL ),
	clientBundlePrimer_( *this ),
	sessionKey_( 0 ),
	pClientEntityMailBox_( new ClientEntityMailBox( *this ) ),
	entitiesEnabled_( false ),
	basePlayerCreatedOnClient_( false ),
	shouldRunCallbackOnEntitiesEnabled_( false ),
	wardsHolder_( wards_, this, /*writable:*/false ),
	lastModWardTime_( 0 ),
	latencyTriggersHolder_( latencyTriggers_, this, /*writable:*/true ),
	latencyAtLastCheck_( 0.f ),
	isRestoringClient_( false ),
	downloadRate_( 0 ),
	apparentStreamingLimit_( 0 ),
	avgUnackedPacketAge_( 0.0 ),
	prevPacketsSent_( 0 ),
	totalBytesDownloaded_( 0 ),
	pProxyPusher_( NULL ),
	lastLatencyCheckTime_( 0 ),
	pBufferedClientBundle_( NULL ),
	pRateLimiter_( NULL ),
	cellHasWitness_( false ),
	cellBackupHasWitness_( false ),
	numOutstandingEnableWitness_( 0 ),
	isGivingClientAway_( false ),
	avgClientBundleDataUnits_( EMA::calculateBiasFromNumSamples(
			CLIENT_BUNDLE_MOVING_AVERAGE_SAMPLES ), 
		1.f ),
	pPendingReLogOn_( NULL ),
	inactivityTimeout_( BaseAppConfig::inactivityTimeout() )
{
	// TRACE_MSG( "Proxy::Proxy: id = %u\n", this->id() );

	// Make sure that this class does not have virtual functions. If it does,
	// it screws up the memory layout when we pretend it is a PyInstanceObject.
	MF_ASSERT( (void *)this == (void *)((PyObject *)this) );
	MF_ASSERT( id_ != 0 );

	isProxy_ = true;

	// set up our list of wards. initially just our client
	wards_.push_back( id_ );	// TODO: really?
	this->regenerateSessionKey();
}


/**
 * When a client logs in, we give a different session key to the login key
 * used to first connect. This one makes a new one, presumably it should only
 * be used soon before the key is sent to the client.
 */
void Proxy::regenerateSessionKey()
{
	do
	{
		// TODO: Not sure why this cannot be 0. If anyone finds out why, write
		// a comment!!
		sessionKey_ = uint32( timestamp() );
	}
	while (sessionKey_ == 0);
}


/**
 *	This method attaches this proxy to the client at the input address.
 *	It is only used the first time this client is attached to a proxy on this
 *	BaseApp, local handoffs are entirely handled by @see giveClientTo.
 *
 *	@param clientAddr 		The client address.
 *	@param loginReplyID		The reply ID of the login request message.
 *	@param pChannel			If this is a Mercury/TCP request, the associated
 *							TCP channel, otherwise NULL.
 */
bool Proxy::attachToClient( const Mercury::Address & clientAddr,
		Mercury::ReplyID loginReplyID, 
		Mercury::Channel * pChannel )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	IF_NOT_MF_ASSERT_DEV( pClientChannel_ == NULL )
	{
		return false;
	}

	BaseApp & baseApp = BaseApp::instance();

	bool hasClient = (clientAddr != Mercury::Address::NONE);

	if (hasClient)
	{
		// Create the BlockCipher to encrypt the channel with.
		Mercury::BlockCipherPtr pBlockCipher = NULL;

		if (encryptionKey_.empty())
		{
			ERROR_MSG( "Proxy::attachToClient( %s ): "
					"No session encryption key, falling back to unencrypted "
					"connection\n",
				clientAddr.c_str() );
		}
		else
		{
			pBlockCipher = 
				Mercury::SymmetricBlockCipher::create( encryptionKey_ );

			if (!pBlockCipher)
			{
				ERROR_MSG( "Proxy::attachToClient( %s ): "
						"Invalid encryption key, falling back to unencrypted "
						"connection\n",
					clientAddr.c_str() );
			}
		}

		if (pChannel == NULL)
		{
			// TCP Channels have already been created before we process
			// bundles, UDP channels need to be created explicitly.

			pChannel = new Mercury::UDPChannel(
				baseApp.extInterface(),
				clientAddr,
				Mercury::UDPChannel::EXTERNAL,
				MIN_CLIENT_INACTIVITY_RESEND_DELAY );
		}

		if (pBlockCipher)
		{
			pChannel->setEncryption( pBlockCipher );
		}

		this->setClientChannel( pChannel );

		// Create a new instance of the rate limiting message filter. This is
		// passed around with the client channel, if a proxy has one, it is
		// responsible for deleting it.
		pRateLimiter_ = new RateLimitMessageFilter( pClientChannel_->addr() );
		pRateLimiter_->setProxy( this );

		pClientChannel_->pMessageFilter( pRateLimiter_.get() );

		pClientChannel_->startInactivityDetection( inactivityTimeout_ );

		// now we are ready for the world to know about us.
		baseApp.addProxy( this );

		// create an object to push ourselves internally
		MF_ASSERT( pProxyPusher_ == NULL );
		if (!this->hasCellEntity())
		{
			pProxyPusher_ = new ProxyPusher( this );
		}

		// Send the login reply before anything else (if required).  This must
		// be the first message on this channel and must be on a bundle by
		// itself.  Even though the login message is off-channel (because it has
		// to be - we don't know the client's address until we get it), we want
		// to send back the reply on the channel because the client has a
		// channel for us now and if we send this off-channel the PacketFilters
		// won't work.  Also - this means that all downstream traffic to the
		// client is filtered (i.e. encrypted).
		if (loginReplyID != Mercury::REPLY_ID_NONE)
		{
			// Make a new session key to send with the reply.
			// Don't buffer this messages behind createBasePlayer
			Mercury::Bundle & bundle = pClientChannel_->bundle();
			bundle.startReply( loginReplyID );
			bundle << sessionKey_;
			this->sendBundleToClient();
		}

		// Now that the external interface will have the ClientInterface
		// registered, we can prime bundles with those messages.
		pClientChannel_->bundlePrimer( &clientBundlePrimer_ );

		// Don't buffer these messages behind createBasePlayer
		Mercury::Bundle & b = pClientChannel_->bundle();

		ClientInterface::updateFrequencyNotificationArgs & frequencyArgs =
			ClientInterface::updateFrequencyNotificationArgs::start( b );

		frequencyArgs.hertz = uint8(BaseAppConfig::updateHertz());

		ClientInterface::setGameTimeArgs::start( b ).gameTime = baseApp.time();

		if (basePlayerCreatedOnClient_ && pBufferedClientBundle_.get())
		{
			// TODO: This is because Channel::send will not send arbitrary
			// bundles on a channel with a bundle primer present.
			// Would it work to copy the messages into the clientBundle instead?
			pClientChannel_->bundlePrimer( NULL );

			Mercury::Bundle & bundle = pClientChannel_->bundle();
			pBufferedClientBundle_->applyToBundle( bundle );
			pClientChannel_->send( &bundle );
			pClientChannel_->bundlePrimer( &clientBundlePrimer_ );
			pBufferedClientBundle_.reset();
		}
	}
	else
	{
		INFO_MSG( "Proxy::attachToClient: "
			"Channel not created for %u. No client yet.\n", id_ );
	}

	return true;
}


/**
 *	Destructor.
 */
Proxy::~Proxy()
{
	// when we are alive someone has a reference to us
	MF_ASSERT( !this->hasClient() );

	if (pProxyPusher_ != NULL)
	{
		delete pProxyPusher_;
		pProxyPusher_ = NULL;
	}

	pRateLimiter_ = NULL;

	// TODO: Deleting dataDownloads_ here can block the main thread since it
	// currently waits for file loading jobs to not be running in background
	// thread.
	// See Bugzilla bug 34002
}


/**
 *	This gets called when a client dies. Currently, we use it to not send them
 *	any more packets. It also informs the client's relatives (the Cell), which
 *	grieves for a few microseconds then removes it.
 */
void Proxy::onClientDeath( ClientDisconnectReason reason,
		bool shouldExpectClient /* = true */ )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	TRACE_MSG( "Proxy::onClientDeath: Client id %u has disconnected (%s)\n", 
		this->id(), clientDisconnectReasonToString( reason ) );

	// we can be called again if we are already dead if we try to send more
	// stuff to the client ... for now we just ignore it but could do no more...
	if (!this->hasClient() && shouldExpectClient)
	{
		return;	// already dead and told the cell about it
	}

	if (shouldExpectClient || this->hasClient())
	{
		// If we don't expect a client, we don't care about finalising
		// acks from the channel
		bool shouldCondemnClient = shouldExpectClient;

		if (reason == CLIENT_DISCONNECT_TIMEOUT ||
			reason == CLIENT_DISCONNECT_RATE_LIMITS_EXCEEDED )
		{
			// We don't care about finalising acks from a timed-out channel
			shouldCondemnClient = false;
		}
		this->logOffClient( shouldCondemnClient );
	}

	// OK, we know the cell and we haven't told it about it yet. Call away
	PyObject * pFunc = PyObject_GetAttrString( this, "onClientDeath" );

	if (!pFunc)
	{
		// For backwards-compatibility.
		PyErr_Clear();
		pFunc = PyObject_GetAttrString( this, "clientDead" );

		if (pFunc)
		{
			WARNING_MSG( "Proxy::onClientDeath: clientDead callback is "
					"deprecated. Use onClientDeath instead\n" );
		}
	}

	if (pFunc != NULL)
	{
		PyObject * res = PyObject_CallFunction( pFunc, "()" );
		Py_DECREF( pFunc );

		if (res == NULL)
		{
			DEBUG_MSG( "Proxy::onClientDeath: An exception was thrown.\n" );
			PyErr_Print();
		}
		else
		{
			Py_DECREF( res );
		}
	}
	else
	{
		PyErr_Clear();
		INFO_MSG( "Proxy::onClientDeath: No onClientDeath method\n" );
	}
}


/**
 *	This method is called when this proxy is destroyed.
 */
void Proxy::onDestroy( bool isOffload )
{
	// Take ourselves out of the baseApp's proxy list
	if (this->hasClient())
	{
		// We must have cleaned up our client during the
		// processing of the offload
		MF_ASSERT( !isOffload );

		this->logOffClient( /* shouldCondemnChannel */ true );
	}

	Py_XDECREF( pClientEntityMailBox_ );
	pClientEntityMailBox_ = NULL;

	MF_ASSERT( pClientChannel_ == NULL );

	this->Base::onDestroy( isOffload );
}




/**
 *	This method sends a message to the client to restore it to a previous state.
 *	This can occur when a cell application dies unexpectedly.
 */
void Proxy::restoreClient()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	if (!this->hasClient())
	{
		return;
	}

	const size_t overheadSize = sizeof( StreamHelper::AddEntityData ) +
 			sizeof( StreamHelper::CellEntityBackupFooter );

	if (cellBackupData_.length() < overheadSize)
	{
		WARNING_MSG( "Disconnected client at %s with no cell backup\n",
			this->c_str() );

		this->onClientDeath( CLIENT_DISCONNECT_CELL_RESTORE_FAILED );
		BaseApp::instance().removeProxy( this );
		return;
	}

	// TRACE_MSG( "Proxy::restoreClient: id = %d\n", id_ );
	const char * pData = cellBackupData_.data();
	int footerOffset = cellBackupData_.length() -
		sizeof( StreamHelper::CellEntityBackupFooter );
	const StreamHelper::CellEntityBackupFooter & backupFooter =
		*(const StreamHelper::CellEntityBackupFooter *)(pData + footerOffset);

	const StreamHelper::AddEntityData & addEntityData =
					*(const StreamHelper::AddEntityData *)pData;
	pData += sizeof( StreamHelper::AddEntityData );

	isRestoringClient_ = true;

	if (cellBackupData_.length() < overheadSize + backupFooter.cellClientSize())
	{
		WARNING_MSG( "Proxy::restoreClient: "
				"Disconnected client at %s with insufficient backup data\n",
			this->c_str() );

		this->onClientDeath( CLIENT_DISCONNECT_CELL_RESTORE_FAILED );
		BaseApp::instance().removeProxy( this );
		return;
	}

	Mercury::Bundle & bundle = this->clientBundle();
	bundle.startMessage( ClientInterface::restoreClient );

	bundle << id_ << this->spaceID() << backupFooter.vehicleID() <<
		addEntityData.position << addEntityData.direction;

	this->addAttributesToStream( bundle,
		EntityDescription::FROM_BASE_TO_CLIENT_DATA );

	// We should never have a templated entity data stream in our backup
	MF_ASSERT( !addEntityData.hasTemplate() );
	bundle.addBlob( pData, backupFooter.cellClientSize() );

	this->sendBundleToClient();
}


/**
 *	This method writes backup information for this proxy to the input stream.
 */
void Proxy::writeBackupData( BinaryOStream & stream )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	stream << this->clientAddr();
	stream << sessionKey_ << encryptionKey_;

	stream << wards_.size();

	for (Wards::size_type i = 0; i < wards_.size(); ++i)
	{
		stream << wards_[i];
	}

	stream << uint32(latencyTriggers_.size());
	for (LatencyTriggers::size_type i = 0; i < latencyTriggers_.size(); ++i)
		stream << latencyTriggers_[i];
	stream << latencyAtLastCheck_;

	// TODO: Consider whether we want the client to send us this information.
	stream << entitiesEnabled_ << basePlayerCreatedOnClient_;
	stream << shouldRunCallbackOnEntitiesEnabled_ << cellHasWitness_;
	stream << dataDownloads_;

	// TODO: Need to consider:
	//	haveChangedAddress_: Can probably assume true.
	//
/*
	Mercury::Channel *		pClientChannel_;		Address is sent
	bool					haveChangedAddress_;	??
	ClientEntityMailBox *	pClientEntityMailBox_;	NO: Restore from clientAddr_
	bool					entitiesEnabled_;		YES
	bool					basePlayerCreatedOnClient_;		YES
	bool					shouldRunCallbackOnEntitiesEnabled_;		YES
	Wards					wards_;					YES
	PySTLSequenceHolder< Wards >	wardsHolder_;	NO
	bool					isRestoringClient_;		?? Updating
	DataDownloads				dataDownloads_;			YES
	ProxyPusher	*			pProxyPusher_;			??
	uint64					lastSentAnything_;		NO
	bool			cellHasWitness_;				YES
*/

}


/**
 *  This function is called when this proxy is offloaded in order to transfer
 *  the connected client.
 */
void Proxy::offload( const Mercury::Address & dstAddr )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (this->hasClient())
	{
		// We don't wait for an acknowledgement of this
		// transfer, since we're going to be destroyed now.
		this->transferClient( dstAddr, /* shouldReset */ false );
		this->detachFromClient( /* shouldCondemn */ true );
	}
}


/**
 *  This function will transfer the connected client and optionally reset 
 *  entities on the client if it has been transferred between two 
 *  different proxies.
 */
void Proxy::transferClient( const Mercury::Address & dstAddr, 
		bool shouldReset, Mercury::ReplyMessageHandler * pHandler )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	Mercury::Bundle & bundle = this->clientBundle();

	Mercury::Address externalAddr = 
		BaseApp::instance().getExternalAddressFor( dstAddr );

	MF_ASSERT( pClientChannel_ != NULL );

	if (NATConfig::isExternalIP( pClientChannel_->addr().ip ))
	{
		externalAddr.ip = NATConfig::externalIPFor( externalAddr.ip );
	}

	// Either a cross-CellApp handoff, or an offload.
	MF_ASSERT( isGivingClientAway_ || pHandler == NULL );

	// Abort any in-progress downloads.
	DownloadCallbacks callbacks;
	dataDownloads_.abortDownloads( callbacks );

	DEBUG_MSG( "Proxy::transferClient( %s %d ): "
			"switching client %s to BaseApp %s (%s)\n",
		this->pType()->name(), 
		id_,
		pClientChannel_->c_str(),
		externalAddr.c_str(),
		shouldReset ? "should reset" : "no reset" );

	ClientInterface::switchBaseAppArgs & rArgs = (pHandler ? 
		(ClientInterface::switchBaseAppArgs::startRequest( bundle, pHandler )) :
		(ClientInterface::switchBaseAppArgs::start( bundle )));

	rArgs.baseAddr = externalAddr;
	rArgs.shouldResetEntities = shouldReset;

	this->sendBundleToClient();

	// If giving the client away, we need to know that we've aborted the
	// downloads.
	// Otherwise, we're offloading, and the new BaseApp's copy of us
	// will note the downloads active in the backup and abort them.
	// If we're offloading, our backup has already been sent, so it's
	// too late to change state. So we _never_ call triggerCallbacks()
	if (isGivingClientAway_)
	{
		callbacks.triggerCallbacks( this );
	}
}


/**
 *	This method restores this proxy from the input backup information.
 */
Mercury::Address Proxy::readBackupData( BinaryIStream & stream,
		bool hasChannel )
{
	Mercury::Address clientAddr;
	stream >> clientAddr;
 	stream >> sessionKey_ >> encryptionKey_;

	Wards::size_type wardsSize;
	stream >> wardsSize;

	wards_.resize( wardsSize );

	for (Wards::size_type i = 0; i < wardsSize; ++i)
	{
		stream >> wards_[i];
	}

	uint32 ltSize;
	stream >> ltSize;
	latencyTriggers_.resize( ltSize );
	for (LatencyTriggers::size_type i = 0; i < ltSize; ++i)
		stream >> latencyTriggers_[i];
	stream >> latencyAtLastCheck_;

	stream >> entitiesEnabled_ >> basePlayerCreatedOnClient_;
	stream >> shouldRunCallbackOnEntitiesEnabled_ >> cellHasWitness_;

	stream >> dataDownloads_;

	if (clientAddr != Mercury::Address::NONE)
	{
		if (hasChannel)
		{
			this->prepareForLogin( clientAddr );
		}
		else
		{
			// If we are not perfectly synced to cell state, restore to
			// a disconnected state since the cell will do that too.
			entitiesEnabled_ = false;
			basePlayerCreatedOnClient_ = false;
			shouldRunCallbackOnEntitiesEnabled_ = false;
			cellHasWitness_ = false;
		}
	}

	return clientAddr;
}


/**
 * This method is called after the Base entity has finished restoring from
 * the backup stream.
 */
void Proxy::onRestored( bool hasChannel, const Mercury::Address & clientAddr )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (!hasChannel && (clientAddr != Mercury::Address::NONE))
	{
		this->onClientDeath( CLIENT_DISCONNECT_BASE_RESTORE,
			/*shouldExpectClient: */ false );
	}
}


/**
 *	This method does proxy-specific operations in addition to those in
 *	Base::restoreTo() when we restore a cell entity from backup.
 */
void Proxy::proxyRestoreTo()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (cellHasWitness_ != cellBackupHasWitness_)
	{
		// We need to explicitly send a enable/disable witness message to
		// our cell entity - the backup data that we just restored with is
		// inconsistent with enable/disable events after the last backup.
		this->sendEnableDisableWitness( /* enable */ cellHasWitness_,
			/* isRestore */true );
	}
}


/**
 *  This method returns this proxy's client address, or Address::NONE if no
 *  client is attached.
 */
const Mercury::Address & Proxy::clientAddr() const
{
	return pClientChannel_ ? pClientChannel_->addr() : Mercury::Address::NONE;
}


/**
 *  This method returns this proxy's C-string representation.
 */
const char * Proxy::c_str() const
{
	return pClientChannel_ ?
		pClientChannel_->c_str() : this->clientAddr().c_str();
}


/******* Internal stuff *******/


/**
 *	This method deals with our cell entity being created.
 */
void Proxy::cellEntityCreated()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (!entitiesEnabled_) return;
	MF_ASSERT( this->hasClient() );

	MF_ASSERT( this->hasCellEntity() );

	//  create the witness
	this->sendEnableDisableWitness( /*enable:*/true );

	// get rid of the proxy pusher now that the witness will be sending us
	// regular updates (the self motivator should definitely be there).
	MF_ASSERT( pProxyPusher_ != NULL );
	delete pProxyPusher_;
	pProxyPusher_ = NULL;
}


/**
 *	This method deals with our cell entity being destroyed.
 */
void Proxy::cellEntityDestroyed( const Mercury::Address * pSrcAddr )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (pSrcAddr != NULL)
	{
		bool shouldSend = false;
		Mercury::Channel & channel = BaseApp::getChannel( *pSrcAddr );
		Mercury::Bundle & bundle = channel.bundle();
		Wards::iterator iter = wards_.begin();

		while (iter != wards_.end())
		{
			EntityID wardID = *iter;

			if (wardID != id_)
			{
				CellAppInterface::delControlledByArgs & rDelControlledBy =
					CellAppInterface::delControlledByArgs::start( bundle,
						wardID );

				rDelControlledBy.deadController = id_;

				shouldSend = true;
			}

			++iter;
		}

		if (shouldSend)
		{
			channel.send();
		}
	}

	if (entitiesEnabled_)
	{
		MF_ASSERT( this->hasClient() );

		MF_ASSERT( !this->hasCellEntity() );

		// tell the client that it should reset all entities except the base
		// player
		Mercury::Bundle & b = this->clientBundle();
		ClientInterface::resetEntitiesArgs::start( b ).
			keepPlayerOnBase = true;
		this->sendBundleToClient();

		// and we'll need to wait for another enableEntities before we tell
		// it of any newly created cell entities (as an ack)
		entitiesEnabled_ = false;
		cellHasWitness_ = false;
		MF_ASSERT( basePlayerCreatedOnClient_ ); // must be if entities enabled
		// must not be the case if entities were enabled
		MF_ASSERT( !shouldRunCallbackOnEntitiesEnabled_ );

		// reinstate a proxy pusher
		MF_ASSERT( pProxyPusher_ == NULL );
		pProxyPusher_ = new ProxyPusher( this );
	}

	MF_ASSERT( !cellHasWitness_ );
}


/**
 *	This reply handler is used to keep track of how many outstanding
 *	enableWitness there are for each proxy.
 */
class EnableWitnessReplyHandler :
	public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	EnableWitnessReplyHandler( ProxyPtr pProxy ) :
		pProxy_( pProxy )
	{
	}

	void handleMessage( const Mercury::Address& /*srcAddr*/,
			Mercury::UnpackedMessageHeader& /*header*/,
			BinaryIStream& data, void * /*arg*/ )
	{
		this->onReply();
	}

	void handleException( const Mercury::NubException&, void * )
	{
		WARNING_MSG( "EnableWitnessReplyHandler::handleException: id = %u\n",
				pProxy_->id() );
		this->onReply();
	}

	void onReply()
	{
		pProxy_->onEnableWitnessAck();
		delete this;
	}

private:
	ProxyPtr pProxy_;
};


/**
 *	This method is called when there is confirmation that the witness has been
 *	created.
 */
void Proxy::onEnableWitnessAck()
{
	MF_ASSERT( numOutstandingEnableWitness_ > 0 );
	--numOutstandingEnableWitness_;
}


/**
 *	This method sends an enable or disable witness message to our cell entity.
 *
 *	@param enable		whether to enable or disable the witness
 *	@param isRestore 	is this an explicit witness enable/disable send as a
 *						result of a restore cell entity?
 */
void Proxy::sendEnableDisableWitness( bool enable, bool isRestore )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	Mercury::Bundle & bundle = this->cellBundle();
	bundle.startRequest( CellAppInterface::enableWitness,
			new EnableWitnessReplyHandler( this ) );

	bundle << id_;
	bundle << isRestore;

	++numOutstandingEnableWitness_;
	cellHasWitness_ = enable;

	if (enable)
	{
		bundle << BaseAppConfig::bytesPerPacketToClient();
	}
	// else just send an empty stream

	this->sendToCell();	// send it straight away
}



/**
 *  This method handles a message from a ClientViaBaseMailBox. It calls
 *  the target method on the client entity.
 */
void Proxy::callClientMethod( const Mercury::Address & srcAddr,
						   Mercury::UnpackedMessageHeader & header, 
						   BinaryIStream & data )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	uint16 methodIndex;
	uint8 recordingOptionValue;
	data >> methodIndex >> recordingOptionValue;
	RecordingOption recordingOption = RecordingOption( recordingOptionValue );

	IF_NOT_MF_ASSERT_DEV( !data.error() )
	{
		ERROR_MSG( "Proxy::callClientMethod(%u): "
				"Truncated packet received, discarding!\n",
			id_ );
		return;
	}

	const MethodDescription * pDescription =
		this->pType()->description().client().internalMethod( methodIndex );
	if (pDescription == NULL)
	{
		ERROR_MSG( "Proxy::callClientMethod(%u): "
				"Invalid method index (%d) on client.\n", 
			id_, methodIndex );
		data.finish();
		return;
	}

	if (this->isDestroyed())
	{
		ERROR_MSG( "Proxy::callClientMethod(%u): "
				"Calling client method %s on a destroyed proxy.\n", 
			id_, pDescription->name().c_str() );
		data.finish();
		return;
	}

	if (!this->hasClient())
	{
		if (numOutstandingEnableWitness_ == 0)
		{
			// We haven't changed our witness state recently, so we shouldn't
			// be getting these when we don't have a client.
			ERROR_MSG( "Proxy::callClientMethod(%u): "
					"requested to call a client method %s "
					"when there is no client channel\n",
				id_, pDescription->name().c_str() );
		}
		else
		{
			// We've recently enabled/disabled our witness, but haven't
			// received acknowledgement from our cell yet. This message may
			// have come while we are in this transition, and so is not an
			// error.
			NOTICE_MSG( "Proxy::callClientMethod(%u): "
					"dropping client method call %s "
					"for client channel recently gone\n",
				id_, pDescription->name().c_str() );
		}
		data.finish();
		return;
	}

	if (pDescription->shouldRecord( recordingOption ))
	{
		if (!this->isClientConnected())
		{
			ERROR_MSG( "RemoteClientMethod::pyCall: %s(%d).%s: "
					"No cell entity channel to send method recording data\n",
				pType_->name(),
				this->id(),
				pDescription->name().c_str() );
			data.finish();
			return;
		}
		else
		{
			Mercury::Bundle & bundle = pChannel_->bundle();
			bundle.startMessage( CellAppInterface::recordClientMethod );

			bundle << this->id() << pDescription->internalIndex();

			MemoryIStream recordingStream( data.retrieve( 0 ),
				data.remainingLength() );

			bundle.transfer( recordingStream,
				recordingStream.remainingLength() );

			if (recordingOption == RECORDING_OPTION_RECORD_ONLY)
			{
				data.finish();
				return;
			}
		}
	}

	BinaryOStream * pBOS = pClientEntityMailBox_->getStream(
			*pDescription );

	if (pBOS == NULL)
	{
		ERROR_MSG( "Proxy::callClientMethod(%u): "
				"Failed to get stream on client method %s.\n",
			id_, pDescription->name().c_str() );
		data.finish();
		return;
	}

	g_privateClientStats.trackEvent( this->pType()->description().name(),
			pDescription->name(), data.remainingLength(),
			pDescription->streamSize( true ) );
	pBOS->transfer( data, data.remainingLength() );
	pClientEntityMailBox_->sendStream();
}


/**
 *	This method handles a script message that should be forwarded to the client.
 */
void Proxy::sendMessageToClient( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	this->sendMessageToClientHelper( data, /*isReliable:*/ true );
}


/**
 *	This method handles a script message that should be forwarded to the client.
 */
void Proxy::sendMessageToClientUnreliable( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	this->sendMessageToClientHelper( data, /*isReliable:*/ false );
}


/**
 *	This method forwards this message to the client.
 */
void Proxy::sendMessageToClientHelper( BinaryIStream & data, bool isReliable )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (this->hasOutstandingEnableWitness())
	{
		// Do nothing. It's for an old client.
		data.finish();
		return;
	}

	Mercury::MessageID msgID;
	data >> msgID;

	if (!this->hasClient())
	{
		WARNING_MSG( "Proxy::sendMessageToClientHelper(%u): "
				"No client. Cannot forward msgID %d\n",
			id_, msgID );

		data.finish();
		return;
	}

	int16 msgStreamSize;
	data >> msgStreamSize;
	MF_ASSERT( msgStreamSize < 0 || msgStreamSize == data.remainingLength() );

	BinaryOStream * pOutput = this->getStreamForEntityMessage(
		msgID, msgStreamSize, isReliable );

	MF_ASSERT( pOutput != NULL );

	pOutput->transfer( data, data.remainingLength() );
}


/**
 *  This method gets a suitable stream to the client for an entity property
 *	update or method call.
 *
 *	@param messageID	The messageID of the client-side event
 *	@param messageStreamSize 	The fixed size of the message or -1 if variable.
 *
 *	@return A BinaryOStream* to which the message can be written
 *		or NULL if no client is attached to this proxy.
 */
BinaryOStream * Proxy::getStreamForEntityMessage( Mercury::MessageID msgID,
	int methodStreamSize, bool isReliable /* = true */ )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (!this->hasClient())
	{
		return NULL;
	}

	int8 lengthStyle = Mercury::FIXED_LENGTH_MESSAGE;
	int lengthParam = methodStreamSize;

	if (methodStreamSize < 0)
	{
		lengthStyle = Mercury::VARIABLE_LENGTH_MESSAGE;
		lengthParam = -methodStreamSize;
	}

	Mercury::InterfaceElement ie( "entityMessage", msgID,
			lengthStyle, lengthParam );

	Mercury::Bundle & bundle = this->clientBundle();

	bundle.startMessage( ie,
		(isReliable ? Mercury::RELIABLE_DRIVER : Mercury::RELIABLE_NO) );

	return &bundle;
}


/**
 *  This message tells a proxy to accept a connection handed to it from
 *	another proxy. 
 */
void Proxy::acceptClient( const Mercury::Address & srcAddr,
						  const Mercury::UnpackedMessageHeader & header, 
						  BinaryIStream & data )
{
	Mercury::Address clientAddr;
	SessionKey sessionKey;
	BW::string encryptionKey;
	data >> sessionKey >> encryptionKey >> clientAddr;

	Mercury::Channel & channel = BaseApp::getChannel( srcAddr );
	Mercury::Bundle & bundle = channel.bundle();
	bundle.startReply( header.replyID );

	if (!this->hasClient())
	{
		// Logins from switching BaseApps will come from a different port.
		clientAddr.port = 0;
		sessionKey_ = sessionKey;
		encryptionKey_ = encryptionKey;

		this->prepareForLogin( clientAddr );

		bundle << true;
	}
	else
	{
		if (this->clientAddr() != Mercury::Address::NONE)
		{
			ERROR_MSG( "Proxy::acceptClient: "
					"proxy %d already attached to client at %s\n",
				id_,
				this->c_str() );
		}
		else
		{
			ERROR_MSG( "Proxy::acceptClient: "
					"proxy %d already pending for accepting a client\n",
				id_ );
		}

		bundle << false;
	}

	channel.send();
}


/**
 *	This method prepares the proxy for an impending login.
 *
 *	@param clientAddress 	The expected login client address.
 *
 *	@return 	The login key generated from adding the pending login.
 */
SessionKey Proxy::prepareForLogin( const Mercury::Address & clientAddress )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	SessionKey loginKey = BaseApp::instance().addPendingLogin( this,
		clientAddress );

	pBufferedClientBundle_.reset( new Mercury::DeferredBundle() );
	clientBundlePrimer_.primeBundle( *pBufferedClientBundle_ );

	return loginKey;
}


/**
 *	This method adds the createBasePlayer message to the given bundle
 *
 *	It should immediately follow a successful login or full
 *	entity reset, so the client is never operating without
 *	a Base Player entity.
 *	Note: When this method is called,
 *		Proxy::sendExposedForReplayClientPropertiesToCell() should be called
 *		together at the same time.
 *
 *	@param bundle	The Mercury::Bundle to add the message to
 */
void Proxy::addCreateBasePlayerToChannelBundle()
{
	DEBUG_MSG( "Proxy::addCreateBasePlayerToChannelBundle(%u): "
			"Creating player on client\n",
		id_ );

	MF_ASSERT( pClientChannel_ != NULL );
	MF_ASSERT( shouldRunCallbackOnEntitiesEnabled_ == false );
	MF_ASSERT( basePlayerCreatedOnClient_ == false );

	Mercury::Bundle & bundle = pClientChannel_->bundle();

	bundle.startMessage( ClientInterface::createBasePlayer );
	bundle << id_ << pType_->description().clientIndex();
	this->addAttributesToStream( bundle,
		EntityDescription::FROM_BASE_TO_CLIENT_DATA );

	shouldRunCallbackOnEntitiesEnabled_ = true;
	basePlayerCreatedOnClient_ = true;
}


/**
 *	This message is the cell telling us that it has now sent us all the
 *	updates for the given tick, and we should forward them on to the client.
 */
void Proxy::sendToClient()
{
	// Do nothing. It's for an old client.
	if (!this->hasOutstandingEnableWitness())
	{
		this->sendBundleToClient();
	}
}


/**
 *	This method forwards this message to the client (reliably)
 */
#define STRUCT_CLIENT_MESSAGE_FORWARDER( MESSAGE )							\
void Proxy::MESSAGE( const BaseAppIntInterface::MESSAGE##Args & args )		\
{																			\
	if (this->hasOutstandingEnableWitness())								\
	{																		\
		/* Do nothing. It's for an old client. */							\
	}																		\
	else if (this->hasClient())												\
	{																		\
		Mercury::Bundle & b =											\
			this->clientBundle();											\
		b.startMessage( ClientInterface::MESSAGE );							\
		((BinaryOStream&)b) << args;										\
	}																		\
	else																	\
	{																		\
		WARNING_MSG( "Proxy::" #MESSAGE										\
			": Cannot forward for %u. No client attached\n", id_ );			\
	}																		\
}																			\

/**
 *	This method forwards this message to the client (reliably)
 */
#define VARLEN_CLIENT_MESSAGE_FORWARDER( MESSAGE )							\
	VARLEN_CLIENT_MESSAGE_FORWARDER_PLUS( MESSAGE, doNothing )

#define VARLEN_CLIENT_MESSAGE_FORWARDER_PLUS( MESSAGE, EXTRA_FN )			\
void Proxy::MESSAGE( const Mercury::Address & srcAddr,						\
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data )	\
{																			\
	if (this->hasOutstandingEnableWitness())								\
	{																		\
		/* Do nothing. It's for an old client. */							\
		data.finish();														\
	}																		\
	else if (this->hasClient())												\
	{																		\
		int length = data.remainingLength();								\
		Mercury::Bundle & b =											\
			this->clientBundle();											\
		b.startMessage( ClientInterface::MESSAGE );							\
		b.transfer( data, length );											\
		EXTRA_FN( this );													\
	}																		\
	else																	\
	{																		\
		WARNING_MSG( "Proxy::" #MESSAGE										\
			": Cannot forward for %u. No client attached\n", id_ );			\
	}																		\
}																			\

inline void doNothing( Proxy * ) {}

inline void onCreateCellPlayer( Proxy * pProxy )
{
	Script::call( PyObject_GetAttrString( pProxy, "onClientGetCell" ),
			PyTuple_New( 0 ), "onClientGetCell", true );
}

VARLEN_CLIENT_MESSAGE_FORWARDER_PLUS( createCellPlayer, onCreateCellPlayer )		// forward to client
VARLEN_CLIENT_MESSAGE_FORWARDER( spaceData )			// forward to client
STRUCT_CLIENT_MESSAGE_FORWARDER( enterAoI )				// forward to client
STRUCT_CLIENT_MESSAGE_FORWARDER( enterAoIOnVehicle )	// forward to client
VARLEN_CLIENT_MESSAGE_FORWARDER( leaveAoI )				// forward to client
VARLEN_CLIENT_MESSAGE_FORWARDER( createEntity )			// forward to client
VARLEN_CLIENT_MESSAGE_FORWARDER( createEntityDetailed )	// forward to client
VARLEN_CLIENT_MESSAGE_FORWARDER( updateEntity )			// forward to client


// The following macros and include are used to implement the message handlers
// for many messages that are sent from the cell and are to be forwarded to the
// client unreliably. This includes handlers for messages such as all of the
// avatarUpdate messages.

#define MF_EMPTY_COMMON_MSG( MESSAGE, RELIABILITY )							\
void Proxy::MESSAGE()														\
{																			\
	if (this->hasOutstandingEnableWitness())								\
	{																		\
		/* Do nothing. It's for an old client. */							\
	}																		\
	else if (this->hasClient())												\
	{																		\
		Mercury::ReliableType isReliable(									\
				BaseApp::instance().shouldMakeNextMessageReliable() ?		\
					Mercury::RELIABLE_DRIVER :								\
					RELIABILITY );											\
		Mercury::Bundle & bundle =											\
				this->clientBundle();										\
		bundle.startMessage( ClientInterface::MESSAGE, isReliable );		\
	}																		\
	else																	\
	{																		\
		WARNING_MSG( "Proxy::" #MESSAGE										\
			": Cannot forward for %u. No client attached\n", id_ );			\
	}																		\
}																			\

#define MF_BEGIN_COMMON_MSG( MESSAGE, RELIABILITY )							\
void Proxy::MESSAGE( const BaseAppIntInterface::MESSAGE##Args & args )		\
{																			\
	if (this->hasOutstandingEnableWitness())								\
	{																		\
		/* Do nothing. It's for an old client. */							\
	}																		\
	else if (this->hasClient())												\
	{																		\
		Mercury::ReliableType isReliable(									\
				BaseApp::instance().shouldMakeNextMessageReliable() ?		\
					Mercury::RELIABLE_DRIVER :								\
					RELIABILITY );											\
		Mercury::Bundle & bundle =											\
				this->clientBundle();										\
		ClientInterface::MESSAGE##Args & newArgs =							\
			ClientInterface::MESSAGE##Args::start( bundle, isReliable );	\
		newArgs = *(ClientInterface::MESSAGE##Args*)(&args);				\
	}																		\
	else																	\
	{																		\
		WARNING_MSG( "Proxy::" #MESSAGE										\
			": Cannot forward for %u. No client attached\n", id_ );			\
	}																		\
}																			\

#define MF_EMPTY_COMMON_RELIABLE_MSG( MESSAGE )								\
	MF_EMPTY_COMMON_MSG( MESSAGE, Mercury::RELIABLE_DRIVER )

#define MF_BEGIN_COMMON_RELIABLE_MSG( MESSAGE )								\
	MF_BEGIN_COMMON_MSG( MESSAGE, Mercury::RELIABLE_DRIVER )

#define MF_BEGIN_COMMON_PASSENGER_MSG( MESSAGE )							\
	MF_BEGIN_COMMON_MSG( MESSAGE, Mercury::RELIABLE_PASSENGER )

#define MF_BEGIN_COMMON_UNRELIABLE_MSG( MESSAGE )							\
	MF_BEGIN_COMMON_MSG( MESSAGE, Mercury::RELIABLE_NO )

#define MF_COMMON_ARGS( ARGS )
#define MF_END_COMMON_MSG()
#define MF_COMMON_ISTREAM( NAME, XSTREAM )
#define MF_COMMON_OSTREAM( NAME, XSTREAM )
#include "connection/common_client_interface.hpp"

#undef MF_EMPTY_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_RELIABLE_MSG
#undef MF_BEGIN_COMMON_PASSENGER_MSG
#undef MF_BEGIN_COMMON_UNRELIABLE_MSG
#undef MF_COMMON_ARGS
#undef MF_END_COMMON_MSG
#undef MF_COMMON_ISTREAM
#undef MF_COMMON_OSTREAM


/**
 *	This method tells us about a change in the status of one of our wards.
 *	We modify our internal list, then forward the message on to the client.
 */
void Proxy::modWard( const BaseAppIntInterface::modWardArgs & args )
{
	if (this->hasOutstandingEnableWitness())
	{
		// TODO: Should make sure that the wards are reset when given a new
		// client.
		ERROR_MSG( "Proxy::modWard( %d ): Has outstanding enableWitness\n",
						id_ );
		return;
	}

	// remove it if it is already there
	for (uint i = 0; i < wards_.size(); i++)
	{
		if (wards_[i] == args.id)
		{
			wards_.erase( wards_.begin() + i );
			--i;
		}
	}

	// add it to the back if so desired
	if (args.on)
	{
		wards_.push_back( args.id );
	}

	// forward it to the client too
	if (this->hasClient())
	{
		Mercury::Bundle & b = this->clientBundle();
		ClientInterface::controlEntityArgs & rControlEntityArgs =
			ClientInterface::controlEntityArgs::start( b );

		rControlEntityArgs.id = args.id;
		rControlEntityArgs.on = args.on;

		// When removing wards, give client some allowance for sending us
		// bad entity updates as it may not have received the above message
		// just yet.
		if (!args.on)
			lastModWardTime_ = BaseApp::instance().time();
	}
}


/**
 *	This method returns the average round trip time in seconds to the client.
 */
double Proxy::roundTripTime() const
{
	if (pClientChannel_ == NULL) return -1.f;	// better to set python error?

	return pClientChannel_->roundTripTimeInSeconds();
}


/**
 *	This method returns the number of seconds since a packet from the client was
 *	last received.
 */
double Proxy::timeSinceHeardFromClient() const
{
	if (pClientChannel_ == NULL) return -1.f;	// better to set python error?

	return double( timestamp() - pClientChannel_->lastReceivedTime() ) /
				stampsPerSecondD();
}


/**
 *	This method sends any messages queued by the internal interface (for the
 *	external interface). Returns true if the send occurred (or was attempted
 *	and failed) and the bundle was flushed.
 *
 *	@param irregular True if this send is not a regular, periodic send.
 */
bool Proxy::sendBundleToClient( bool expectData )
{
	// make sure we have a client attached to us
	if (!this->hasClient())
	{
		ERROR_MSG( "Proxy::sendBundleToClient( %u ): "
				"No client. hasOutstandingEnableWitness = %d\n",
			id_, this->hasOutstandingEnableWitness() );

		return false;
	}

	// We are buffering, so just leave everything alone.
	if (pClientChannel_ == NULL)
	{
		return true;
	}

	if (expectData && !pClientChannel_->hasUnsentData())
	{
		WARNING_MSG( "Proxy::sendBundleToClient: There is no data to send\n" );
	}

	Mercury::Bundle & bundle = pClientChannel_->bundle();

	avgClientBundleDataUnits_.sample( bundle.numDataUnits() );

	if (avgClientBundleDataUnits_.average() >=
			CLIENT_BUNDLE_DATA_UNITS_THRESHOLD)
	{
		// Complain only if enough bundles sent recently have been
		// multi-data-unit.
		WARNING_MSG( "Proxy::sendBundleToClient: "
				"Client %u has consistently sent multiple data units per "
				"update (moving average = %.01f)\n",
			id_,
			avgClientBundleDataUnits_.average() );
	}

	// see if we can squeeze any more data on the bundle
	this->addOpportunisticData( &bundle );

	// now actually send the packet!
	pClientChannel_->send();

	// see if a second has passed for low resolution routines
	GameTime gameTime = BaseApp::instance().time();
	int updateHertz = BaseAppConfig::updateHertz();

	if (lastLatencyCheckTime_ + updateHertz <= gameTime)
	{
		// check our latency triggers
		if (!latencyTriggers_.empty())
		{
			float latencyNow = this->timeSinceHeardFromClient();

			LatencyTriggers tocall;	// no cost for common empty vector
			for (uint i = 0; i < latencyTriggers_.size(); i++)
			{
				if ((latencyNow >= latencyTriggers_[i]) !=
					(latencyAtLastCheck_ >= latencyTriggers_[i]))
				{
					tocall.push_back( latencyTriggers_[i] );
				}
			}

			if (!tocall.empty())
			{
				bool isLatencyWorse = (latencyNow >= latencyAtLastCheck_);

				if (isLatencyWorse)
				{
					std::sort( tocall.begin(), tocall.end() );
				}
				else
				{
					std::sort( tocall.begin(), tocall.end(),
							std::greater<float>() );
				}

				LatencyTriggers::iterator iter = latencyTriggers_.begin();

				while (iter != latencyTriggers_.end())
				{
					Script::call(
						PyObject_GetAttrString( this, "onLatencyTrigger" ),
						Py_BuildValue( "(fi)", *iter, isLatencyWorse ),
						"Proxy::latencyTriggerCheck" );

					++iter;
				}
			}

			latencyAtLastCheck_ = latencyNow;
		}

		// set the game time when we last sent anything (non-intermediate...)
		lastLatencyCheckTime_ = gameTime;
	}

	return true;
}


/**
 *  This method writes the 'authenticate' message to the start of the bundle if
 *  required.  It also sends the tickSync to the client if we don't have a cell
 *  entity (which normally handles sending tickSync messages).
 */
void Proxy::ClientBundlePrimer::primeBundle( Mercury::Bundle & bundle )
{
	if (BaseAppConfig::sendAuthToClient())
	{
		ClientInterface::authenticateArgs::start( bundle,
			Mercury::RELIABLE_PASSENGER ).key = proxy_.sessionKey();
	}

	if (proxy_.pProxyPusher_ != NULL)
	{
		ClientInterface::tickSyncArgs::start( bundle,
			Mercury::RELIABLE_DRIVER ).tickByte =
				uint8( BaseApp::instance().time() );
	}
}


/**
 *  This method returns the number of unreliable messages that this primer
 *  writes to the Bundle.
 */
int Proxy::ClientBundlePrimer::numUnreliableMessages() const
{
	// We send authenticate messages if sendAuthToClient() is true.
	return int( BaseAppConfig::sendAuthToClient() );
}


/**
 *  This method sets the client channel to be used by this proxy.
 */
void Proxy::setClientChannel( Mercury::Channel * pNewClientChannel )
{
	MF_ASSERT( pClientChannel_ == NULL || pClientChannel_->refCount() > 0 );

	if (pClientChannel_)
	{
		pClientChannel_->pMessageFilter( NULL );
		pClientChannel_->bundlePrimer( NULL );
		pClientChannel_->userData( NULL );
	}

	pClientChannel_ = pNewClientChannel;

	if (pClientChannel_)
	{
		pClientChannel_->userData( this );
	}

	// Infer regularity of base -> cell stream from whether or not we have a
	// client connected.
	bool isRegular = (pNewClientChannel != NULL );
	pChannel_->isLocalRegular( isRegular );
	pChannel_->isRemoteRegular( isRegular );
}


/**
 *	This method adds any opportunistic data to the given bundle, to fill up the
 *	corners before it is sent.  Right now this means any data which has been
 *	queued for download using streamStringToClient() or streamFileToClient().
 *
 *	@return	The number of extra bytes added onto the bundle.
 */
int Proxy::addOpportunisticData( Mercury::Bundle * pBundle )
{
	// We must not be called without a client channel
	MF_ASSERT( pClientChannel_ != NULL );

	uint32 leftOvers = pBundle->freeBytesInLastDataUnit();
	int backlog = pClientChannel_->isTCP() ? 
			0 : 
			static_cast< Mercury::UDPChannel & >( *pClientChannel_ ).sendWindowUsage();

	// Just throttle back to 0 if we don't have pending downloads
	if (dataDownloads_.empty())
	{
		if (downloadRate_)
		{
			this->modifyDownloadRate( -downloadRate_ );
		}

		prevPacketsSent_ = 1;
		return 0;
	}

	const DownloadStreamer & streamer = Proxy::downloadStreamer();

	// How much actual payload we expect to get on every fresh packet
	const int PACKET_CAPACITY = Mercury::Packet::maxCapacity();

	// This is the the real value the backlog is checked against, and takes
	// into account the client's average backlog.
	int backlogThreshold =
		streamer.downloadBacklogLimit() +
		(int)round( avgUnackedPacketAge_ ) * prevPacketsSent_;

	// If we're sending too much data for either our link or the client, drop
	// the stream rate by the size of a UDP packet and don't send anything.
	// TODO: This will probably happen as soon as the client drops a packet if
	// the RTT is a lot more than the downloadBacklogLimit().  Just throttle on
	// the basis of dropped packet counts, but make sure you only consider
	// regions sent since the last throttle.
	if (backlog > backlogThreshold)
	{
		// Remember this limit so we know to approach it slowly next time
		apparentStreamingLimit_ = this->scaledDownloadRate();
		this->modifyDownloadRate( -PACKET_CAPACITY );

		DEBUG_MSG( "Proxy::addOpportunisticData: "
			"Throttled downloads for %u back to %d bytes/tick "
			"due to ack backlog of %d packets\n",
			id_, downloadRate_, backlog );
		return 0;
	}

	// Otherwise, if this client is allowed to increase its download rate, ramp
	// up the download.  Note this may not increase its actual download rate
	// since we may have exhausted the total streaming bandwidth for this app.
	else if (downloadRate_ < streamer.maxClientDownloadRate())
	{
		// If we've exceeded the previously established download limit, ramp up
		// *really* slowly
		if (apparentStreamingLimit_ &&
			this->scaledDownloadRate() > apparentStreamingLimit_)
		{
			this->modifyDownloadRate( streamer.downloadRampUpRate() / 32 );
		}

		// If we're within striking distance of the apparent limit of this
		// client's connection, ramp up kinda slowly
		else if (apparentStreamingLimit_ &&
			(this->scaledDownloadRate() + PACKET_CAPACITY > 
				 apparentStreamingLimit_))
		{
			this->modifyDownloadRate( streamer.downloadRampUpRate() / 8 );
		}

		// Otherwise, ramp up at the full rate
		else
		{
			this->modifyDownloadRate( streamer.downloadRampUpRate() );
		}
	}

	// Finalise the amount of data we're actually going to send to the client.
	int maxExtraData = this->scaledDownloadRate();

	// If we're using more than the first packet, then we always round down to
	// the next packet boundary
	if (maxExtraData > (int)leftOvers)
	{
		int extraPackets = (maxExtraData - leftOvers) / PACKET_CAPACITY;
		maxExtraData = leftOvers + PACKET_CAPACITY * extraPackets;
	}

	int remainingLength = maxExtraData;

	DownloadCallbacks callbacks;
	totalBytesDownloaded_ +=
		dataDownloads_.addToBundle( *pBundle, remainingLength, callbacks );

	prevPacketsSent_ = pBundle->numDataUnits();

	// If we didn't use all of the allocated bandwidth budget, scale back to
	// reflect what we actually used
	if (remainingLength > 0)
	{
		this->modifyDownloadRate( -remainingLength );
	}

	// Update the average backlog statistic if we only sent a single packet. We
	// only want to calculate this on single packet sends since we don't want
	// the data skewed by big multi-packet download streams.
	if (!pBundle->hasMultipleDataUnits())
	{
		avgUnackedPacketAge_ = (avgUnackedPacketAge_ * 0.9) + (backlog * 0.1);
	}

	callbacks.triggerCallbacks( this );

	return maxExtraData - remainingLength;
}


/******* External stuff *******/

/**
 *	This method expands and forwards this avatar position update to the cell.
 */
void Proxy::avatarUpdateImplicit(
	const BaseAppExtInterface::avatarUpdateImplicitArgs & args )
{
	//TRACE_MSG( "Proxy::avatarUpdate[Ext]: id_=%d\n", id_ );

	// Don't forward it if we don't have entities enabled on this proxy.
	if (!entitiesEnabled_)
	{
		return;
	}

	// and send it off then
	CellAppInterface::avatarUpdateImplicitArgs & rAvatarUpdateImplictArgs =
		CellAppInterface::avatarUpdateImplicitArgs::start( this->cellBundle(),
			id_ );

	rAvatarUpdateImplictArgs.pos = args.pos;
	rAvatarUpdateImplictArgs.dir = args.dir;
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	rAvatarUpdateImplictArgs.refNum = args.refNum;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
}


/**
 *	This method expands and forwards this avatar position update to the cell.
 */
void Proxy::avatarUpdateExplicit(
	const BaseAppExtInterface::avatarUpdateExplicitArgs & args )
{
	//TRACE_MSG( "Proxy::avatarUpdate[Ext]: id_=%d\n", id_ );

	// Don't forward it if we don't have entities enabled on this proxy.
	if (!entitiesEnabled_)
	{
		return;
	}

	// and send it off then
	CellAppInterface::avatarUpdateExplicitArgs & rAvatarUpdateExplictArgs =
		CellAppInterface::avatarUpdateExplicitArgs::start( this->cellBundle(),
			id_ );

	rAvatarUpdateExplictArgs.vehicleID = args.vehicleID;
	rAvatarUpdateExplictArgs.pos = args.pos;
	rAvatarUpdateExplictArgs.dir = args.dir;
	rAvatarUpdateExplictArgs.flags = args.flags;
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	rAvatarUpdateExplictArgs.refNum = args.refNum;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
}


/**
 *	This method expands and forwards this avatar position update to the client.
 */
void Proxy::avatarUpdateWardImplicit(
	const BaseAppExtInterface::avatarUpdateWardImplicitArgs & args )
{
	//TRACE_MSG( "Proxy::avatarUpdate[Ext]: id_=%d\n", id_ );

	// make sure that the ward is in our list of wards (usu v. small)
	EntityID ward = args.ward;
	uint i;
	for (i = 0; i < wards_.size(); i++)
		if (ward == wards_[i]) break;
	if (i >= wards_.size())
	{
		this->logBadWardWarning( ward );
		return;
	}

	// and send it off then
	CellAppInterface::avatarUpdateImplicitArgs & rAvatarUpdateImplictArgs =
		CellAppInterface::avatarUpdateImplicitArgs::start( this->cellBundle(),
			ward );

	rAvatarUpdateImplictArgs.pos = args.pos;
	rAvatarUpdateImplictArgs.dir = args.dir;
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	rAvatarUpdateImplictArgs.refNum = 0;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
}

/**
 *	This method expands and forwards this avatar position update to the client.
 */
void Proxy::avatarUpdateWardExplicit(
	const BaseAppExtInterface::avatarUpdateWardExplicitArgs & args )
{
	//TRACE_MSG( "Proxy::avatarUpdate[Ext]: id_=%d\n", id_ );

	// make sure that the ward is in our list of wards (usu v. small)
	EntityID ward = args.ward;
	uint i;
	for (i = 0; i < wards_.size(); i++)
		if (ward == wards_[i]) break;
	if (i >= wards_.size())
	{
		this->logBadWardWarning( ward );
		return;
	}

	// and send it off then
	CellAppInterface::avatarUpdateExplicitArgs & rAvatarUpdateExplictArgs =
		CellAppInterface::avatarUpdateExplicitArgs::start( this->cellBundle(),
			ward );

	rAvatarUpdateExplictArgs.vehicleID = args.vehicleID;
	rAvatarUpdateExplictArgs.pos = args.pos;
	rAvatarUpdateExplictArgs.dir = args.dir;
	rAvatarUpdateExplictArgs.flags = args.flags;
#if !VOLATILE_POSITIONS_ARE_ABSOLUTE
	rAvatarUpdateExplictArgs.refNum = 0;
#endif /* !VOLATILE_POSITIONS_ARE_ABSOLUTE */
}


/**
 *	This method handles a message from the client acknowledging that it has
 *	received a physics correction.
 */
void Proxy::ackPhysicsCorrection()
{
	if (!entitiesEnabled_)
	{
		// This ack is likely left over from when this connection was connected
		// to another proxy. 
		return;
	}

	// TODO: Better handling of prefixed empty messages
	this->cellBundle().startMessage( CellAppInterface::ackPhysicsCorrection );
	this->cellBundle() << id_;
}


/**
 *	This method handles a message from the client acknowledging that it has
 *	received a physics correction.
 */
void Proxy::ackWardPhysicsCorrection(
		const BaseAppExtInterface::ackWardPhysicsCorrectionArgs & args )
{
	// TODO: Put this in a function.
	// make sure that the ward is in our list of wards (usu v. small)
	EntityID ward = args.ward;
	uint i;
	for (i = 0; i < wards_.size(); i++)
		if (ward == wards_[i]) break;
	if (i >= wards_.size())
	{
		this->logBadWardWarning( ward );
		return;
	}

	// TODO: Better handling of prefixed empty messages
	this->cellBundle().startMessage( CellAppInterface::ackPhysicsCorrection );
	this->cellBundle() << ward;
}


/**
 *	This method logs a warning about a client trying to control a non-ward.
 */
void Proxy::logBadWardWarning( EntityID ward )
{
	// Have some tolerance for clients whose wards have just been deleted
	// recently. They may not have received the message to remove control
	// of the entity just yet.
	static const uint BAD_WARD_TOLERANCE_SECONDS = 5;
	if ((BaseApp::instance().time() - lastModWardTime_) >
			(BAD_WARD_TOLERANCE_SECONDS * BaseAppConfig::updateHertz()))
	{
		WARNING_MSG( "Proxy::avatarUpdate(%u): "
			"Attempted to control non-ward %u\n", id_, ward );
	}
}


/**
 *	This method handles a request from the client for information about a given
 *	entity. It forwards this message on to the cell.
 */
void Proxy::requestEntityUpdate( const Mercury::Address & srcAddr,
								 Mercury::UnpackedMessageHeader & header, 
								 BinaryIStream & data )
{
	Mercury::Bundle & b = this->cellBundle();
	b.startMessage( CellAppInterface::requestEntityUpdate );
	b << id_;
	b.transfer( data, data.remainingLength() );
}


/**
 *	This method handles a request from the client to enable or disable updates
 *	from the cell. It forwards this message on to the cell.
 */
void Proxy::enableEntities()
{
	DEBUG_MSG( "Proxy::enableEntities(%u)\n", id_ );

	// if this is the first we've heard of it, then send the client the props
	// it shares with us, call the base script...
	if (!basePlayerCreatedOnClient_)
	{
		this->addCreateBasePlayerToChannelBundle();
		this->sendExposedForReplayClientPropertiesToCell();

		if (pBufferedClientBundle_.get())
		{
			// Make sure the BasePlayer arrives before the buffered messages
			this->sendBundleToClient();

			// TODO: This is because Channel::send will not send arbitrary
			// bundles on a channel with a bundle primer present.
			// Would it work to copy the messages into the clientBundle instead?
			pClientChannel_->bundlePrimer( NULL );

			Mercury::Bundle & bundle = pClientChannel_->bundle();
			pBufferedClientBundle_->applyToBundle( bundle );
			pClientChannel_->send( &bundle );
			pClientChannel_->bundlePrimer( &clientBundlePrimer_ );
			pBufferedClientBundle_.reset();
		}
	}

	// ... and tell the cell the game is on
	if (!entitiesEnabled_)
	{
		entitiesEnabled_ = true;

		if (this->hasCellEntity())
		{
			this->sendEnableDisableWitness( /*enable:*/true );

			// remove ProxyPusher
			if (pProxyPusher_ != NULL)
			{
				delete pProxyPusher_;
				pProxyPusher_ = NULL;
			}
		}
		else
		{
			// Add a proxy pusher because we don't have a cell to do it.
			if (pProxyPusher_ == NULL)
			{
				pProxyPusher_ = new ProxyPusher( this );
			}
		}
	}

	if (shouldRunCallbackOnEntitiesEnabled_)
	{
		// call the script and let it have its naughty way with the client
		Script::call( PyObject_GetAttrString( this, "onEntitiesEnabled" ),
			PyTuple_New( 0 ), "", true );

		shouldRunCallbackOnEntitiesEnabled_ = false;
	}
}


/**
 *	This method handles an acknowledgement by the client of a call to restore
 *	its state to a previous state.
 */
void Proxy::restoreClientAck(
	const BaseAppExtInterface::restoreClientAckArgs & args )
{
	isRestoringClient_= false;
	// TODO: Should do something with the id.
}


/**
 *	This method handles a message from the client telling us that we should
 *	disconnect it.
 */
void Proxy::disconnectClient(
		const BaseAppExtInterface::disconnectClientArgs & args )
{
	this->onClientDeath( CLIENT_DISCONNECT_CLIENT_REQUESTED );
}


/**
 *	This method forwards a message received on the external interface to the
 *	cell.
 */
void Proxy::cellEntityMethod( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
	const Mercury::MessageID msgID = header.identifier;

	if (shouldRunCallbackOnEntitiesEnabled_)
	{
		NOTICE_MSG( "Proxy::cellEntityMethod: Dropping script message %d. "
				"It was for previous proxy of client of entity %u\n",
			msgID, this->id() );

		data.finish();
		return;
	}

	EntityID	entityID;
	data >> entityID;

	IF_NOT_MF_ASSERT_DEV( !data.error() )
	{
		ERROR_MSG("Proxy::forwardMessageFromClient: "
				  "Truncated packet received, discarding!\n");
		return;
	}

	Mercury::Bundle & b = this->cellBundle();

	b.startMessage( CellAppInterface::runExposedMethod );

	if (entityID == 0)
	{
		entityID = id_;
	}

	b << entityID;
	b << msgID;
	b << id_;			// the method had better be exposed!

	b.transfer( data, data.remainingLength() );
}


/**
 *	This method handles a message received on the external interface.
 */
void Proxy::baseEntityMethod( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
	const Mercury::MessageID msgID = header.identifier;

	if (shouldRunCallbackOnEntitiesEnabled_)
	{
		NOTICE_MSG( "Proxy::baseEntityMethod: Dropping script message %d. "
				"It was for previous proxy of client of entity %u\n",
			msgID, this->id() );

		data.finish();
		return;
	}

	const ExposedMethodMessageRange & range =
						BaseAppExtInterface::Range::baseEntityMethodRange;
	const EntityDescription & entityDesc = this->pType()->description();

	const MethodDescription * pMethodDesc =
		entityDesc.base().exposedMethodFromMsgID( msgID, data, range );

	if (!pMethodDesc)
	{
		ERROR_MSG( "Proxy::handleMessageFromClient: "
				"Proxy %u does not have exposed method %d\n",
			id_, msgID );
		return;
	}
	
	if (pMethodDesc->isComponentised())
	{
		MF_ASSERT(pEntityDelegate_);
		if (!pEntityDelegate_->handleMethodCall(*pMethodDesc, data))
		{
			ERROR_MSG( "Proxy::handleMessageFromClient: "
					"failed to call method %s on %s entity's delegate",
					pMethodDesc->name().c_str(),
					this->pType()->name());
				
		}
	}
	else // conventional call to entity method
	{
		if (!pMethodDesc->callMethod( ScriptObject( this, 
			ScriptObject::FROM_BORROWED_REFERENCE ), data ))
		{
			ERROR_MSG( "Proxy::handleMessageFromClient: "
					"CHEAT: Failed for proxy %u\n", id_ );
		}
	}
}


// ---------------------------------------------------------------------------
// Section: Script interface
// ---------------------------------------------------------------------------


/**
 *	This method implements the 'get' method for the client attribute.
 */
PyObject * Proxy::pyGet_ownClient()
{
	return Script::getData( pClientEntityMailBox_ );
}


/**
 *	The client wants to receive this much bandwidth in total per second.
 */
int Proxy::pySet_bandwidthPerSecond( PyObject * value )
{
	uint bps = uint(-1);
	if (Script::setData( value, bps, "Proxy.bandwidthPerSecond" ) != 0)
		return -1;

	// TODO: Should use the real limit of say 80% of max packet size
	// times update hertz (since this is per second)
	uint arbitraryMaxBPS = 512*1024/10; // about 512 kbps
	if (bps > arbitraryMaxBPS)
	{
		WARNING_MSG( "Proxy.bandwidthPerSecond( %d ): "
						"A bandwidth of %d bytes per second looks too big. "
						"Arbitrary max is %d.\n",
					id_, bps, arbitraryMaxBPS );
	}

	CellAppInterface::witnessCapacityArgs wca = { id_, bps };

	this->cellBundle() << std::make_pair( id_, wca );

	return 0;
}


/**
 *	This method changes the client channel inactivity timeout
 */
int Proxy::pySet_inactivityTimeout( PyObject * value )
{
	float newInactivityTimeout = 0.f;
	int retVal =
		Script::setData( value, newInactivityTimeout,
				"Proxy.inactivityTimeout" );

	if (retVal == 0)
	{
		inactivityTimeout_ = newInactivityTimeout;
		if (pClientChannel_ != NULL)
		{
			pClientChannel_->startInactivityDetection( inactivityTimeout_ );
		}
	}

	return retVal;
}


/**
 *  This class holds the context of a transfer of a client between two bases.
 */
class AcceptClientRequestHandler :
	public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	AcceptClientRequestHandler( Proxy * pSrcProxy ) :
		pSrcProxy_( pSrcProxy )
	{}

	virtual ~AcceptClientRequestHandler() {}

	/**
	 *	This method is called to handle the reply from a remote BaseApp for the 
	 *	acceptClient request, and from the client for the switchBaseApps request.
	 */
	virtual void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg )
	{
		if (pSrcProxy_->isDestroyed())
		{
			WARNING_MSG( "AcceptClientRequestHandler::handleMessage: "
				"Proxy %d was destroyed during giveClientTo(). "
				"Aborting Proxy offload.\n", pSrcProxy_->id() );
			data.finish();
			delete this;
			return;
		}

		if (pSrcProxy_->clientAddr() == Mercury::Address::NONE)
		{
				WARNING_MSG( "AcceptClientRequestHandler::handleMessage: "
					"Proxy %d lost its client during giveClientTo(). "
					"Aborting Proxy offload.\n", pSrcProxy_->id() );
		}
		else if (source != pSrcProxy_->clientAddr())
		{
			// This is the reply from the remote BaseApp.

			bool success;
			data >> success;

			if (success)
			{
				pSrcProxy_->transferClient( source, /* shouldReset */ true,
					this );

				// Our handleMessage() will be called back again when the
				// client acks our transfer, so don't delete ourselves yet.
				return;
			}

			pSrcProxy_->onGiveClientToCompleted( /* success */ false );
		}
		else
		{
			// This is the reply from the client.
			pSrcProxy_->onGiveClientToCompleted();
		}

		delete this;
	}

	virtual void handleException( const Mercury::NubException & exception,
								  void * arg )
	{
		if (pSrcProxy_->isDestroyed())
		{
			WARNING_MSG( "AcceptClientRequestHandler::handleException: "
					"Proxy %d was destroyed during giveClientTo().",
				pSrcProxy_->id() );
		}
		else
		{
			pSrcProxy_->onGiveClientToCompleted( /* success */ false );
		}
		delete this;
	}

private:
	SmartPointer< Proxy > pSrcProxy_;
};


/**
 *	This method is called from script. It allows this proxy to gives its
 *	connection to a client to another proxy.
 *
 *	@return True if successful, otherwise false. If the call failed, the Python
 *		error state is set.
 */
bool Proxy::giveClientTo( PyObjectPtr pDestProxy )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (pClientChannel_ == NULL)
	{
		PyErr_SetString( PyExc_ValueError, 
			(pBufferedClientBundle_.get() != NULL) ? 
				"Proxy is expecting a login" :
				"No client to give" );
		return false;
	}

	if (isGivingClientAway_)
	{
		PyErr_SetString( PyExc_ValueError, "Client is already being given" );
		return false;
	}

	// We are still waiting for the client given to us in a previous
	// giveClientTo from a different BaseApp to enableEntities
	if ((pDestProxy.get() != NULL) && !entitiesEnabled_)
	{
		PyErr_SetString( PyExc_ValueError, "Proxy.entitiesEnabled is false" );
		return false;
	}

	if (ServerEntityMailBox::Check( pDestProxy.get() ))
	{
		// If this is a local entity, resolve it now.
		ServerEntityMailBox * pRemoteDest = 
			static_cast< ServerEntityMailBox * >( pDestProxy.get() );

		if (pRemoteDest->address() == 
				BaseApp::instance().intInterface().address())
		{
			Base * pDestinationBase = BaseApp::instance().bases().findEntity( 
				pRemoteDest->id() );

			if (!pDestinationBase)
			{
				PyErr_SetString( PyExc_ValueError, 
					"Destination pointed to by mailbox does not exist "
					"on this BaseApp" );
				return false;
			}

			pDestProxy = pDestinationBase;
		}
	}

	if (ServerEntityMailBox::Check( pDestProxy.get() ))
	{
		ServerEntityMailBox * pRemoteDest = 
			static_cast< ServerEntityMailBox * >( pDestProxy.get() );

		if (pRemoteDest->component() != EntityMailBoxRef::BASE)
		{
			PyErr_SetString( PyExc_TypeError,
							 "Destination is not a base mailbox to a proxy" );
			return false;
		}

		if (!pRemoteDest->localType().isProxy())
		{
			PyErr_SetString( PyExc_TypeError,
							 "Destination is not a mailbox to a proxy" );
			return false;
		}

		const Mercury::Address externalDestAddr = 
			BaseApp::instance().getExternalAddressFor( 
				pRemoteDest->address() );

		if (externalDestAddr.ip == 0)
		{
			PyErr_Format( PyExc_ValueError,
				"Destination BaseApp address does not exist "
					"(no external address registered for %s)", 
				pRemoteDest->address().c_str() );
			return false;
		}

		DEBUG_MSG( "Proxy::giveClientTo: proxy %d notifying remote BaseApp %s "
				"to accept client %s for proxy %d\n",
			id_,  
			pRemoteDest->address().c_str(),
			this->c_str(),
			pRemoteDest->id() );

		isGivingClientAway_ = true;

		pRemoteDest->bundle().startRequest( BaseAppIntInterface::acceptClient,
			new AcceptClientRequestHandler( this ) );
		pRemoteDest->bundle() << pRemoteDest->id() << sessionKey_ << 
			encryptionKey_ << this->clientAddr();

		if (!pRemoteDest->pChannel())
		{
			PyErr_SetString( PyExc_TypeError,
					"Destination is an invalid base mailbox" );
			return false;
		}

		pRemoteDest->pChannel()->send();

		return true;
	}
	else if ( pDestProxy.get() == NULL )
	{
		this->logOffClient( /* shouldCondemnChannel */ true );
		return true;
	}
	else if (Proxy::Check( pDestProxy.get() ))
	{
		return this->giveClientLocally( static_cast< Proxy * >( 
			pDestProxy.get() ) );
	}

	PyErr_SetString( PyExc_TypeError,
					 "Destination is not a proxy or a mailbox to a proxy" );
	return false;
}


/**
 *  If a client is being transferred between two local proxies, use this special
 *  case to avoid having to reconnect.
 */
bool Proxy::giveClientLocally( Proxy * pLocalDest )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (pLocalDest->hasClient())
	{
		this->onGiveClientToCompleted( /* success */ false );
		return true;
	}

	MF_ASSERT( pClientChannel_ != NULL );

	// Abort the downloads now, although we don't tell the script about
	// it until we've finished handing off the client
	DownloadCallbacks callbacks;
	dataDownloads_.abortDownloads( callbacks );

	if (basePlayerCreatedOnClient_)
	{
		Mercury::Bundle & b = pClientChannel_->bundle();
		ClientInterface::resetEntitiesArgs::start( b ).
			keepPlayerOnBase = false;
		this->sendBundleToClient();
	}

	BaseApp::instance().removeProxy( this );
	// transfer channel stuff to the new proxy
	Mercury::ChannelPtr pSavedClientChannel = pClientChannel_;
	this->setClientChannel( NULL );
	pLocalDest->setClientChannel( pSavedClientChannel.get() );

	// pass ownership of the rate limiter
	pLocalDest->pRateLimiter( pRateLimiter_ );
	pRateLimiter_ = NULL;

	pLocalDest->pClientChannel_->
		bundlePrimer( &(pLocalDest->clientBundlePrimer_) );

	pLocalDest->sessionKey_ = sessionKey_;
	pLocalDest->encryptionKey_ = encryptionKey_;

	// always create a new proxy pusher on the dest - even if it already
	// has a cell entity, it'll need the pusher at least until it receives
	// enableEntities from the client (at which pt it'll make the witness).
	MF_ASSERT( pLocalDest->pProxyPusher_ == NULL );
	pLocalDest->pProxyPusher_ = new ProxyPusher( pLocalDest );

	MF_ASSERT( !pLocalDest->entitiesEnabled_ );
	MF_ASSERT( !pLocalDest->basePlayerCreatedOnClient_ );

	// add the new proxy as a fully-fledged client-bearing proxy
	BaseApp::instance().addProxy( pLocalDest );

	// we sent a full reset, so send the new BasePlayer immediately
	pLocalDest->addCreateBasePlayerToChannelBundle();
	pLocalDest->sendExposedForReplayClientPropertiesToCell();

	MF_ASSERT( pBufferedClientBundle_.get() == NULL );

	// We shouldn't have a client connection to condemn or destroy
	MF_ASSERT( pClientChannel_ == NULL );
	this->detachFromClient( /* shouldCondemn */ false );

	callbacks.triggerCallbacks( this );

	Script::call( PyObject_GetAttrString( this, 
		  "onGiveClientToSuccess" ),
		  PyTuple_New( 0 ), 
		  "Proxy.giveClientTo", /* okIfFnNull */ true );

	return true;
}


/**
 *	This method buffers away the details of a re-logon to be completed 
 *	after an attempt to give the existing client away completes.
 */
void Proxy::prepareForReLogOnAfterGiveClientToSuccess(
		const Mercury::Address & clientAddress,
		Mercury::ReplyID replyID,
		const BW::string & encryptionKey )
{
	if (pPendingReLogOn_.get() != NULL)
	{
		// We respect the most recently accepted re-log-on attempt.
		NOTICE_MSG( 
				"Proxy::prepareForReLogOnAfterGiveClientToSuccess( %s %d ): "
				"Accepted new re-log-on attempt received for %s, "
				"removing existing attempt for %s\n",
			this->pType()->name(),
			id_,
			clientAddress.c_str(),
			pPendingReLogOn_->clientAddress().c_str() );

		this->discardReLogOnAttempt();
	}
	else
	{
		NOTICE_MSG(
				"Proxy::prepareForReLogOnAfterGiveClientToSuccess( %s %d ): "
				"Accepted re-log-on attempt %s, waiting for pending client "
				"transfer to complete\n",
			this->pType()->name(),
			id_,
			clientAddress.c_str() );
	}

	pPendingReLogOn_.reset( new PendingReLogOn( clientAddress, replyID,
		encryptionKey ) );
}


/**
 *	This method discards any buffered re-log-on attempt data, replying back to
 *	DBApp that the re-log-on attempt should be rejected.
 */
void Proxy::discardReLogOnAttempt()
{
	if (pPendingReLogOn_.get() == NULL)
	{
		return;
	}
	
	BaseApp & baseApp = BaseApp::instance();
	Mercury::ChannelSender sender( baseApp.dbApp().channel() );
	Mercury::Bundle & bundle = sender.bundle();
	bundle.startReply( pPendingReLogOn_->replyID() );

	bundle << BaseAppIntInterface::LOG_ON_ATTEMPT_REJECTED;

	pPendingReLogOn_.reset( NULL );
}


/**
 *	This method completes a re-log-on attempt.
 *
 *	@param clientAddress 	The client address that is now expected to log on.
 *	@param replyID 			The reply ID for the request for onLogOnAttempt.
 *	@param encryptionKey	The encryption key to be used.
 */
void Proxy::completeReLogOnAttempt( const Mercury::Address & clientAddress,
		Mercury::ReplyID replyID,
		const BW::string & encryptionKey )
{
	INFO_MSG( "Proxy::completeReLogOnAttempt( %s %d ): "
			"Waiting to accept re-log-on attempt from %s\n",
		this->pType()->name(),
		id_,
		clientAddress.c_str() );

	BaseApp & baseApp = BaseApp::instance();

	SessionKey loginKey = 0;
	if (clientAddress.ip)
	{
		loginKey = this->prepareForLogin( clientAddress );
	}

	this->encryptionKey( encryptionKey );

	Mercury::ChannelSender sender( baseApp.dbApp().channel() );
	Mercury::Bundle & bundle = sender.bundle();
	bundle.startReply( replyID );

	bundle << BaseAppIntInterface::LOG_ON_ATTEMPT_TOOK_CONTROL;

	// This needs to match what the BaseAppMgr sends back to the
	// database.
	bundle << baseApp.extInterface().address();
	bundle << this->baseEntityMailBoxRef();
	bundle << loginKey;

	pPendingReLogOn_.reset( NULL );
}


/**
 *	This method is called when our request to give the client away to another
 *	proxy fails.
 */
void Proxy::onGiveClientToCompleted( bool success )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	isGivingClientAway_ = false;

	if (!success)
	{
		NOTICE_MSG( "Proxy::onGiveClientToCompleted: Failed for proxy %d\n",
			id_ );


		if (pPendingReLogOn_.get() != NULL)
		{
			// We received a re-log-on attempt to take control. It would be 
			// nice to have script specify that re-log-on attempts should
			// only occur if a client was successfully given away (or not).
			// But we currently do it for both cases if script told us to 
			// take control.
			this->logOffClient( /* shouldCondemnChannel */ true );

			this->completeReLogOnAttempt( pPendingReLogOn_->clientAddress(),
				pPendingReLogOn_->replyID(),
				pPendingReLogOn_->encryptionKey() );
		}

		if (!isDestroyed_)
		{
			Script::call( PyObject_GetAttrString( this, 
				  "onGiveClientToFailure" ),
				  PyTuple_New( 0 ), 
				  "Proxy.giveClientTo", /* okIfFnNull */ true );
		}

		return;
	}

	this->detachFromClient( /* shouldCondemn */ true );

	if (pPendingReLogOn_.get() != NULL)
	{
		// Client successfully given away, we can complete the re-logon.
		this->completeReLogOnAttempt( pPendingReLogOn_->clientAddress(),
			pPendingReLogOn_->replyID(), pPendingReLogOn_->encryptionKey() );
	}

	Script::call( PyObject_GetAttrString( this,
		  "onGiveClientToSuccess" ),
		  PyTuple_New( 0 ), 
		  "Proxy.giveClientTo", /* okIfFnNull */ true );
}


/**
 *  Finalise a session with the currently attached client.
 *
 *	@param shouldCondemn 	If true, the client channel is condemned (and so
 *				will continue to send acknowledgements for some
 *				time), otherwise, the channel is reset.
 */
void Proxy::detachFromClient( bool shouldCondemn )
{
	isGivingClientAway_ = false;

	if (this->hasClient())
	{
		BaseApp::instance().removeProxy( this );
	}

	if (pClientChannel_ != NULL)
	{
		// Put aside the pointer which setClientChannel
		// uses and then sets to NULL, so we can clean it up.
		pClientChannel_->pChannelListener( NULL );

		Mercury::ChannelPtr pSavedClientChannel = pClientChannel_;
		this->setClientChannel( NULL );

		if (pSavedClientChannel->isConnected())
		{
			if (shouldCondemn)
			{
				pSavedClientChannel->shutDown();
			}
			else
			{
				pSavedClientChannel->destroy();	
			}
		}
	}

	// Don't try to disable the witness if we've already sent the
	// destroyCell message.
	if (cellHasWitness_ && this->shouldSendToCell())
	{
		this->sendEnableDisableWitness( /*enable:*/false );
	}
	cellHasWitness_ = false;

	pBufferedClientBundle_.reset();

	pRateLimiter_ = NULL;
	sessionKey_ = 0;
	encryptionKey_ = BW::string();

	// don't tell the dest to enableEntities if the client had enabled them
	// with us... it waits for a new enableEntities from the client as an
	// ack of the change of base player entity.
	entitiesEnabled_ =  false;
	basePlayerCreatedOnClient_ = false;
	shouldRunCallbackOnEntitiesEnabled_ = false;

	// Delete the thing that makes us send packets if we still have one
	if (pProxyPusher_ != NULL)	// hadn't got cellEntityCreated notification yet
	{
		delete pProxyPusher_;
		pProxyPusher_ = NULL;
	}

	// Abort any remaining downloads now that we no longer have a
	// client channel. No new streams can be started now.
	// It's possible this has already been called, that's okay.
	DownloadCallbacks callbacks;
	dataDownloads_.abortDownloads( callbacks );
	callbacks.triggerCallbacks( this );
}


/**
 *  Send the client a disconnect message, then disconnect.
 *
 *	@param shouldCondemnChannel	True if any client channel should be condemned,
 *								false if it should be immediately destroyed.
 *								Generally true unless it was a client time-out
 *								or other situation where the client channel is
 *								probably no longer in a good state.
 */
void Proxy::logOffClient( bool shouldCondemnChannel )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	// Abort any pending downloads, and then tell the script about it after
	// the client has gone.
	DownloadCallbacks callbacks;
	dataDownloads_.abortDownloads( callbacks );

	if (this->isClientConnected())
	{
		// Send a message to the client telling it that it has been logged off.
		// The reason parameter is not yet used.
		Mercury::Bundle & bundle = pClientChannel_->bundle();
		ClientInterface::loggedOffArgs::start( bundle ).reason = 0;
		this->sendBundleToClient();
	}

	this->detachFromClient( shouldCondemnChannel );

	callbacks.triggerCallbacks( this );
}


/**
 *	This method queues some data to be sent to the client, which is
 *	opportunistically streamed to the client when bandwidth is available.
 *
 *	@returns A Python object representing the ID of the data download object.
 */
PyObject * Proxy::streamStringToClient( PyObjectPtr pData, PyObjectPtr pDesc,
	int id )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (pClientChannel_ == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "No client connected" );
		return NULL;
	}

	if (isGivingClientAway_)
	{
		PyErr_SetString( PyExc_ValueError,
			"Client is being given away" );
		return NULL;
	}

	StringDataDownloadFactory factory( pData );
	return dataDownloads_.streamToClient( factory, pDesc, id );
}


/**
 *  Queues a file to be opportunistically streamed to the client.
 *
 *	@returns A Python object representing the ID of the data download object.
 */
PyObject * Proxy::streamFileToClient( const BW::string & path,
	PyObjectPtr pDesc, int id )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (pClientChannel_ == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "No client connected" );
		return NULL;
	}

	if (isGivingClientAway_)
	{
		PyErr_SetString( PyExc_ValueError,
			"Client is being given away" );
		return NULL;
	}

	FileDataDownloadFactory factory( path );
	return dataDownloads_.streamToClient( factory, pDesc, id );
}


/**
 *	Pass ownership of the rate limiter and its associated callback object
 *	to this proxy.
 *
 *	@param pRateLimiter 		the RateLimitMessageFilter object
 */
RateLimitMessageFilterPtr Proxy::pRateLimiter()
{
	return pRateLimiter_;
}


/**
 *	Pass ownership of the rate limiter and its associated callback object
 *	to this proxy.
 *
 *	@param pRateLimiter 		the RateLimitMessageFilter object
 */
void Proxy::pRateLimiter( RateLimitMessageFilterPtr pRateLimiter )
{
	pRateLimiter_ = pRateLimiter;
	if (pRateLimiter_)
	{
		MF_ASSERT( pClientChannel_ != NULL );
		pClientChannel_->pMessageFilter( pRateLimiter.get() );
		pRateLimiter_->setProxy( this );
	}
}


/**
 *	This is a callback called when filter limits are exceeded.
 */
void Proxy::onFilterLimitsExceeded( const Mercury::Address & srcAddr, 
		BufferedMessage * pMessage )
{
	this->onClientDeath( CLIENT_DISCONNECT_RATE_LIMITS_EXCEEDED );
}

// ---------------------------------------------------------------------------
// Section: Instrumentation
// ---------------------------------------------------------------------------

/**
 *	This static function returns a watcher that can be used to watch objects of
 *	type Proxy.
 */
WatcherPtr Proxy::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (pWatcher == NULL)
	{
		pWatcher = new DirectoryWatcher();

		Proxy * pNull = NULL;

		// TODO: Much of this should be part of Base.

		// id
		pWatcher->addChild( "id", makeWatcher( &Proxy::id_ ) );

		pWatcher->addChild( "cell",
				new PolymorphicWatcher( "Proxy cell channel" ),
			&pNull->pChannel_ );

		pWatcher->addChild( "client",
				new PolymorphicWatcher( "Proxy client channel" ),
			&pNull->pClientChannel_ );

		pWatcher->addChild( "hasClient", makeWatcher( &Proxy::hasClient ) );

		pWatcher->addChild( "entitiesEnabled",
				makeWatcher( &Proxy::entitiesEnabled_ ) );

		pWatcher->addChild( "basePlayerCreatedOnClient",
				makeWatcher( &Proxy::basePlayerCreatedOnClient_ ) );

		pWatcher->addChild( "download/rate",
				makeWatcher( &Proxy::downloadRate_, Watcher::WT_READ_WRITE ) );

		pWatcher->addChild( "download/averageEarliestUnackedPacketAge",
				makeWatcher( &Proxy::avgUnackedPacketAge_ ) );

		pWatcher->addChild( "download/totalBytesDownloaded",
				makeWatcher( &Proxy::totalBytesDownloaded_ ) );

		if (!g_privateClientStatsInited)
		{
			g_privateClientStatsInited = true;
			g_privateClientStats.init( "eventProfiles/privateClientEvents" );
		}

		pWatcher->addChild( "avgClientBundleDataUnits",
			makeWatcher( &Proxy::avgClientBundleDataUnits ) );
	}

	return pWatcher;
}


namespace
{
DownloadStreamer g_downloadStreamer;
}

DownloadStreamer & Proxy::downloadStreamer()
{
	return g_downloadStreamer;
}


/**
 *	This method returns the bundle destined for the client channel.
 */
Mercury::Bundle & Proxy::clientBundle()
{ 
	return pClientChannel_ ? pClientChannel_->bundle() : 
		*pBufferedClientBundle_;
}


BW_END_NAMESPACE


// proxy.cpp
