#ifndef CUCKOO_CYCLE_LOGIN_CHALLENGE_FACTORY_HPP
#define CUCKOO_CYCLE_LOGIN_CHALLENGE_FACTORY_HPP


#include "login_challenge_factory.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class creates instances of cuckoo cycle proof-of-work login
 *	challenges.
 */
class CuckooCycleLoginChallengeFactory : public LoginChallengeFactory
{
public:
	CuckooCycleLoginChallengeFactory();

	// Overrides from LoginChallengeFactory.
	bool configure( const LoginChallengeConfig & config ) /* override */;
	LoginChallengePtr create() /* override */;

	void easiness( double value ) 
	{ 
		easiness_ = std::max( 0.0, std::min( 100.0, value ) );
	}
	double easiness() const { return easiness_; }

protected:

#if ENABLE_WATCHERS
	// Overrides from LoginChallengeFactory.
	WatcherPtr pWatcher() /* override */;
#endif // ENABLE_WATCHERS

private:
	double easiness_;

	static const double DEFAULT_EASINESS;
};


BW_END_NAMESPACE


#endif // CUCKOO_CYCLE_LOGIN_CHALLENGE_FACTORY_HPP
