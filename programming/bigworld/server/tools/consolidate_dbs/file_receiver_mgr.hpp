#ifndef CONSOLIDATE_DBS__FILE_RECEIVER_MGR_HPP
#define CONSOLIDATE_DBS__FILE_RECEIVER_MGR_HPP

#include "secondary_db_info.hpp"

#include "cstdmf/shared_ptr.hpp"
#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
	class EventDispatcher;
}

class DBConsolidatorErrors;
class FileReceiver;
class FileTransferProgressReporter;

typedef BW::vector< BW::string >	FileNames;

/**
 *	Receives secondary database files.
 */
class FileReceiverMgr
{
public:
	FileReceiverMgr( Mercury::EventDispatcher & dispatcher,
			FileTransferProgressReporter & progressReporter,
			const SecondaryDBInfos & secondaryDBs,
			const BW::string & consolidationDir );
	~FileReceiverMgr();

	Mercury::EventDispatcher & dispatcher()
		{ return dispatcher_; }

	FileTransferProgressReporter & progressReporter()
		{ return progressReporter_; }

	const BW::string & consolidationDir() const
		{ return consolidationDir_; }

	bool finished() const;

	const FileNames & receivedFilePaths() const
		{ return receivedFilePaths_; }

	bool cleanUpLocalFiles();
	bool cleanUpRemoteFiles( const DBConsolidatorErrors & errorDBs );

	// Called by TcpListener
	void onAcceptedConnection( int socket, uint32 ip, uint16 port );

	// Called by FileReceiver
	void onFileReceiverStart( FileReceiver & receiver );
	void onFileReceived( FileReceiver & receiver );

	// Called to notify us of an error.
	void onFileReceiveError();

	// Map of file location to host IP address.
	bool hasUnstartedDBs() const
		{ return ((unfinishedDBs_.size() - startedReceivers_.size()) > 0); }

	typedef BW::map< BW::string, uint32 >	SourceDBs;
	SourceDBs getUnstartedDBs() const;

	typedef BW::set< FileReceiver* > ReceiverSet;
	const ReceiverSet& startedReceivers() const 
		{ return startedReceivers_; }

	static void onFailedBind( uint32 ip, uint16 port );
	static void onFailedAccept( uint32 ip, uint16 port );

private:
	Mercury::EventDispatcher &		dispatcher_;
	FileTransferProgressReporter & 	progressReporter_;
	BW::string						consolidationDir_;
	SourceDBs						unfinishedDBs_;
	ReceiverSet						startedReceivers_;
	typedef BW::vector< FileReceiver * > Receivers;
	Receivers						completedReceivers_;
	FileNames						receivedFilePaths_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATE_DBS__FILE_RECEIVER_MGR_HPP
