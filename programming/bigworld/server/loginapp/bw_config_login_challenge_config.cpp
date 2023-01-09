#include "bw_config_login_challenge_config.hpp"

#include "server/bwconfig.hpp"


BW_BEGIN_NAMESPACE

namespace // (anonymous)
{
const BW::string CHALLENGE_ROOT_PATH = "loginApp/loginChallenge";
}


/**
 *	Constructor.
 */
BWConfigLoginChallengeConfig::BWConfigLoginChallengeConfig(
		const BW::string & path ) :
	LoginChallengeConfig(),
	path_( path )
{}


/**
 *	This method returns the login challenge configuration root.
 */
/* static */ LoginChallengeConfigPtr BWConfigLoginChallengeConfig::root()
{
	static LoginChallengeConfigPtr pInstance =
		new BWConfigLoginChallengeConfig( CHALLENGE_ROOT_PATH );

	return pInstance;
}


/*
 *	Override from LoginChallengeConfig.
 */
LoginChallengeConfigPtr BWConfigLoginChallengeConfig::getChild(
		const BW::string & childName ) const /* override */
{
	return new BWConfigLoginChallengeConfig( path_ + "/" + childName );
}


/* Override from LoginChallengeConfig. */
const BW::string BWConfigLoginChallengeConfig::getString(
		const BW::string & path,
		BW::string defaultValue /* = BW::string() */ ) const /* override */
{
	return BWConfig::get( (path_ + "/" + path).c_str(), defaultValue );
}


/* Override from LoginChallengeConfig. */
long BWConfigLoginChallengeConfig::getLong( const BW::string & path,
		long defaultValue /* = 0 */ ) const /* override */
{
	return BWConfig::get( (path_ + "/" + path).c_str(), defaultValue );
}


/* Override from LoginChallengeConfig. */
double BWConfigLoginChallengeConfig::getDouble( const BW::string & path,
		double defaultValue /* = 0.0 */ ) const /* override */
{
	return BWConfig::get( (path_ + "/" + path).c_str(), defaultValue );
}


BW_END_NAMESPACE

// bw_config_login_challenge_config.cpp
