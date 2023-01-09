#ifndef CLIENT_LOGIN_REQUEST_HPP
#define CLIENT_LOGIN_REQUEST_HPP

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stdmf.hpp"

#include "connection/login_reply_record.hpp"


BW_BEGIN_NAMESPACE

class LoginChallenge;
typedef SmartPointer< LoginChallenge > LoginChallengePtr;
class LogOnParams;
typedef SmartPointer< LogOnParams > LogOnParamsPtr;

namespace Mercury
{
	class Channel;
} // end namespace Mercury


/**
 *	This class holds state about a client login request.
 */
class ClientLoginRequest
{
public:
	ClientLoginRequest();
	~ClientLoginRequest();

	void setLoginChallenge( const BW::string & challengeType,
			LoginChallengePtr pLoginChallenge );
	const BW::string & challengeType() const { return challengeType_; }
	LoginChallenge * pLoginChallenge() { return pLoginChallenge_.get(); }

	bool didFailChallenge() const { return didFailChallenge_; }
	void didFailChallenge( bool value ) { didFailChallenge_ = value; }

	void clearChallenge();


	void setData( const LoginReplyRecord & record,
			const BW::string & serverMsg );

	bool isTooOld() const;

	bool hasPendingChallenge() const { return pLoginChallenge_.hasObject(); }
	bool isPendingAuthentication() const;

	void pParams( LogOnParamsPtr pParams ) { pParams_ = pParams; }
	LogOnParamsPtr pParams() const { return pParams_; }

	void writeLoginChallengeToStream( BinaryOStream & stream ) const;

	void writeSuccessResultToStream( BinaryOStream & stream ) const
	{
		stream << replyRecord_ << serverMsg_;
	}
	Mercury::Channel * pChannel() { return pChannel_; }
	void pChannel( Mercury::Channel * pChannel ) { pChannel_ = pChannel; }

	/// This method re-initialises the cache object to indicate that it is
	/// pending.
	void reset() { creationTime_ = 0; }

private:
	uint64 				creationTime_;
	LogOnParamsPtr 		pParams_;
	Mercury::Channel * 	pChannel_;
	BW::string 			challengeType_;
	bool				didFailChallenge_;
	LoginChallengePtr 	pLoginChallenge_;
	LoginReplyRecord 	replyRecord_;
	BW::string 			serverMsg_;
};


BW_END_NAMESPACE
#endif // CLIENT_LOGIN_REQUEST_HPP

