#include "pch.hpp"

#include "login_challenge.hpp"
#include "login_challenge_factory.hpp"

#include "cuckoo_cycle_login_challenge_factory.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE


// -----------------------------------------------------------------------------
// Section: DelayLoginChallengeFactory and DelayLoginChallenge
// -----------------------------------------------------------------------------

/**
 *	This class implements an example login challenge where the client-side
 *	simply waits for a time period. Note that this is not enforced on the
 *	server, this challenge is not intended for production use.
 */
class DelayLoginChallenge : public LoginChallenge
{
public:
	/**
	 *	Constructor.
	 */
	DelayLoginChallenge() :
		LoginChallenge(),
		duration_( 0.f )
	{}


	/**
	 *	This method sets a delay for this challenge. Call this before
	 *	writeChallengeToStream.
	 */
	void setDuration( float duration )
	{
		duration_ = duration;
	}
	

	/* Override from LoginChallenge. */
	bool writeChallengeToStream( BinaryOStream & data ) /* override */
	{
		data << duration_;
		return true;
	}


	/* Override from LoginChallenge. */
	bool readChallengeFromStream( BinaryIStream & data ) /* override */
	{
		data >> duration_;

		return true;
	}


	/* Override from LoginChallenge. */
	bool writeResponseToStream( BinaryOStream & data ) /* override */
	{
#if defined( _WIN32 )
		Sleep( int( duration_ * 1000 ) );
#else
		usleep( int( duration_ * 1000000 ) );
#endif
		data << duration_;

		return true;
	}


	/* Override from LoginChallenge. */
	bool readResponseFromStream( BinaryIStream & data ) /* override */
	{
		float duration = 0.f;
		data >> duration;
		return isEqual( duration, duration_ );
	}


private:
	float duration_;
};


/**
 *	Factory class for creating DelayLoginChallenges.
 */
class DelayLoginChallengeFactory : public LoginChallengeFactory
{
	static const float DEFAULT_DURATION;

public:
	/**
	 *	Constructor.
	 */
	DelayLoginChallengeFactory() :
		LoginChallengeFactory(),
		duration_( DEFAULT_DURATION )
	{
	}


	/* Override from LoginChallengeFactory. */
	LoginChallengePtr create()  /* override */
	{
		DelayLoginChallenge * pChallenge = new DelayLoginChallenge();
		pChallenge->setDuration( duration_ );

		return pChallenge;
	}


	/* Override from LoginChallengeFactory. */
	bool configure( const LoginChallengeConfig & config ) /* override */
	{
		duration_ = static_cast< float >(
			config.getDouble( "duration", duration_ ) );

		if (duration_ <= 0.f)
		{
			ERROR_MSG( "DelayLoginChallengeFactory::configure: "
					"Invalid duration configured\n" );
			return false;
		}

		INFO_MSG( "DelayLoginChallengeFactory::configure: Duration %.03fs\n",
			duration_ );

		return true;
	}

#if ENABLE_WATCHERS
	/* Override from LoginChallengeFactory. */
	WatcherPtr pWatcher() /* override */
	{
		WatcherPtr pWatcher = new DirectoryWatcher();
		DelayLoginChallengeFactory * pNull = NULL;

		pWatcher->addChild( "duration", 
			makeNonRefWatcher( *pNull, 
				&DelayLoginChallengeFactory::duration,
				&DelayLoginChallengeFactory::duration ) );

		return pWatcher;
	}
#endif // ENABLE_WATCHERS

	void duration( float value ) { duration_ = value; }
	float duration() const { return duration_; }

private:
	float duration_;
};


const float DelayLoginChallengeFactory::DEFAULT_DURATION = 1.f;


// -----------------------------------------------------------------------------
// Section: AlwaysFailLoginChallenge
// -----------------------------------------------------------------------------


/**
 *	This challenge class always fails.
 */
class AlwaysFailLoginChallenge : public LoginChallenge
{
public:
	/**
	 *	Constructor.
	 */
	AlwaysFailLoginChallenge() :
		LoginChallenge()
	{}


	/* Override from LoginChallenge. */
	bool writeChallengeToStream( BinaryOStream & data ) /* override */
	{
		return true;
	}


	/* Override from LoginChallenge. */
	bool readChallengeFromStream( BinaryIStream & data ) /* override */
	{
		return true;
	}


	/* Override from LoginChallenge. */
	bool writeResponseToStream( BinaryOStream & data ) /* override */
	{
		return true;
	}


