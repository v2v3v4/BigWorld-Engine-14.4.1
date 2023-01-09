#include "transfer_db.hpp"

#include "consolidate.hpp"
#include "snapshot.hpp"

#include "cstdmf/debug_filter.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/singleton.hpp"

#include "network/basictypes.hpp"
#include "network/machined_utils.hpp"

#include "server/bwconfig.hpp"

#include "cstdmf/bw_string.hpp"

#define APP_NAME "TransferDB"

#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )

BW_BEGIN_NAMESPACE

// TransferDB Singleton
BW_SINGLETON_STORAGE( TransferDB )


/**
 *	Constructor.
 */
TransferDB::TransferDB() :
//	eventDispatcher_(),
	pEventDispatcher_( 0 ),
	pWatcherNub_( 0 ),
	pLoggerMessageForwarder_( 0 )
{
}


/**
 *	Destructor.
 */
TransferDB::~TransferDB()
{
}


/**
 *
 */
bool TransferDB::init( bool isVerbose )
{
	if (!this->initLogger( APP_NAME, BWConfig::get( "loggerID", "" ), 
			isVerbose ))
	{
		ERROR_MSG( "TransferDB::init: Failed to initialise logger\n" );
		return false;
	}

	return true;
}


/**
 *	This method is called from a child process to close all the sockets and
 *	file descriptors that were opened in the parent.
 */
void TransferDB::onChildAboutToExec()
{
	pLoggerMessageForwarder_.reset();
	pWatcherNub_.reset();
	pEventDispatcher_.reset();
}


// TODO: copied from DatabaseToolApp... make into common code for both
/**
 *
 */
bool TransferDB::initLogger( const char * appName, 
		const BW::string & loggerID, bool isVerbose )
{

	// Get the internal IP
	uint32 internalIP;
	if (!Mercury::MachineDaemon::queryForInternalInterface( internalIP ))
	{
		ERROR_MSG( "TransferDB::initLogger: "
				"failed to query for internal interface\n" );
		return false;
	}

	// Set up the watcher nub and message forwarder manually
	pWatcherNub_.reset( new WatcherNub() );

	if (!pWatcherNub_->init( inet_ntoa( (struct in_addr &)internalIP ), 0 ))
	{
		pWatcherNub_.reset( NULL );
		return false;
	}

	pEventDispatcher_.reset( new Mercury::EventDispatcher );

	pLoggerMessageForwarder_.reset( 
		new LoggerMessageForwarder( appName, pWatcherNub_->udpSocket(), 
			*pEventDispatcher_, loggerID,
			/*enabled=*/true, /*spamFilterThreshold=*/0 ) );

	DebugFilter::shouldWriteToConsole( true );
	if (!isVerbose)
	{
		DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_INFO );
	}
	else
	{
		DebugFilter::shouldWriteToConsole( true );
	}

	return true;
}


/**
 *	This method is a simple wrapper to handle the command line arguments for
 *	the 'consolidate' command and pass them through to the Consolidate class.
 */
bool TransferDB::consolidate( BW::string secondaryDB, BW::string sendToAddr )
{
	//const char *secondaryDB = argv[ ++i ];
	//BW::string sendToAddr( argv[ ++i ] );

	size_t tokenPos = sendToAddr.rfind( ":" );
	if (tokenPos == BW::string::npos)
	{
		ERROR_MSG( "TransferDB::consolidate: "
			"Address provided appears invalid '%s'. Expecting <ip:port>\n",
			sendToAddr.c_str() );
		return false;
	}

	BW::string ipAddrStr = sendToAddr.substr( 0, tokenPos );
	BW::string portStr = sendToAddr.substr( tokenPos + 1 );

	Mercury::Address addr;
	if (Endpoint::convertAddress( ipAddrStr.c_str(), addr.ip ) == -1)
	{
		ERROR_MSG( "TransferDB::consolidate: "
			"Unable to convert string address '%s'.\n",
			ipAddrStr.c_str() );
		return false;
	}

// TODO:
//			strtoul( );
	addr.port = ntohs( atoi( portStr.c_str() ) );

	Consolidate consolidate( secondaryDB );

	return consolidate.transferTo( addr );
}


/**
 *
 */
bool TransferDB::snapshotPrimary( BW::string destinationIP,
			BW::string destinationPath, BW::string limitKbps )
{

	Snapshot snapshot;

	if (!snapshot.init( destinationIP, destinationPath, limitKbps ))
	{
		ERROR_MSG( "TransferDB::snapshotPrimary: "
			"Failed to initialise the snapshotter. Terminating.\n" );
		return false;
	}

	bool wasSnapshotSuccessful = snapshot.transferPrimary();
	if (wasSnapshotSuccessful)
	{
		INFO_MSG( "TransferDB::snapshotPrimary: Completed.\n" );
	}

	return wasSnapshotSuccessful;
}


/**
 *
 */
bool TransferDB::snapshotSecondary( BW::string secondaryDB,
		BW::string destinationIP, BW::string destinationPath,
		BW::string limitKbps )
{

	Snapshot snapshot;

	if (!snapshot.init( destinationIP, destinationPath, limitKbps ))
	{
		return false;
	}

	bool wasSnapshotSuccessful = snapshot.transferSecondary( secondaryDB );
	if (wasSnapshotSuccessful)
	{
		INFO_MSG( "TransferDB::snapshotSecondary: "
			"Completed transfer of '%s'\n", secondaryDB.c_str() );
	}

	return wasSnapshotSuccessful;
}

BW_END_NAMESPACE

// transfer_db.cpp
