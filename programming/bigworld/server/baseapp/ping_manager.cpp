#ifdef USE_OPENSSL
#define USE_PING_MANAGER 1
#endif

#include "cstdmf/bw_namespace.hpp"

#ifdef USE_PING_MANAGER

#include "cstdmf/bwversion.hpp"
#include "cstdmf/time_queue.hpp"

#include "network/endpoint.hpp"
#include "network/event_dispatcher.hpp"
#include "network/misc.hpp"
#include "network/network_interface.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/datasection.hpp"

#include "openssl/err.h"
#include "openssl/pem.h"

#include "cstdmf/bw_string.hpp"

#else // USE_PING_MANAGER
BW_BEGIN_NAMESPACE
namespace Mercury {
class EventDispatcher;
class NetworkInterface;
} // Mercury
BW_END_NAMESPACE
#endif // USE_PING_MANAGER

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Ping Manager
// -----------------------------------------------------------------------------

namespace PingManager
{

#ifdef USE_PING_MANAGER
/**
 * The Ping Manager.
 */
class PingManager : public TimerHandler
{
public:
	/**
	 * Constructor.
	 */
	PingManager( Mercury::EventDispatcher & dispatcher,
		const Mercury::Address & bindAddr,
		unsigned int periodMin,
		unsigned int periodMax );

	virtual ~PingManager();

	void addDestination( const char * hostName, uint16 port );

	bool init();

public: // from TimerHandler
	virtual void handleTimeout( TimerHandle handle, void * arg );


private:
	unsigned long randomPeriod() const;
	void printError( const char * location ) const;

private:
	static const unsigned long UNIT_MS = 1000000L;
	static const char PING_KEY[];
	static const char * KEY_FILE;

	BW::string			pingContents_;
	unsigned int 		periodMin_;
	unsigned int		periodMax_;
	TimerHandle			timerHandle_;
	Endpoint			ep_;

	typedef BW::vector< Mercury::Address >	Destinations;
	Destinations 		destinations_;

