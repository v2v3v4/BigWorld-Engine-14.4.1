#ifndef RELOGON_ATTEMPT_HANDLER_HPP
#define RELOGON_ATTEMPT_HANDLER_HPP

#include "cstdmf/timer_handler.hpp"

#include "connection/log_on_params.hpp"

#include "db_storage/idatabase.hpp"

#include "network/udp_bundle.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class is used to receive the reply from a createEntity call to
 *	BaseAppMgr during a re-logon operation.
 */
class RelogonAttemptHandler : public Mercury::ReplyMessageHandler,
							public TimerHandler
{
public:
	RelogonAttemptHandler( EntityTypeID entityTypeID,
			DatabaseID dbID,
			const Mercury::Address & replyAddr,
			Mercury::ReplyID replyID,
			LogOnParamsPtr pParams,
			const Mercury::Address & addrForProxy,
			const BW::string & dataForClient );

	virtual ~RelogonAttemptHandler();

	// Mercury::ReplyMessageHandler methods
	virtual void handleMessage( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg );

	virtual void handleException( const Mercury::NubException & exception,
		void * arg );

	virtual void handleShuttingDown( const Mercury::NubException & exception,
		void * arg );

	void onEntityLogOff();

	// TimerHandler methods
	virtual void handleTimeout( TimerHandle handle, void * pUser );

	double ageInSeconds() const { return creationTime_.ageInSeconds(); }

private:
	void terminateRelogonAttempt( const char *clientMessage );

	void abort();

	bool 					hasAborted_;
	EntityDBKey				ekey_;
	Mercury::Address 		replyAddr_;
	Mercury::ReplyID 		replyID_;
	LogOnParamsPtr 			pParams_;
	Mercury::Address 		addrForProxy_;
	Mercury::UDPBundle		replyBundle_;

	TimerHandle				waitForDestroyTimer_;

	TimeStamp				creationTime_;

	BW::string				dataForClient_;
};

BW_END_NAMESPACE

#endif // RELOGON_ATTEMPT_HANDLER_HPP
