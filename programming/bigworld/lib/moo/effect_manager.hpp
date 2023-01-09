#ifndef __EFFECT_MANAGER_HPP__
#define __EFFECT_MANAGER_HPP__

#include <deque>
#include "cstdmf/bw_set.hpp"

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/singleton.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "math/vector4.hpp"

#include "moo_dx.hpp"
#include "forward_declarations.hpp"
#include "device_callback.hpp"
#include "effect_state_manager.hpp"


BW_BEGIN_NAMESPACE

#if ENABLE_ASSET_PIPE
class AssetClient;
#endif // ENABLE_ASSET_PIPE

namespace Moo {

/**
*	This class loads and manages managed effects.
*/
class EffectManager : 
	public DeviceCallback,
	public ResourceModificationListener,
	public Singleton< EffectManager >
{
public:

	/**
	*	Interface for objects desiring callbacks at selection of PS version cap.
	*/
	class IListener
	{
	public:
		virtual ~IListener() {}
		virtual void onSelectPSVersionCap( int psVerCap ) = 0;
	};

	EffectManager();
	~EffectManager();

	void ensureCompiled( const BW::string& resourceID );
	//-- creates effect pool as a managed effect. There may be only one effect pool per constant 
	//-- group. Effect pool acts as a shader constant table handler it gives opportunity to set
	//-- shared constants in the very effective way. Every time when some shared parameters have been
	//-- changed, for example Time, View, ViewProj, Screen constants, we have to send theirs updated
	//-- values to the device through calling EffectVisualContext's function updateSharedConstants().
	ManagedEffectPtr	createEffectPool( const BW::string& resourceID );

	ManagedEffectPtr get( const BW::string& resourceID,
				bool loadIfNotLoaded = true,
				bool isEffectPool = false );

	bool registerGraphicsSettings();

	int PSVersionCap() const;
	void PSVersionCap( int psVersion );

	void addListener(IListener * listener);
	void delListener(IListener * listener);

#if ENABLE_ASSET_PIPE
	void setAssetClient( AssetClient * pAssetClient )
	{
		pAssetClient_ = pAssetClient;
	}
#endif

protected:
	virtual void onResourceModified(
		const BW::StringRef& basePath,
		const BW::StringRef& resourceID,
		ResourceModificationListener::Action action);

private:
	void createUnmanagedObjects();
	void deleteUnmanagedObjects();

	StateManager * pStateManager();

private:
	friend class ManagedEffect;
	void deleteEffect( ManagedEffect* pEffect );

	typedef GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;
	typedef BW::map< BW::string, ManagedEffect* > Effects;
	typedef BW::list< ManagedEffectPtr >			EffectList;
	typedef BW::vector<IListener *>				ListenerVector;

	ManagedEffect*		find( const BW::string& resourceID );
	void				add( ManagedEffect* pEffect, 
								const BW::string& resourceID );

	void			setPSVerCapOption( int );
private:
	SimpleMutex loadMutex_;

	// All loaded effects
	// These are *not reference counted* in here, so that they get unloaded 
	// manually via the ManagedEffect destructor when all *external* references
	// are gone, e.g. the last referencing EffectMaterial was destroyed.
	// If we kept a reference here, ManagedEffects would never unload.
	Effects							effects_;

	SmartPointer< StateManager >	pStateManager_;
	SimpleMutex						effectsLock_;

	ListenerVector					listeners_;
	SimpleMutex						listenersLock_;
	GraphicsSettingPtr				psVerCapSettings_;

private:

#if ENABLE_ASSET_PIPE
	AssetClient * pAssetClient_;
#endif
};

} // namespace Moo

BW_END_NAMESPACE

#endif 