	Mercury::EventDispatcher &		dispatcher_;
	Mercury::Address  				bindAddr_;
};

namespace
{

const char PINGMANAGER_PRIMARY_HOST[] = "59.167.229.195";
const uint16 PINGMANAGER_PRIMARY_PORT = 30000;

const char PINGMANAGER_SECONDARY_HOST[] = "64.85.165.94";
const uint16 PINGMANAGER_SECONDARY_PORT = 30000;

} // namespace anonymous

PingManager * g_pPingManager = NULL;
const char PingManager::PING_KEY[] =
	"-----BEGIN PUBLIC KEY-----\n"
	"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC13HH1AO8Un9A3L0wqDZ0Z79dq\n"
	"UDfFK70KxbyZWZcZlPPUIe2zyzVK7v6g7aJJwYiFgqqjR5/zFWLmzS1ElUXPw+Dz\n"
	"aga+/GPuikNln1fmjr7Vgtjbo6sCRMfw/XbP3eU6a74wJM9ROHPyx/7x1d8aht++\n"
	"eZ2ZxdwS6WOSlZotxQIDAQAB\n"
	"-----END PUBLIC KEY-----\n";

const char * PingManager::KEY_FILE = "server/bigworld.key";


// -----------------------------------------------------------------------------
// Section: Ping Manager class
// -----------------------------------------------------------------------------

PingManager::PingManager(
		Mercury::EventDispatcher & dispatcher,
		const Mercury::Address & bindAddr,
		unsigned int periodMin,
		unsigned int periodMax ) :
	TimerHandler(),
	periodMin_( periodMin ),
	periodMax_( periodMax ),
	timerHandle_(),
	ep_(),
	destinations_(),
	dispatcher_( dispatcher ),
	bindAddr_( bindAddr )
{
}

/**
 *	Destructor.
 */
PingManager::~PingManager()
{
	timerHandle_.cancel();
}


/**
 *	Add a destination to ping.
 *
 *	@param hostName 	The host IP as a dotted decimal string.
 *	@param port 		The host port (host order).
 */
void PingManager::addDestination( const char * hostName, uint16 port )
{
	u_int32_t pingManagerHost;
	Endpoint::convertAddress( hostName, pingManagerHost );
	Mercury::Address dest( pingManagerHost, htons( port ) );
	destinations_.push_back( dest );
}


/**
 *	Override from TimerHandler::handleTimeout.
 *
 *	@param handle	The timer handle.
 *	@param arg		An opaque data argument associated with the timer.
 */
void PingManager::handleTimeout( TimerHandle handle, void * arg )
{
	// create end point
	ep_.socket( SOCK_DGRAM );
	ep_.bind( 0, bindAddr_.ip );

	// Send to each destination
	Destinations::iterator iDest = destinations_.begin();
	while (iDest != destinations_.end())
	{
		ep_.sendto(
			const_cast< char * >( pingContents_.data() ),
			pingContents_.size(),
			iDest->port, iDest->ip );
		++iDest;
	}
	ep_.close();

	// reset timer with new random period
	timerHandle_.cancel();
	timerHandle_ = dispatcher_.addTimer( randomPeriod() * UNIT_MS, this, NULL,
		"PingManager" );
}


/**
 *	Initialise the ping manager.
 *
 *	@return True if successful, false otherwise.
 */
bool PingManager::init()
{
	DataSectionPtr pKeySection =
		BWResource::instance().rootSection()->openSection( KEY_FILE );

	if (!pKeySection)
	{
		ERROR_MSG( "No key file %s found.\n"
			"Either place your %s file at the root of your resource tree (for\n"
			"example bigworld/res/%s) or contact support@bigworldtech.com for a "
			"key.\n", KEY_FILE, KEY_FILE, KEY_FILE );

		return false;
	}

	BinaryPtr pKeyBinary = pKeySection->asBinary();
	BW::string key( pKeyBinary->cdata(), pKeyBinary->len() );

	BIO * mem = BIO_new( BIO_s_mem() );

	if (!mem)
	{
		this->printError( "BIO_new" );
		return false;
	}

	BIO_puts( mem, PING_KEY );
	RSA * pRSAKey = PEM_read_bio_RSA_PUBKEY( mem, 0, 0, 0 );
	BIO_free( mem );

	if (!pRSAKey)
	{
		this->printError( "PEM_read_bio_RSAPublicKey" );
		return false;
	}

	char * decrypted = new char[ RSA_size( pRSAKey ) ];

	int decryptedSize = RSA_public_decrypt( key.size(),
			(unsigned char *)key.data(),
			(unsigned char *)decrypted, pRSAKey, RSA_PKCS1_PADDING );

	bool isOkay = (decryptedSize != -1);

	if (isOkay)
	{
		const BW::string & versionString = BWVersion::versionString();
		char pad[] = "w\x03H\x12\xac/P\xd7\xaf\xa8\xc4^";
		int versionSize = std::min( versionString.size(),
				std::min( sizeof( pad ), size_t( decryptedSize ) ) );

		memcpy( decrypted, versionString.data(), versionSize );

		for (int i = 0; i < versionSize; ++i)
		{
			decrypted[i] ^= pad[i];
		}

		pingContents_.assign( decrypted, decryptedSize );

		// start timer
		timerHandle_ = dispatcher_.addTimer( this->randomPeriod() * UNIT_MS,
			this, NULL, "PingManager" );
	}
	else
	{
		this->printError( "RSA_public_decrypt" );
	}

	delete [] decrypted;

	return isOkay;
}


void PingManager::printError( const char * location ) const
{
	ERROR_MSG( "Invalid key file %s found.\n"
		"Either update your %s file at the root of your resource tree (for\n"
		"example bigworld/res/%s) or contact support@bigworldtech.com for a\n"
		"new key.\n", KEY_FILE, KEY_FILE, KEY_FILE );

#if 0
	ERR_load_crypto_strings();
	ERROR_MSG( "PingManager::init: %s failed - %s\n",
		location,
		ERR_error_string( ERR_get_error(), NULL ) );
#endif
}



unsigned long PingManager::randomPeriod() const
{
	// this doesn't have to be perfectly random
	const unsigned long mask = 0xFFFFL;
	uint64 ts = timestamp();

	// least significant two bytes
	unsigned int tsLsw =  ( unsigned int ) ( ts & mask );

	if( ( ( 1 << 16 ) - 1 ) < ( periodMax_ - periodMin_ ) )
	{
		// need to scale this
		tsLsw *= (periodMax_ - periodMin_) / ( ( 1 << 16 ) - 1 );
	}

	unsigned int out = ( ( tsLsw % ( periodMax_ - periodMin_ ) ) +
		periodMin_ );

	return out;
}

#endif //USE_PING_MANAGER


/**
 *	Initialise the Ping Manager singleton.
 *
 *	@param dispatcher			An event dispatcher.
 *	@param networkInterface 	A network interface.
 *
 *	@return True if successful, false otherwise.
 */
bool init( Mercury::EventDispatcher & dispatcher,
		Mercury::NetworkInterface & networkInterface )
{
#ifdef USE_PING_MANAGER
	g_pPingManager = new PingManager(
			dispatcher,
			networkInterface.address(),
			15 * 60,
			30 * 60 );

	g_pPingManager->addDestination( PINGMANAGER_PRIMARY_HOST, 
		PINGMANAGER_PRIMARY_PORT );
	g_pPingManager->addDestination( PINGMANAGER_SECONDARY_HOST, 
		PINGMANAGER_SECONDARY_PORT );

	return g_pPingManager->init();
#endif // USE_PING_MANAGER
}


/**
 *	Finalise the Ping Manager singleton.
 */
void fini()
{
#ifdef USE_PING_MANAGER
	delete g_pPingManager;
	g_pPingManager = NULL;
#endif
}

} // namespace PingManager

BW_END_NAMESPACE

// ping_manager.cpp

