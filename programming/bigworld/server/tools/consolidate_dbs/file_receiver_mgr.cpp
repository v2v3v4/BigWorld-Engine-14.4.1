#include "file_receiver_mgr.hpp"

#include "db_consolidator_errors.hpp"
#include "file_receiver.hpp"
#include "file_transfer_progress_reporter.hpp"

#include "cstdmf/debug.hpp"

#include "network/basictypes.hpp"
#include "network/event_dispatcher.hpp"

DECLARE_DEBUG_COMPONENT2( "SecondaryDB", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
FileReceiverMgr::FileReceiverMgr( Mercury::EventDispatcher & dispatcher,
		FileTransferProgressReporter & progressReporter,
		const SecondaryDBInfos & secondaryDBs,
		const BW::string & consolidationDir ) :
	dispatcher_( dispatcher ),
	progressReporter_( progressReporter ),
	consolidationDir_( consolidationDir )
{
	for (SecondaryDBInfos::const_iterator i = secondaryDBs.begin();
			i != secondaryDBs.end(); ++i)
	{
		// All paths should be unique.
		MF_VERIFY( unfinishedDBs_.insert(
				SourceDBs::value_type( i->location, i->hostIP ) ).second );
	}
}


/**
 * 	Destructor.
 */
FileReceiverMgr::~FileReceiverMgr()
{
	this->cleanUpLocalFiles();

	for (ReceiverSet::iterator iReceiver = startedReceivers_.begin();
			iReceiver != startedReceivers_.end();
			++iReceiver)
	{
		delete *iReceiver;
	}

	for (Receivers::iterator iReceiver = completedReceivers_.begin();
			iReceiver != completedReceivers_.end();
			++iReceiver)
	{
		delete *iReceiver;
	}
}


/**
 *	Called by TcpListener if listener bind failed.
 */
void FileReceiverMgr::onFailedBind( uint32 ip, uint16 port )
{
	MF_ASSERT( (ip = INADDR_ANY) && (port = 0) );
	ERROR_MSG( "FileReceiverMgr::onFailedBind: Failed to bind to any port.\n" );
}


/**
 *	Called by TcpListener if we failed to accept incoming connection.
 */
void FileReceiverMgr::onFailedAccept( uint32 ip, uint16 port )
{
	Mercury::Address addr( ip, port );
	ERROR_MSG( "FileReceiverMgr::onFailedAccept: Failed to accept incoming "
			"connection from %s\n", 
		addr.c_str() );
}


/**
 * 	Called by FileReceiver when they finished receiving a file.
 */
void FileReceiverMgr::onFileReceived( FileReceiver& receiver )
{
	SourceDBs::iterator pSourceDB = unfinishedDBs_.find( receiver.srcPath() );
	if (pSourceDB != unfinishedDBs_.end())
	{
		unfinishedDBs_.erase( pSourceDB );
	}
	else
	{
		ERROR_MSG( "DBConsolidator: Received unknown file '%s' from %s\n",
			receiver.srcPath().c_str(), receiver.srcAddr().c_str() );
	}

	MF_VERIFY( startedReceivers_.erase( &receiver ) == 1 );
	completedReceivers_.push_back( &receiver );
	receivedFilePaths_.push_back( receiver.destPath() );

	progressReporter_.onFinishTransfer();

	if (unfinishedDBs_.empty())
	{
		// Break processing. This will be picked up by DBConsolidator.
		dispatcher_.breakProcessing();
	}
}

/**
 * 	Called by to notify us of an error in file transfer.
 */
void FileReceiverMgr::onFileReceiveError()
{
	ERROR_MSG( "FileReceiverMgr:onFileReceiveError: Aborting file "
			"transfer!\n" );

	// Break processing. This will be picked up by DBConsolidator.
	dispatcher_.breakProcessing();
}

/**
 * 	Returns true if we've finished receiving all our files.
 */
bool FileReceiverMgr::finished() const
{
	return unfinishedDBs_.empty();
}

/**
 *	This function takes a mighty good guess as to which databases still have
 *	not started their transfer.
 */
FileReceiverMgr::SourceDBs FileReceiverMgr::getUnstartedDBs() const
{
	SourceDBs unstartedDBs = unfinishedDBs_;
	Receivers unstartedReceivers;

	// Remove databases that have started transferring.
	for (ReceiverSet::const_iterator ppReceiver = startedReceivers_.begin();
			ppReceiver != startedReceivers_.end(); 
			++ppReceiver)
	{
		if (!(*ppReceiver)->srcPath().empty())
		{
			SourceDBs::iterator pFound =
					unstartedDBs.find( (*ppReceiver)->srcPath() );

			if (pFound != unstartedDBs.end())
			{
				unstartedDBs.erase(pFound);
			}
			else
			{
				ERROR_MSG( "FileReceiverMgr::getUnstartedDBs: Cannot find %s\n",
						(*ppReceiver)->srcPath().c_str() );
			}
		}
		else
		{
			// Connected but haven't yet transferred their source path.
			unstartedReceivers.push_back( *ppReceiver );
		}
	}

	if (!unstartedReceivers.empty())
	{
		// Have to work out which entry to remove for those that have
		// connected but not sent their source path.

		// Make IP address to unstartedDBs item map.
		typedef BW::multimap< uint32, SourceDBs::iterator > IPToSrcDB;
		IPToSrcDB	ipToSrcDB;
		for ( SourceDBs::iterator i = unstartedDBs.begin();
				i != unstartedDBs.end(); ++i )
		{
			ipToSrcDB.insert( IPToSrcDB::value_type( i->second, i ) );
		}

		// Now remove secondary DBs that have connected but haven't started
		// their transfer.
		for (Receivers::const_iterator ppReceiver = unstartedReceivers.begin();
				ppReceiver != unstartedReceivers.end(); 
				++ppReceiver)
		{
			// __kyl__(6/8/2008) We are removing some random entry in
			// unstartedDBs that matches the IP address. It could be the wrong
			// entry but doesn't matter much at the moment since we're only
			// using this to print out error messages.
			IPToSrcDB::iterator i = ipToSrcDB.find( (*ppReceiver)->srcAddr().ip );
			MF_ASSERT( i != ipToSrcDB.end() );
			unstartedDBs.erase( i->second );
		}
	}

	return unstartedDBs;
}

/**
 * 	Deletes the local copies of the secondary DB files.
 */
bool FileReceiverMgr::cleanUpLocalFiles()
{
	bool isOK = true;
	for (Receivers::iterator i = completedReceivers_.begin();
			i != completedReceivers_.end(); 
			++i)
	{
		isOK = (*i)->deleteLocalFile() && isOK;
	}

	for (ReceiverSet::iterator i = startedReceivers_.begin();
			i != startedReceivers_.end(); 
			++i)
	{
		(*i)->abort();
		isOK = (*i)->deleteLocalFile() && isOK;
	}

	return isOK;
}

/**
 * 	Sends a message to delete the remote secondary database files, except
 * 	for those secondary databases that had errors when we tried to
 * 	consolidate them (i.e. those in errorDBs).
 */
bool FileReceiverMgr::cleanUpRemoteFiles( 
		const DBConsolidatorErrors & errorDBs )
{
	MF_ASSERT( finished() );

	bool isOK = true;

	for (Receivers::iterator iReceiver = completedReceivers_.begin();
			iReceiver != completedReceivers_.end(); 
			++iReceiver)
	{
		FileReceiver & receiver = **iReceiver;
		const BW::string & secondaryDBPath = receiver.destPath();

		if (!errorDBs.secondaryDBHasError( secondaryDBPath ))
		{
			isOK = receiver.deleteRemoteFile() && isOK;
		}
		else
		{
			WARNING_MSG( "FileReceiverMgr::cleanUpRemoteFiles: "
					"Skipped deletion of secondary database file %s on %s "
					"because there were errors during consolidation\n",
				secondaryDBPath.c_str(), receiver.srcAddr().ipAsString() );
		}
	}

	return isOK;
}


/**
 *	
 */
void FileReceiverMgr::onAcceptedConnection( int socket, uint32 ip, 
		uint16 port )
{
	in_addr addr = {ip};
	DEBUG_MSG( "FileReceiverMgr::onAcceptedConnection: "
			"got new connection from %s:%hu\n",
		inet_ntoa( addr ), ntohs( port ) );
	startedReceivers_.insert( new FileReceiver( socket, ip, port, *this ) );
}

BW_END_NAMESPACE

// file_receiver_mgr.cpp