	/* Override from LoginChallenge. */
	bool readResponseFromStream( BinaryIStream & data ) /* override */
	{
		return false;
	}
	
};


/**
 *	This factory class creates instances of AlwaysFailLoginChallenge instances.
 */
class AlwaysFailLoginChallengeFactory : public LoginChallengeFactory
{
public:
	/**
	 *	Constructor.
	 */
	AlwaysFailLoginChallengeFactory() :
		LoginChallengeFactory()
	{
	}


	/* Override from LoginChallengeFactory. */
	LoginChallengePtr create()  /* override */
	{
		return new AlwaysFailLoginChallenge();
	}
};




// -----------------------------------------------------------------------------
// Section: LoginChallengeFactories
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 *
 *	This method initialises the available factories.
 */
LoginChallengeFactories::LoginChallengeFactories()
{
	this->registerDefaultFactories();
}


/**
 *	Destructor.
 */
LoginChallengeFactories::~LoginChallengeFactories()
{
}


/**
 *	This method registers the default set of factories.
 */
void LoginChallengeFactories::registerDefaultFactories()
{
	// This is done explicitly rather than using e.g. static initialisers and
	// tokens so that the available challenge types for a particular version
	// can be all in one place.

	this->registerFactory( "delay", new DelayLoginChallengeFactory );
	this->registerFactory( "fail", new AlwaysFailLoginChallengeFactory );
	this->registerFactory( "cuckoo_cycle",
		new CuckooCycleLoginChallengeFactory );
}


/**
 *	This method registers a factory under the given name.
 *
 *	@param name 	The challenge type name.
 *	@param factory 	The factory object.
 */
void LoginChallengeFactories::registerFactory( const BW::string & name,
		LoginChallengeFactory * pFactory )
{
	map_[name] = pFactory;
}


/**
 *	This method removes the registry entry for a given name.
 */
void LoginChallengeFactories::deregisterFactory( const BW::string & name )
{
	map_.erase( name );
}


/**
 *	This method returns the factory registered for the given name.
 *
 *	@param name 	The name of the challenge type.
 *	@return 		The factory registered for that name, or NULL if no such
 *					factory is registered.
 */
LoginChallengeFactoryPtr LoginChallengeFactories::getFactory(
		const BW::string & name )
{
	FactoryMap::const_iterator iter = map_.find( name );
	if (iter == map_.end())
	{
		return LoginChallengeFactoryPtr();
	}

	return iter->second;
}


/**
 *	This method returns a challenge created from the factory registered under
 *	the given name.
 *
 *	@param name 	The name of the challenge type.
 */
LoginChallengePtr LoginChallengeFactories::createChallenge( 
		const BW::string & name )
{
	FactoryMap::const_iterator iter = map_.find( name );
	if (iter == map_.end())
	{
		ERROR_MSG( "LoginChallengeFactories::createChallenge: "
				"No such factory registered for \"%s\"\n",
			name.c_str() );

		return LoginChallengePtr();
	}

	MF_ASSERT( iter->second.hasObject() );
	return iter->second->create();
}


/**
 *	This method configures all registered factories from the given
 *	configuration object.
 */
bool LoginChallengeFactories::configureFactories(
		const LoginChallengeConfig & config )
{
	bool success = true;

	FactoryMap::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		LoginChallengeFactoryPtr pFactory = iter->second;
		FactoryMap::iterator current = iter;
		++iter;

		LoginChallengeConfigPtr pConfig = config.getChild( current->first );
		
		if (pConfig.hasObject() && !pFactory->configure( *pConfig ))
		{
			ERROR_MSG( "LoginChallengeFactories::configureFactories: "
					"Failed to configure %s challenge type\n",
				current->first.c_str() );
			map_.erase( current );
			success = false;
		}
	}

	return success;
}


#if ENABLE_WATCHERS
/**
 *	This method adds watchers for each of the registered factories to the given
 *	Watcher root.
 */
void LoginChallengeFactories::addWatchers( WatcherPtr pWatcherRoot )
{
	FactoryMap::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		WatcherPtr pWatcher = iter->second->pWatcher();
		if (!pWatcher.hasObject())
		{
			// Make it empty directory. This is so that we can list the
			// supported login challenge types by looking at the contents of
			// the root watcher.
			pWatcher = new DirectoryWatcher();
		}

		pWatcherRoot->addChild( iter->first.c_str(), pWatcher,
			(void*)iter->second.get() );

		++iter;
	}
}
#endif // ENABLE_WATCHERS


BW_END_NAMESPACE

// login_challenge_factory.cpp
