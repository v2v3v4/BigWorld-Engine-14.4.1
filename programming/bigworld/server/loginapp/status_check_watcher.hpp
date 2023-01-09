#ifndef STATUS_CHECK_WATCHER_HPP
#define STATUS_CHECK_WATCHER_HPP

#include "cstdmf/watcher.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This watcher is used to check the current status of the system.
 */
class StatusCheckWatcher : public CallableWatcher
{
public:
	StatusCheckWatcher();

protected:
	virtual bool setFromStream( void * base,
			const char * path,
			WatcherPathRequestV2 & pathRequest );

private:
	class ReplyHandler : public Mercury::ShutdownSafeReplyMessageHandler
	{
	public:
		ReplyHandler( StatusCheckWatcher & rWatcher,
			WatcherPathRequestV2 & pathRequest );

	private:
		virtual void handleMessage( const Mercury::Address & source,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			void * arg );

		virtual void handleException( const Mercury::NubException & ne,
			void * arg );

		void sendResult( bool status, const BW::string & output );

		StatusCheckWatcher & rWatcher_;
		WatcherPathRequestV2 & pathRequest_;
	};

};

BW_END_NAMESPACE

#endif // STATUS_CHECK_WATCHER_HPP
