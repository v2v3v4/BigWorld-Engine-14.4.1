#ifndef TRANSFER_DB_HPP
#define TRANSFER_DB_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/singleton.hpp"

#include "network/event_dispatcher.hpp"
#include "network/watcher_nub.hpp"
#include "network/logger_message_forwarder.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class TransferDB : public Singleton< TransferDB >
{
public:
	TransferDB();
	~TransferDB();

	bool init( bool isVerbose );

	// Command methods
	bool consolidate( BW::string secondaryDB, BW::string sendToAddr );

	bool snapshotPrimary( BW::string destinationIP,
			BW::string destinationPath, BW::string limitKbps );

	bool snapshotSecondary( BW::string secondaryDB, BW::string destinationIP,
			BW::string destinationPath, BW::string limitKbps );

	// General accessors

	Mercury::EventDispatcher & dispatcher()
		{ return *pEventDispatcher_; }


	// Methods called from a child process after fork()
	void onChildAboutToExec();

private:
	bool initLogger( const char * appName, 
		const BW::string & loggerID, bool isVerbose );


	std::auto_ptr< Mercury::EventDispatcher > pEventDispatcher_;

	std::auto_ptr< WatcherNub > pWatcherNub_;

	std::auto_ptr< LoggerMessageForwarder > pLoggerMessageForwarder_;
};

BW_END_NAMESPACE

#endif // TRANSFER_DB_HPP
