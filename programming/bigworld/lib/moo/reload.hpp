#ifndef RELOAD_HPP
#define RELOAD_HPP

#include "cstdmf/config.hpp"


BW_BEGIN_NAMESPACE

class ReloadListener;

#if ENABLE_RELOAD_MODEL


/**
 *	This class indicates it potentially get reloaded(visual/model/primtive)
 */
class Reloader
{
	static bool s_enable;
	typedef BW::vector<ReloadListener*> Listeners;

	SimpleMutex listenerMutex_;
	Listeners listeners_;

	bool findListener( ReloadListener* pListener, Listeners::iterator* pItRet = NULL );

public:
	void onReloaded( Reloader* pSourceReloader = NULL );
	void onPreReload( Reloader* pSourceReloader = NULL );
	void registerListener( ReloadListener* pListener, bool bothDirection = true );
	void deregisterListener( ReloadListener* pListener, bool bothDirection );
	
	static void enable( bool enable );
	static bool enable();

	~Reloader();
	friend class ReloadListener;

};

/**
 *	This class listens to the Reloader(s) and if the reloader(s)
 *	are reloaded it will reset up it's info that has been
 *	pulled out from the Reloader(s) (ie. action, dye, pyModel)
 */
class ReloadListener
{
	static const int MAX_LISTNED_RELOADER  = 20;
	Reloader* listenedReloaders_[MAX_LISTNED_RELOADER];

	void registerReloader( Reloader* pReloader );
	void deregisterReloader( Reloader* pReloader );
public:

	virtual void onReloaderReloaded( Reloader* pReloader) = 0;

	// this is happened right before the reload happened, override it if it's needed.
	virtual void onReloaderPreReload( Reloader* pReloader){}

	ReloadListener();
	~ReloadListener();
	friend class Reloader;
};
#else
class Reloader
{
public:
	void onReloaded( Reloader* pSourceReloader = NULL ){}
	void onPreReload( Reloader* pSourceReloader = NULL ){}
	void registerListener( ReloadListener* pListener, bool bothDirection = true ){}
	void deregisterListener( ReloadListener* pListener, bool bothDirection ){}

};
class ReloadListener
{
};


#endif//ENABLE_RELOAD_MODEL

BW_END_NAMESPACE


#endif // RELOAD_HPP
