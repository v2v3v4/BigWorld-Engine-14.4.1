#ifndef SNAPSHOT_HPP
#define SNAPSHOT_HPP

#include "network/event_dispatcher.hpp"

#include "server/signal_processor.hpp"

#include "cstdmf/bw_string.hpp"
#include <memory>


BW_BEGIN_NAMESPACE

class TransferDB;

class Snapshot
{
public:
	Snapshot();

	bool init( BW::string destinationIP, BW::string destinationPath,
				BW::string limitKbps );

	bool transferPrimary();
	bool transferSecondary( const BW::string & sqlitePath );

	void onChildAboutToExec();

private:
	BW::string destination() const;

	bool isSnapshotHelperSetUidRoot() const;
	bool takePrimarySnapshot( BW::string & pathToSnapshot );
	bool removePrimarySnapshot();

	bool transferSnapshotViaRsync( BW::string srcPath );
	bool sendSnapshot( BW::string srcPath );
	bool sendCompletionFile( BW::string srcPath );

 	// Methods for secondary database snapshotting
	bool backupSqliteDatabase( const BW::string & srcPath,
		const BW::string & destPath );


	BW::string destinationIP_;
	BW::string destinationPath_;
	BW::string limitKbps_;

	std::auto_ptr< SignalProcessor > pSignalProcessor_;
};

BW_END_NAMESPACE

#endif // SNAPSHOT_HPP
