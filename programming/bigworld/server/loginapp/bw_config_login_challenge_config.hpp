#ifndef BW_CONFIG_LOGIN_CHALLENGE_CONFIG_HPP
#define BW_CONFIG_LOGIN_CHALLENGE_CONFIG_HPP

#include "connection/login_challenge_factory.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class wraps a resmgr-dependent BWConfig for use with the
 *	LoginChallengeConfig class in the connection library.
 */
class BWConfigLoginChallengeConfig : public LoginChallengeConfig
{
public:
	static LoginChallengeConfigPtr root();

	virtual LoginChallengeConfigPtr getChild( 
		const BW::string & childName ) const /* override */;

	const BW::string getString( const BW::string & path,
		BW::string defaultValue = BW::string() ) const /* override */;

	long getLong( const BW::string & path,
		long defaultValue = 0 ) const /* override */;

	double getDouble( const BW::string & path,
		double defaultValue = 0.0 ) const /* override */;

private:
	BWConfigLoginChallengeConfig( const BW::string & path );

	BW::string path_;
};


BW_END_NAMESPACE


#endif // BW_CONFIG_LOGIN_CHALLENGE_CONFIG_HPP
