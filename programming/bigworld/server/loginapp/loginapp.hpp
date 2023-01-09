#ifndef LOGINAPP_HPP
#define LOGINAPP_HPP

#include "client_login_request.hpp"
#include "login_int_interface.hpp"

#include "connection/log_on_params.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection/login_reply_record.hpp"

#include "cstdmf/bw_vector.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/singleton.hpp"

#include "network/channel_owner.hpp"
#include "network/event_dispatcher.hpp"
#include "network/netmask.hpp"
#include "network/public_key_cipher.hpp"
#include "network/interfaces.hpp"
#include "network/tcp_server.hpp"

#include "resmgr/datasection.hpp"
#include "server/server_app.hpp"
#include "server/stream_helper.hpp"

#include "math/ema.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

class LoginAppConfig;
class StreamEncoder;

namespace Mercury
{
class EventDispatcher;
class NetworkInterface;
}

typedef Mercury::ChannelOwner DBApp;


/**
 *	This class implements the main singleton object in the login application.
 */
class LoginApp : public ServerApp, public TimerHandler,
	public Singleton< LoginApp >
{
public:
	SERVER_APP_HEADER( LoginApp, loginApp )

	typedef LoginAppConfig Config;

	LoginApp( Mercury::EventDispatcher & mainDispatcher,
			Mercury::NetworkInterface & interface );
	~LoginApp();

	bool finishInit( LoginAppID appID,
		const Mercury::Address & dbAppAlphaAddress );

	// Overrides from TimerHandler
	void handleTimeout( TimerHandle handle, void * arg ) /* override */;

	// external methods

	void login( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void probe( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void challengeResponse( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	// internal methods
	void controlledShutDown( const Mercury::Address & source,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data );

	void handleDBAppMgrBirth(
		const LoginIntInterface::handleDBAppMgrBirthArgs & args );

	void notifyDBAppAlpha(
		const LoginIntInterface::notifyDBAppAlphaArgs & args );

	void handleFailure( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID, int status,
		const char * msg = NULL, LogOnParamsPtr pParams = NULL );

	void sendAndCacheSuccess( const Mercury::Address & addr,
			Mercury::Channel * pChannel, Mercury::ReplyID replyID, 
			const LoginReplyRecord & replyRecord,
			const BW::string & serverMsg, LogOnParamsPtr pParams );

	void handleBanIP( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		LogOnParamsPtr pParams, ::time_t banDuration );

	Mercury::NetworkInterface &	intInterface()		{ return interface_; }
	Mercury::NetworkInterface &	extInterface()		{ return extInterface_; }

	DBApp & dbAppAlpha()						{ return dbAppAlpha_; }
	const DBApp & dbAppAlpha() const			{ return dbAppAlpha_; }

	Mercury::ChannelOwner & dbAppMgr()	{ return *dbAppMgr_.pChannelOwner(); }
	const Mercury::ChannelOwner & dbAppMgr() const
	{
		return *dbAppMgr_.pChannelOwner();
	}

	bool isDBReady() const
	{
		return dbAppAlpha_.channel().isEstablished();
	}

	bool isDBAppMgrReady() const
	{
		return this->dbAppMgr().channel().isEstablished();
	}

	void controlledShutDown();
	
	void clearIPAddressBans();

	uint8 systemOverloaded() const
	{ return systemOverloaded_; }

	void systemOverloaded( uint8 status )
	{
		systemOverloaded_ = status;
		systemOverloadedTime_ = timestamp();
	}

	BW::string challengeType() const;

	void challengeType( BW::string value );

private:
	// From ServerApp
	bool init( int argc, char * argv[] ) /* override */;
	void onRunComplete() /* override */;
	void onSignalled( int sigNum ) /* override */;

	bool initLogOnParamsEncoder();

	bool handleResentPendingAttempt( const Mercury::Address & addr,
		Mercury::ReplyID replyID );
	bool handleResentCachedAttempt( const Mercury::Address & addr,
		LogOnParamsPtr pParams, Mercury::ReplyID replyID );
	bool processForLoginChallenge( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		BinaryIStream & data );

	void sendSuccess( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		const ClientLoginRequest & request );

	void sendChallengeReply( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		const BW::string & challengeType,
		LoginChallengePtr pChallenge );

	void sendRawReply( const Mercury::Address & addr,
		Mercury::Channel * pChannel, Mercury::ReplyID replyID,
		BinaryIStream & payload );

	void onChallengeTypeModified( const BW::string & oldValue,
			const BW::string & newValue );

	std::auto_ptr< StreamEncoder > 	pLogOnParamsEncoder_;
	Mercury::NetworkInterface		extInterface_;
	std::auto_ptr< Mercury::StreamFilterFactory > 
									pStreamFilterFactory_;
	Mercury::TCPServer 				tcpServer_;

	uint8				systemOverloaded_;
	uint64				systemOverloadedTime_;

	typedef BW::map< Mercury::Address, ClientLoginRequest > ClientLoginRequests;
	ClientLoginRequests loginRequests_;

	LoginChallengeFactories challengeFactories_;

	DBApp					dbAppAlpha_;

	AnonymousChannelClient	dbAppMgr_;

	// TS when to reset the counter below
	uint64 				repliedFailsCounterResetTime_;
	// decreasing from Config::maxErrRepliesPerSec
	uint				numFailRepliesLeft_;

	// Rate Limiting state

	// the time of the start of the last time block
	uint64 				lastRateLimitCheckTime_;
	// the number of logins left for this time block
	uint				numAllowedLoginsLeft_;

	// client_IP_address -> ban_end_time
	typedef BW::map< uint32, uint64 > IPAddressBanMap;
	IPAddressBanMap		ipAddressBanMap_;
	uint64				nextIPAddressBanMapCleanupTime_;

	LoginAppID id_;

	/**
	 *	This class represents login statistics. These statistics are exposed to
	 *	watchers.
	 */
	class LoginStats: public TimerHandler
	{
	public:
		LoginStats();

		/** Destructor. */
		~LoginStats() {}

		/*
		 *	Overridde from TimerHandler.
		 */
		void handleTimeout( TimerHandle handle, void * arg ) /* override */
		{
			this->update();
		}

		// Incrementing accessors

		/**
		 *	Increment the count for rate-limited logins.
		 */
		void incRateLimited() 	{ ++all_.value(); ++rateLimited_.value(); }

		/**
		 *	Increment the count for failed logins.
		 */
		void incFails() 		{ ++all_.value(); ++fails_.value(); }

		/**
		 *	Increment the count for repeated logins (duplicate logins that came
		 *	in from the client while the original was pending.
		 */
		void incPending() 		{ ++all_.value(); ++pending_.value(); }

		/**
		 *	Increment the count for successful logins.
		 */
		void incSuccesses() 	{ ++all_.value(); ++successes_.value(); }


		/**
		 *	This method adds a sample for a successful challenge calculation
		 *	duration.
		 */
		void challengeCalculationTimeSample( float sample )
		{
			calculationTime_.sample( sample );
		}


		/**
		 *	This method adds a sample for a challenge verification duration.
		 */
		void challengeVerificationTimeSample( float sample )
		{
			verificationTime_.sample( sample );
		}

		/**
		 *	Increment the count for failedByIPAddressBan logins.
		 */
		void incFailedByIPAddressBan() { ++failedByIPAddressBan_.value(); }

		/**
		 *	Increment the count for attemptsWithPassword logins.
		 */
		void incAttemptsWithPassword() { ++attemptsWithPassword_.value(); }

		// Average accessors

		/**
		 *	Return the failed logins per second average.
		 */
		float fails() const 		{ return fails_.average(); }

		/**
		 *	Return the rate-limited logins per second average.
		 */
		float rateLimited() const 	{ return rateLimited_.average(); }

		/**
		 *	Return the repeated logins (due to already pending login) per
		 *	second average.
		 */
		float pending() const 		{ return pending_.average(); }

		/**
		 *	Return the successful logins per second average.
		 */
		float successes() const 	{ return successes_.average(); }

		/**
		 *	Return the logins per second average.
		 */
		float all() const 			{ return all_.average(); }


		/**
		 *	Return the average time reported for calculating challenge
		 *	results.
		 */
		float challengeCalculationAverage() const
		{ 
			return calculationTime_.average();
		}


		/**
		 *	Return the average time for verifying incoming challenges.
		 */
		float challengeVerificationAverage() const
		{ 
			return verificationTime_.average();
		}


		/**
		*	Returns the failedByIPAddressBan per second average.
		*/
		float failedByIPAddressBan() const { return failedByIPAddressBan_.average(); }

		/**
		*	Returns the attemptsWithPassword per second average.
		*/
		float attemptsWithPassword() const { return attemptsWithPassword_.average(); }

		/**
		 *	This method updates the averages to the accumulated values.
		 */
		void update()
		{
			fails_.sample();
			rateLimited_.sample();
			successes_.sample();
			pending_.sample();
			all_.sample();
			failedByIPAddressBan_.sample();
			attemptsWithPassword_.sample();
		}

	private:
		/// Failed logins.
		AccumulatingEMA< uint32 > fails_;
		/// Rate-limited logins.
		AccumulatingEMA< uint32 > rateLimited_;
		/// Repeated logins that matched a pending login.
		AccumulatingEMA< uint32 > pending_;
		/// Successful logins.
		AccumulatingEMA< uint32 > successes_;
		/// All logins.
		AccumulatingEMA< uint32 > all_;
		/// Failed login attempts because of their IP address banned.
		AccumulatingEMA< uint32 > failedByIPAddressBan_;
		/// Failed basic login attempts  
		/// when passwordless only logins allowed
		AccumulatingEMA< uint32 > attemptsWithPassword_;

		/// Reported client-side calculation time for challenges.
		EMA calculationTime_;
		/// Server-side verification time for challenge responses.
		EMA verificationTime_;

		/// The bias for all the count-based exponential averages.
		static const float COUNT_BIAS;
		/// The bias for the challenge calculation and verification time
		/// averages.
		static const float TIME_BIAS;
	};

	LoginStats loginStats_;
	TimerHandle statsTimer_;

	enum TimeoutType
	{
		TIMEOUT_TICK
	};

	TimerHandle tickTimer_;

	static const uint32 UPDATE_STATS_PERIOD;
	static const uint32 MAX_SANE_CALCULATION_SECONDS = 3600;

};

BW_END_NAMESPACE

#endif // LOGINAPP_HPP
