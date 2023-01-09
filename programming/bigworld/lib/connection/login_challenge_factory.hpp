#ifndef LOGIN_CHALLENGE_FACTORY_HPP
#define LOGIN_CHALLENGE_FACTORY_HPP

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_safe_allocatable.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE


class LoginChallenge;
typedef SmartPointer< LoginChallenge > LoginChallengePtr;
class LoginChallengeConfig;
typedef SmartPointer< LoginChallengeConfig > LoginChallengeConfigPtr;
class LoginChallengeFactory;
typedef SmartPointer< LoginChallengeFactory > LoginChallengeFactoryPtr;

class Watcher;
typedef SmartPointer< Watcher > WatcherPtr;


/**
 *	This class is used to configure the LoginChallengeFactory instances on the
 *	server side, supplying simple configuration data used for the creating
 *	challenge instances.
 *
 *	This interface means that libresmgr doesn't need to be a dependency of
 *	libconnection.
 */
class LoginChallengeConfig : public SafeReferenceCount
{
public:
	virtual LoginChallengeConfigPtr getChild(
		const BW::string & childName ) const = 0;

	virtual const BW::string getString( const BW::string & path,
		BW::string defaultValue = BW::string() ) const = 0;

	virtual long getLong( const BW::string & path,
		long defaultValue = 0 ) const = 0;

	virtual double getDouble( const BW::string & path,
		double defaultValue = 0.0 ) const = 0;

protected:

	/**
	 *	Constructor.
	 */
	LoginChallengeConfig() :
			SafeReferenceCount()
	{}


	/** Destructor.*/
	virtual ~LoginChallengeConfig()
	{}
};


/**
 *	This class is the base class for LoginChallenge factory instances.
 *
 *	Each factory instance typically registers itself into the static collection
 *	of factories under a name string. This name string is sent over the wire
 *	to clients to identify the challenge type.
 */
class LoginChallengeFactory : public SafeReferenceCount
{
public:

	/** Destructor. */
	virtual ~LoginChallengeFactory()
	{}


	/**
	 *	This method creates a new login challenge.
	 */
	virtual LoginChallengePtr create() = 0;


	/**
	 *	This method is used to configure this object using a configuration
	 *	object.
	 */
	virtual bool configure( const LoginChallengeConfig & config )
	{
		return true;
	}


#if ENABLE_WATCHERS
	/**
	 *	This method creates a directory watcher for this factory. The watcher
	 *	is only valid for the duration of the lifetime of this factory
	 *	instance.
	 */
	virtual WatcherPtr pWatcher() 
	{
		return WatcherPtr( NULL );
	}
#endif // ENABLE_WATCHERS


protected:


	/**
	 *	Constructor.
	 */
	LoginChallengeFactory() :
		SafeReferenceCount()
	{}
};


/**
 *	This class initialises all available LoginChallengeFactory instances on
 *	construction, and makes sure they are properly disposed of on destruction.
 */
class LoginChallengeFactories : public SafeAllocatable
{
public:
	BWENTITY_API LoginChallengeFactories();
	BWENTITY_API ~LoginChallengeFactories();

	void registerDefaultFactories();

	void registerFactory( const BW::string & name,
		LoginChallengeFactory * pFactory );
	void deregisterFactory( const BW::string & name );

	LoginChallengePtr createChallenge( const BW::string & name );
	LoginChallengeFactoryPtr getFactory( const BW::string & name );
	bool configureFactories( const LoginChallengeConfig & config );

#if ENABLE_WATCHERS
	void addWatchers( WatcherPtr pWatcherRoot );
#endif // ENABLE_WATCHERS

private:

	typedef BW::map< BW::string, LoginChallengeFactoryPtr > FactoryMap;
	FactoryMap map_;
};


BW_END_NAMESPACE


#endif // LOGIN_CHALLENGE_FACTORY_HPP
