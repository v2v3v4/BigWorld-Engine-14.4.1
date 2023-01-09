#include "script/first_include.hpp"

#include "relogon_attempt_handler.hpp"

#include "dbapp.hpp"

#include "db_storage/db_entitydefs.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "network/event_dispatcher.hpp"
#include "network/nub_exception.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
RelogonAttemptHandler::RelogonAttemptHandler( EntityTypeID entityTypeID,
		DatabaseID dbID,
		const Mercury::Address & replyAddr,
		Mercury::ReplyID replyID,
		LogOnParamsPtr pParams,
		const Mercury::Address & addrForProxy,
		const BW::string & dataForClient ) :
	hasAborted_( false ),
	ekey_( entityTypeID, dbID ),
	replyAddr_( replyAddr ),
	replyID_( replyID ),
	pParams_( pParams ),
	addrForProxy_( addrForProxy ),
	replyBundle_(),
	waitForDestroyTimer_(),
	creationTime_( timestamp() ),
	dataForClient_( dataForClient )
{
	DBApp & dbApp = DBApp::instance();

	MF_ASSERT( dbApp.getEntityDefs().isValidEntityType( entityTypeID ) );

	INFO_MSG( "RelogonAttemptHandler: Starting relogon attempt for "
			"user: %s, client: %s, DBID: %" FMT_DBID ", type: %d (%s)\n",
		pParams->username().c_str(),
		addrForProxy.c_str(),
		dbID, 
		entityTypeID,
		dbApp.getEntityDefs().
			getEntityDescription( entityTypeID ).name().c_str() );

	dbApp.onStartRelogonAttempt( entityTypeID, dbID, this );
}


/**
 *	Destructor.
 */
RelogonAttemptHandler::~RelogonAttemptHandler()
{
	waitForDestroyTimer_.cancel();

	if (!hasAborted_)
	{
		this->abort();
	}
}


/*
 *	Mercury::ReplyMessageHandler override.
 */
void RelogonAttemptHandler::handleMessage(
	const Mercury::Address & source,
	Mercury::UnpackedMessageHeader & header,
	BinaryIStream & data,
	void * arg )
{
	uint8 result;
	data >> result;

	if (hasAborted_)
	{
		DEBUG_MSG( "RelogonAttemptHandler: DBID %" FMT_DBID ": "
				"Ignoring BaseApp reply, re-logon attempt has been aborted.\n",
			ekey_.dbID );

		// Delete ourselves as we have been aborted.
		delete this;
		return;
	}

	switch (result)
	{
	case BaseAppIntInterface::LOG_ON_ATTEMPT_TOOK_CONTROL:
	{
		INFO_MSG( "RelogonAttemptHandler: DBID %" FMT_DBID ": "
				"It's taken over.\n",
			ekey_.dbID );
		Mercury::Address proxyAddr;
		data >> proxyAddr;

		EntityMailBoxRef baseRef;
		data >> baseRef;

		replyBundle_.startReply( replyID_ );

		// Assume success.
		replyBundle_ << (uint8)LogOnStatus::LOGGED_ON;
		replyBundle_ << proxyAddr;
		replyBundle_.transfer( data, data.remainingLength() );
		replyBundle_ << dataForClient_;

		DBApp::instance().interface().sendOnExistingChannel( replyAddr_,
				replyBundle_ );

		delete this;

		break;
	}

	case BaseAppIntInterface::LOG_ON_ATTEMPT_REJECTED:
	{
		INFO_MSG( "RelogonAttemptHandler: DBID %" FMT_DBID ": "
				"Re-login not allowed for %s.\n",
			ekey_.dbID, pParams_->username().c_str() );

		DBApp::instance().sendFailure( replyID_, replyAddr_,
			LogOnStatus::LOGIN_REJECTED_ALREADY_LOGGED_IN,
			"Relogin denied." );

		delete this;

		break;
	}

	case BaseAppIntInterface::LOG_ON_ATTEMPT_WAIT_FOR_DESTROY:
	{
		INFO_MSG( "RelogonAttemptHandler: DBID %" FMT_DBID ": "
				"Waiting for existing entity to be destroyed.\n",
			ekey_.dbID );

		// Wait 5 seconds for an entity to be destroyed before
		// giving up.
		const int waitForDestroyTimeout = 5000000;
		waitForDestroyTimer_ =
			DBApp::instance().mainDispatcher().addTimer(
				waitForDestroyTimeout, this, NULL, "RelogonDestroy" );

		// Don't delete ourselves just yet, wait for the timeout
		// to occur.
		break;
	}

	default:
		CRITICAL_MSG( "RelogonAttemptHandler: DBID %" FMT_DBID ": "
				"Invalid result %d\n",
			ekey_.dbID,
			int(result) );
		delete this;
		break;
	}
}


/*
 *	Mercury::ReplyMessageHandler override.
 */
void RelogonAttemptHandler::handleException(
	const Mercury::NubException & exception,
	void * arg )
{
	ERROR_MSG( "RelogonAttemptHandler::handleException: DBID %" FMT_DBID ": "
			"Unexpected channel exception.\n",
		ekey_.dbID );
	this->terminateRelogonAttempt(
			Mercury::reasonToString( exception.reason() ));
}


void RelogonAttemptHandler::handleShuttingDown(
	const Mercury::NubException & exception, void * arg )
{
	INFO_MSG( "RelogonAttemptHandler::handleShuttingDown: DBID %" FMT_DBID ": "
			"Ignoring\n",
		ekey_.dbID );
	delete this;
}


/*
 *	TimerHandler override.
 */
void RelogonAttemptHandler::handleTimeout( TimerHandle handle, void * pUser )
{
	// If we timeout after receiving a LOG_ON_ATTEMPT_WAIT_FOR_DESTROY
	// then it's time to give up and destroy ourselves.
	this->terminateRelogonAttempt(
			"Timeout waiting for previous login to disconnect." );
}


/**
 *	This method sends a notification to a client connection that a re-login
 *	attempt has failed.
 */
void RelogonAttemptHandler::terminateRelogonAttempt(
	const char * clientMessage )
{
	if (!hasAborted_)
	{
		this->abort();
		ERROR_MSG( "RelogonAttemptHandler: %s.\n", clientMessage );

		DBApp::instance().sendFailure( replyID_, replyAddr_,
				LogOnStatus::LOGIN_REJECTED_BASEAPP_TIMEOUT,
				clientMessage );
	}

	delete this;
}


/**
 *	This method is called when the entity that we're trying to re-logon
 *	to suddenly logs off.
 */
void RelogonAttemptHandler::onEntityLogOff()
{
	if (hasAborted_)
	{
		return;
	}

	this->abort();

	// Log on normally
	DBApp::instance().logOn( replyAddr_, replyID_, pParams_, addrForProxy_ );
}


/**
 *	This method flags this handler as aborted. It should not do any more
 *	processing but may still need to wait for callbacks.
 */
void RelogonAttemptHandler::abort()
{
	hasAborted_ = true;
	DBApp::instance().onCompleteRelogonAttempt(
			ekey_.typeID, ekey_.dbID );
}

BW_END_NAMESPACE

// relogon_attempt_handler.cpp
