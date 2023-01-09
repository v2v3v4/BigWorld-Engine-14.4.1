#include "pch.hpp"
#include "reload.hpp"


#if ENABLE_RELOAD_MODEL

BW_BEGIN_NAMESPACE


bool Reloader::s_enable = true;

/*static*/void Reloader::enable( bool enable )
{
	s_enable = enable;
}

/*static*/bool Reloader::enable()
{
	return s_enable;
}
/**
*	This internal function is not thread safe, so the caller need 
*	look after the concurrency.
*/
bool Reloader::findListener( ReloadListener* pListener, 
							Reloader::Listeners::iterator* pItRet /*=NULL*/ )
{
	BW_GUARD;

	Listeners::iterator it = listeners_.begin();
	for (; it != listeners_.end(); ++it)
	{
		if ((*it) == pListener)
		{
			if (pItRet)
			{
				*pItRet = it;
			}
			return true;
		}
	}
	return false;
}


void Reloader::registerListener( ReloadListener* pListener, bool bothDirection /*= true*/ )
{
	BW_GUARD;
	if (!s_enable)
		return;

	{
		SimpleMutexHolder lock( listenerMutex_ );

		if ( !this->findListener( pListener ) )
		{
			listeners_.push_back( pListener );
		}
	}

	if (bothDirection)
	{
		pListener->registerReloader( this );
	}
}


void Reloader::deregisterListener( ReloadListener* pListener, bool bothDirection )
{
	BW_GUARD;
	if (!s_enable)
		return;

	{
		SimpleMutexHolder lock( listenerMutex_ );

		Listeners::iterator itFound;
		if ( this->findListener( pListener, &itFound ) )
		{
			listeners_.erase( itFound );
		}
	}

	if (bothDirection)
	{
		pListener->deregisterReloader( this );
	}

}

/**
*	To notify all my listeners that I am about to reload
*	@param pSourceReloader		if not NULL, then we pass it
*								to the listeners, cause sometimes
*								it's the reloader listened reloader
*								get reloaded. if NULL, we simply pass
*								this to the listeners.
*/
void Reloader::onPreReload( Reloader* pSourceReloader/* = NULL*/ )
{
	BW_GUARD;
	if (!s_enable)
		return;
	if (pSourceReloader == NULL)
	{
		pSourceReloader = this;
	}

	Listeners listenersCopy;

	// the scope of mutex
	{
		SimpleMutexHolder lock( listenerMutex_ );
		listenersCopy = listeners_;
	}

	Listeners::iterator it = listenersCopy.begin();
	for (; it!= listenersCopy.end(); ++it)
	{
		// cause this function can internally remove node
		if (this->findListener( *it ))
		{
			(*it)->onReloaderPreReload( pSourceReloader );
		}
	}
}

/**
*	To notify all my listeners that I have just been reloaded
*	@param pSourceReloader		if not NULL, then we pass it
*								to the listeners, cause sometimes
*								it's the reloader listened reloader
*								get reloaded. if NULL, we simply pass
*								this to the listeners.
*/
void Reloader::onReloaded( Reloader* pSourceReloader/* = NULL*/ )
{
	BW_GUARD;
	if (!s_enable)
		return;
	if (pSourceReloader == NULL)
	{
		pSourceReloader = this;
	}

	Listeners listenersCopy;
	
	// the scope of mutex
	{
		SimpleMutexHolder lock( listenerMutex_ );
		listenersCopy = listeners_;
	}

	Listeners::iterator it = listenersCopy.begin();
	for (; it!= listenersCopy.end(); ++it)
	{
		// cause this function can internally remove node
		if (this->findListener( *it ))
		{
			(*it)->onReloaderReloaded( pSourceReloader );
		}
	}
}


/**
*	Tell the listeners that have been listening to me that I will 
*	die so stop listen to me.
*/
Reloader::~Reloader()
{
	BW_GUARD;
	{
		SimpleMutexHolder lock( listenerMutex_ );

		Listeners::iterator it = listeners_.begin();
		for (; it!= listeners_.end(); ++it)
		{
			(*it)->deregisterReloader( this );
		}
	}
}




/*
*	The following is for class ReloadListener
*/

void ReloadListener::registerReloader( Reloader* pReloader )
{
	BW_GUARD;
	if (!Reloader::enable())
		return;

	for (int i = 0; i < MAX_LISTNED_RELOADER; ++i)
	{
		// Already exist.
		if (listenedReloaders_[i] == pReloader)
		{
			return;
		}
	}

	for (int i = 0; i < MAX_LISTNED_RELOADER; ++i)
	{
		if (listenedReloaders_[i] == NULL)
		{
			listenedReloaders_[i] = pReloader;
			return;
		}
	}

	// Couldn't find a slot, bad, then we need increase MAX_LISTNED_RELOADER
	MF_ASSERT( false );

}


void ReloadListener::deregisterReloader( Reloader* pReloader )
{
	BW_GUARD;
	if (!Reloader::enable())
		return;

	for (int i = 0; i < MAX_LISTNED_RELOADER; ++i)
	{
		if (listenedReloaders_[i] == pReloader)
		{
			listenedReloaders_[i] = NULL;
			return;
		}
	}

}


ReloadListener::ReloadListener()
{
	BW_GUARD;
	for (int i = 0; i < MAX_LISTNED_RELOADER; ++i)
	{
		listenedReloaders_[i] = NULL;
	}
}


/**
*	Tell the loaders that I'v been listening that I will no 
*	longer listen to them.
*/
ReloadListener::~ReloadListener()
{
	BW_GUARD;
	for (int i = 0; i < MAX_LISTNED_RELOADER; ++i)
	{
		if (listenedReloaders_[i] != NULL)
		{
			listenedReloaders_[i]->deregisterListener( this, false );
		}
	}
}

BW_END_NAMESPACE

#endif//ENABLE_RELOAD_MODEL
