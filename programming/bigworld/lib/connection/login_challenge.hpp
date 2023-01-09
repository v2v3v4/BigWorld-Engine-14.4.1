#ifndef LOGIN_CHALLENGE_HPP
#define LOGIN_CHALLENGE_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE


class BinaryIStream;
class BinaryOStream;

class LoginChallenge;
typedef SmartPointer< LoginChallenge > LoginChallengePtr;


/**
 *	This class represents a login challenge given to a client.
 *
 *	The LoginApp is configured to send a certain type of challenge to the 
 *	client, and the client instantiates a LoginChallenge based on the 
 *	type specified by the server. The server will also send down any data for
 *	the challenge, which can be read from the given stream. The client should
 *	perform the challenge and send back the result to the server.
 */
class LoginChallenge : public SafeReferenceCount
{
public:
	/**
	 *	Destructor.
	 */
	virtual ~LoginChallenge()
	{
	}


	/**
	 *	This method is used to send the login challenge data to the client.
	 */
	virtual bool writeChallengeToStream( BinaryOStream & data ) = 0;

	/**
	 *	This method is used to read the login challenge data from the server.
	 */
	virtual bool readChallengeFromStream( BinaryIStream & data ) = 0;

	/**
	 *	This method is used to verify the login challenge response from the
	 *	client.
	 */
	virtual bool writeResponseToStream( BinaryOStream & data ) = 0;

	/**
	 *	This method is used to verify the login challenge response from the
	 *	client.
	 */
	virtual bool readResponseFromStream( BinaryIStream & data ) = 0;

protected:
	/**
	 *	Constructor.
	 */
	LoginChallenge() :
		SafeReferenceCount()
	{}

};


BW_END_NAMESPACE

#endif // LOGIN_CHALLENGE_HPP

