#include "pch.hpp"

#include "polymorphic_watcher.hpp"


#if ENABLE_WATCHERS

BW_BEGIN_NAMESPACE


/**
 *	Constructor.
 */
PolymorphicWatcher::PolymorphicWatcher( const char * comment ) :
	Watcher( comment )
{
}


/*
 *	Override from Watcher.
 */
bool PolymorphicWatcher::getAsString( const void * base,
	const char * path,
	BW::string & result,
	BW::string & desc,
	Mode & mode ) const
{
	WatcherPtr watcher = this->getWatcher( base );

	if (watcher == NULL)
	{
		return false;
	}

	return watcher->getAsString(
			this->getProvider( base ),
			path, result, desc, mode );
}


/*
 *	Override from Watcher.
 */
bool PolymorphicWatcher::getAsString( const void * base, const char * path,
	BW::string & result, Mode & mode ) const
{
	WatcherPtr watcher = this->getWatcher( base );

	if (watcher == NULL)
	{
		return false;
	}

	return watcher->getAsString(
			this->getProvider( base ),
			path, result, mode );
}


/*
 *	Override from Watcher.
 */
bool PolymorphicWatcher::setFromString( void * base,
	const char * path,
	const char * valueStr )
{
	WatcherPtr watcher = this->getWatcher( base );

	if (watcher == NULL)
	{
		return false;
	}

	return watcher->setFromString(
			this->getProvider( base ),
			path, valueStr );
}


/*
 *	Override from Watcher.
 */
bool PolymorphicWatcher::setFromStream( void * base,
	const char * path,
	WatcherPathRequestV2 & pathRequest )
{
	WatcherPtr watcher = this->getWatcher( base );

	if (watcher == NULL)
	{
		return false;
	}

	return watcher->setFromStream(
			this->getProvider( base ),
			path, pathRequest );
}


/*
 *	Override from Watcher.
 */
bool PolymorphicWatcher::visitChildren( const void * base, const char *path,
		WatcherPathRequest & pathRequest )
{
	WatcherPtr watcher = this->getWatcher( base );

	if (watcher == NULL)
	{
		return false;
	}

	return watcher->visitChildren(
			this->getProvider( base ),
			path, pathRequest );
}


/*
 *	Override from Watcher.
 */
bool PolymorphicWatcher::getAsStream( const void * base,
	const char * path, WatcherPathRequestV2 & pathRequest ) const
{
	WatcherPtr watcher = this->getWatcher( base );

	if (watcher == NULL)
	{
		return false;
	}

	return watcher->getAsStream( this->getProvider( base ),	path, pathRequest );
}


/*
 *	Override from Watcher.
 */
WatcherPtr PolymorphicWatcher::getWatcher( const void * base ) const
{
	WatcherProvider * provider = this->getProvider( base );

	if (provider == NULL)
	{
		return NULL;
	}

	return provider->getWatcher();
}


/**
 *	This method dereferences the base pointer to be a WatcherProvider *.
 */
WatcherProvider * PolymorphicWatcher::getProvider( const void * base ) const
{
	return *(WatcherProvider **)base;
}


BW_END_NAMESPACE

#endif // ENABLE_WATCHERS


// polymorphic_watcher.cpp
