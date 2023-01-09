#ifndef POLYMORPHIC_WATCHER_HPP
#define POLYMORPHIC_WATCHER_HPP

#include "config.hpp"

#if ENABLE_WATCHERS

#include "watcher.hpp"

BW_BEGIN_NAMESPACE

class WatcherProvider
{
public:
	virtual WatcherPtr getWatcher() = 0;
};


/**
 *	This watcher supports looking at an object in a polymorphic way. That is,
 *	there may be different watchers associated with the different instances
 *	being watched.
 *
 *	The watched object must implement WatcherProvider.
 *
 *	Note: Currently there is an assumption that the pointer to the
 *	WatcherProvider is the same as the pointer to the watched instance. If this
 *	is offset, for example with multiple inheritance, offset support will need
 *	to be implemented.
 *
 *	@ingroup WatcherModule
 */
class PolymorphicWatcher : public Watcher
{
public:
	/// @name Construction/Destruction
	//@{
	/// Constructor.
	CSTDMF_DLL PolymorphicWatcher( const char * comment = "" );
	//@}

	virtual bool getAsString( const void * base,
		const char * path,
		BW::string & result,
		BW::string & desc,
		Mode & mode ) const;

	virtual bool getAsString( const void * base, const char * path,
		BW::string & result, Mode & mode ) const;

	virtual bool setFromString( void * base,
		const char * path,
		const char * valueStr );

	virtual bool setFromStream( void * base,
		const char * path,
		WatcherPathRequestV2 & pathRequest );

	virtual bool visitChildren( const void * base, const char *path,
			WatcherPathRequest & pathRequest );

	virtual bool getAsStream( const void * base,
		const char * path, WatcherPathRequestV2 & pathRequest ) const;

private:
	WatcherPtr getWatcher( const void * base ) const;
	WatcherProvider * getProvider( const void * base ) const;
};

BW_END_NAMESPACE

#endif // ENABLE_WATCHERS

#endif // POLYMORPHIC_WATCHER_HPP
