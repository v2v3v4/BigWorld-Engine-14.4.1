#include "client_login_request.hpp"

#include "loginapp_config.hpp"

#include "connection/login_challenge.hpp"
#include "connection/log_on_params.hpp"

#include "cstdmf/timestamp.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
ClientLoginRequest::ClientLoginRequest() :
		// We set creationTime_ to 0 to indicate that the login is pending.
		creationTime_( 0 ),
		pParams_( NULL ),
		pChannel_( NULL ),
		challengeType_(),
		didFailChallenge_( false ),
		pLoginChallenge_( NULL ),
		replyRecord_()
{}


/**
 *	Destructor.
 */
ClientLoginRequest::~ClientLoginRequest()
{
}


/**
 *	This method sets the login challenge issued for this client.
 */
void ClientLoginRequest::setLoginChallenge( const BW::string & challengeType,
		LoginChallengePtr pLoginChallenge )
{
	challengeType_ = challengeType;
	pLoginChallenge_ = pLoginChallenge;

	MemoryOStream data;
	pLoginChallenge_->writeChallengeToStream( data );
	didFailChallenge_ = false;
}


/**
 *	This method clears the challenge for this login request. This is usually
 *	because it was completed and verified.
 */
void ClientLoginRequest::clearChallenge()
{
	challengeType_.clear();
	pLoginChallenge_ = NULL;
}


/**
 *  This method returns true if this login is pending, i.e. we are waiting on
 *  the DBApp to tell us whether or not the login is successful.
 */
bool ClientLoginRequest::isPendingAuthentication() const
{
	return !pLoginChallenge_ && pParams_ && (creationTime_ == 0);
}


/**
 *	This method returns whether or not this cache is too old to use.
 */
bool ClientLoginRequest::isTooOld() const
{
	const uint64 MAX_LOGIN_DELAY = LoginAppConfig::maxLoginDelayInStamps();

	return !this->isPendingAuthentication() &&
		(timestamp() - creationTime_ > MAX_LOGIN_DELAY);
}


/**
 *	This method writes the login challenge for this request to the given stream.
 */
void ClientLoginRequest::writeLoginChallengeToStream(
		BinaryOStream & stream ) const
{
	MF_ASSERT( pLoginChallenge_ );
	stream << challengeType_;
	MF_VERIFY( pLoginChallenge_->writeChallengeToStream( stream ) );
}


/**
 *  This method sets the data of this cached object, and is called when the
 *	DBApp replies.
 */
void ClientLoginRequest::setData( const LoginReplyRecord & record,
		const BW::string & serverMsg )
{
	replyRecord_ = record;
	serverMsg_ = serverMsg;
	creationTime_ = timestamp();
}


BW_END_NAMESPACE


// client_login_request.cpp
