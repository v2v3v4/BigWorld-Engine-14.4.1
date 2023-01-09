#include "pch.hpp"

#include "login_challenge_task.hpp"

#include "login_handler.hpp"
#include "login_challenge.hpp"
#include "loginapp_login_request_protocol.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
LoginChallengeTask::LoginChallengeTask( LoginHandler & handler,
			LoginChallengePtr pChallenge ) :
		BackgroundTask( "LoginChallenge" ),
		pHandler_( &handler ),
		pChallenge_( pChallenge ),
		calculationDuration_( 0.f ),
		response_(),
		isFinished_( false )
{}


/**
 *	Destructor.
 */
LoginChallengeTask::~LoginChallengeTask()
{
	response_.finish();
}


/**
 *	This method performs the challenge in the calling thread.
 */
void LoginChallengeTask::perform()
{
	if (isFinished_)
	{
		ERROR_MSG( "LoginChallengeTask::perform: Already finished\n" );
		return;
	}

	TimeStamp start( BW::timestamp() );
	pChallenge_->writeResponseToStream( response_ );
	calculationDuration_ = static_cast< float >( start.ageInSeconds() );
	isFinished_ = true;
}


/* Override from BackgoundTask. */
void LoginChallengeTask::doBackgroundTask( TaskManager & mgr ) /* override */
{
	if (!pHandler_)
	{
		return;
	}

	this->perform();
	mgr.addMainThreadTask( this );
}


/* Override from BackgroundTask. */
void LoginChallengeTask::doMainThreadTask( TaskManager & mgr ) /* override */
{
	if (pHandler_)
	{
		pHandler_->onLoginChallengeCompleted();
		this->disassociate();
	}
}


BW_END_NAMESPACE


// login_challenge_task.cpp
