#include "pch.hpp"
#include "effect_manager.hpp"
#include "effect_macro_setting.hpp"

#include "resmgr/auto_config.hpp"

#include "effect_state_manager.hpp"
#include "effect_compiler.hpp"
#include "effect_helpers.hpp"
#include "managed_effect.hpp"

#if ENABLE_ASSET_PIPE
#include "asset_pipeline/asset_client.hpp"
#endif

BW_BEGIN_NAMESPACE

// Singleton instance implementation
BW_SINGLETON_STORAGE( Moo::EffectManager );

namespace { 
	const int c_MaxPixelShaderVersion = 3;
} // anonymous namespace

namespace Moo {

// ----------------------------------------------------------------------------
// Section: EffectManager
// ----------------------------------------------------------------------------

// Constructor
EffectManager::EffectManager()
	: effects_()
	, pStateManager_( new StateManager() )
	, effectsLock_()
	, listeners_()
#if ENABLE_ASSET_PIPE
	, pAssetClient_( NULL )
#endif
{
	BW_GUARD;

	// Initialise the macro settings for the effect files
	// These settings are used to select different graphics
	// options when compiling effects
	EffectMacroSetting::setupSettings();
	EffectMacroSetting::updateManagerInfix();

	//
	// Set up the max pixel shader version graphics option

	const int maxPSVerSupported = (Moo::rc().psVersion() & 0xff00) >> 8;
	const int currentSetting = c_MaxPixelShaderVersion - maxPSVerSupported;

	// When using the Direct3D 9Ex device, the graphics card must have a
	// minimum of SM 2.0. No need to support lower.
	int minPSVerSupported = 0;
	if (Moo::rc().usingD3DDeviceEx())
	{
		minPSVerSupported = 2;
	}

	// Disable fixed function option if the graphics card is SM1 series
	// Bug 18920
	else if (maxPSVerSupported == 1)
	{
		minPSVerSupported = 1;
	}

	psVerCapSettings_ = Moo::makeCallbackGraphicsSetting(
		"SHADER_VERSION_CAP",
		"Shader Version Cap",
		*this, 
		&EffectManager::setPSVerCapOption, 
		currentSetting,
		false,
		false );

	for (int i = 0; i <= c_MaxPixelShaderVersion; ++i)
	{
		const int shaderVersion = (c_MaxPixelShaderVersion - i);
		BW::stringstream verStream;
		verStream << "SHADER_MODEL_" << shaderVersion;
		BW::stringstream descStream;
		if (shaderVersion == 0)
		{
			descStream << "Fixed Function";
		}
		else
		{
			descStream << "Shader Model " << shaderVersion;
		}

		const bool supported = ((shaderVersion >= minPSVerSupported) &&
			(shaderVersion <= maxPSVerSupported));
		if (supported)
		{
			psVerCapSettings_->
				addOption( verStream.str(), descStream.str(), supported );
		}
	}
	Moo::GraphicsSetting::add(psVerCapSettings_);

	BWResource::instance().addModificationListener( this );
}

// Destructor
EffectManager::~EffectManager()
{
	BW_GUARD;

	// The reference count for StateManager should be 1, as EffectManager should
	// now be the only one holding a reference and is dropping it at the end
	// of its destructor
	if (pStateManager_->refCount() > 1)
	{
		WARNING_MSG( "EffectManager::doFini: StateManager not deleted, "
				"%" PRIzu " ManagedEffect's are still using it.\n",
			pStateManager_->refCount() - 1 );
	}
	// Drop reference
	pStateManager_ = NULL;

	BWResource::instance().removeModificationListener( this );

	// Count remaining references, should be exactly 0 or there are reference
	// leaks
	Effects::iterator	it	= effects_.begin();
	uint32				i	= 0;
	bool				msg	= false;

	while (it != effects_.end())
	{
		if ( !msg )
		{
			WARNING_MSG( "EffectManager::~EffectManager: "
					"contains %" PRIzu " effects on destruct.\n"
					"This could indicate a possible reference leak "
					"or destruction order issue.\n",
				effects_.size() );
			msg = true;
		}

		WARNING_MSG( "%d - Effect %s not unloaded in time. \n", 
					++i, it->first.c_str() );

		// next managed effect
		it++;
	}

	EffectMacroSetting::finiSettings();
}

/*
*	Remove the effect from the effect manager.
*/
void EffectManager::deleteEffect( ManagedEffect* pEffect )
{
	BW_GUARD;
	SimpleMutexHolder smh( effectsLock_ );

	this->delListener( pEffect );

	Effects::iterator it = effects_.begin();
	Effects::iterator end = effects_.end();
	while (it != end)
	{
		if (it->second == pEffect)
		{
			effects_.erase( it );
			return;
		}
		else
			it++;
	}
}

/**
*	Sets the current pixel shader version cap. This will force all 
*	MaterialEffects that use GetNextValidTechnique to limit the valid 
*	techniques to those within the current pixel shader version cap. 
*	Effect files registered as graphics settings will also be capped,
*	but they can be individually reset later. Implicitly called when
*	the user changes the PS Version Cap's active option.
*
*	@param activeOption		the new active option index.
*/
// TODO: this should really be in EffectMaterial.
void EffectManager::setPSVerCapOption(int activeOption)
{
	BW_GUARD;
	Effects::const_iterator effIt = this->effects_.begin();
	Effects::const_iterator effEnd = this->effects_.end();

	while (effIt != effEnd)
	{
		EffectTechniqueSetting * setting = effIt->second->graphicsSettingEntry();

		if ( setting == NULL && 
			effIt->second->registerGraphicsSettings( effIt->first ) )
		{
			// on creation success.
			setting = effIt->second->graphicsSettingEntry();
		}

		// If we did create a setting, use it 
		if ( setting )
		{
			setting->setPSCapOption(this->PSVersionCap());
		}
		effIt++;
	}

	// update all listeners (usually EffectMaterial instances)
	this->listenersLock_.grab();
	ListenerVector::const_iterator listIt  = this->listeners_.begin();
	ListenerVector::const_iterator listEnd = this->listeners_.end();
	while (listIt != listEnd)
	{
		(*listIt)->onSelectPSVersionCap(c_MaxPixelShaderVersion-activeOption);
		++listIt;
	}
	this->listenersLock_.give();
}

//--------------------------------------------------------------------------------------------------
ManagedEffectPtr EffectManager::createEffectPool( const BW::string& resourceID )
{
	return this->get( resourceID, true, true );
}

/**
*	Block until an effect has been compiled
*
*	@param resourceID the name of the effect to wait on
*/
void EffectManager::ensureCompiled( const BW::string& resourceID )
{
#if ENABLE_ASSET_PIPE
	if (pAssetClient_ != NULL)
    {
	    BW::string fxoName = EffectHelpers::fxoName( resourceID );
	    pAssetClient_->requestAsset( fxoName, true );
    }
#endif
}


/**
*	Get the effect from the manager, create it if it's not there.
*
*	@param resourceID the name of the effect to load
*	@return smart pointer to the loaded effect
*/
ManagedEffectPtr EffectManager::get( const BW::string& resourceID,
					bool loadIfNotLoaded,
					bool isEffectPool )
{
	BW_GUARD;

	// See if we have a copy of the effect already
	ManagedEffect* pEffect = this->find( resourceID );
	if (!pEffect && loadIfNotLoaded)
	{
		// ensure the effect is compiled before we try to load it
		ensureCompiled( resourceID );

		// We don't allow loading multiple shaders at the same time.
		SimpleMutexHolder smh( loadMutex_ );

		// Create the effect if it could not be found.
		pEffect = new ManagedEffect(isEffectPool);
		if (pEffect->load( resourceID ))
		{
			this->add( pEffect, resourceID );
		}
		else
		{
			ASSET_MSG( "EffectManager::get - unable to load %s\n",
				resourceID.c_str() );
			bw_safe_delete(pEffect);
		}
	}
	return pEffect;
}

/**
*	Find an effect in the effect manager
*	@param resourceID the name of the effect
*	@return pointer to the effect
*/
ManagedEffect* EffectManager::find( const BW::string& resourceID )
{
	BW_GUARD;
	SimpleMutexHolder smh( effectsLock_ );
	ManagedEffect* pEffect = NULL;
	Effects::iterator it = effects_.find( resourceID );
	if (it != effects_.end())
	{
		pEffect = it->second;
	}

	return pEffect;
}

/**
*	Add an effect to the effect manager
*	@param pEffect the effect
*	@param resourceID the name of the effect
*/
void EffectManager::add( ManagedEffect* pEffect,
						const BW::string& resourceID )
{
	BW_GUARD;
	SimpleMutexHolder smh( effectsLock_ );
	effects_.insert( std::make_pair( resourceID, pEffect ) );
}

/**
*	DeviceCallback override
*/
void EffectManager::createUnmanagedObjects()
{
	BW_GUARD;
	SimpleMutexHolder smh( effectsLock_ );

	Effects::iterator it = effects_.begin();
	Effects::iterator end = effects_.end();
	while (it != end)
	{
		if (it->second)
		{
			it->second->pEffect()->OnResetDevice();
		}
		it++;
	}
}

/**
*	DeviceCallback override
*/
void EffectManager::deleteUnmanagedObjects()
{
	BW_GUARD;
	SimpleMutexHolder smh( effectsLock_ );

	Effects::iterator it = effects_.begin();
	Effects::iterator end = effects_.end();
	while (it != end)
	{
		if (it->second)
		{
			it->second->pEffect()->OnLostDevice();
		}
		it++;
	}
}

/**
*	Retrieves the current pixel shader version cap (major version number).
*/
int EffectManager::PSVersionCap() const
{
	BW_GUARD;
	return (c_MaxPixelShaderVersion - this->psVerCapSettings_->activeOption());
}

/**
*	Set the current pixel shader version cap (major version number).
*/
void EffectManager::PSVersionCap( int psVersion )
{
	BW_GUARD;
	this->psVerCapSettings_->selectOption( c_MaxPixelShaderVersion - psVersion );
}


/**
*	This method returns the singleton's state manager.
*
*	@return		Pointer to the StateManager.
*/
StateManager * EffectManager::pStateManager()
{
	BW_GUARD;
	MF_ASSERT( pStateManager_.hasObject() );
	return pStateManager_.getObject();
}


/**
*	Registers an EffectManager listener instance.
*/
void EffectManager::addListener(IListener * listener)
{
	BW_GUARD;
	this->listenersLock_.grab();
	ListenerVector::iterator it = std::find(
		this->listeners_.begin(), 
		this->listeners_.end(), 
		listener);
	MF_ASSERT_DEV( it == this->listeners_.end());

	if( it == this->listeners_.end() )
		this->listeners_.push_back(listener);
	this->listenersLock_.give();
}

/**
*	Unregisters an EffectManager listener instance.
*/
void EffectManager::delListener(IListener * listener)
{
	BW_GUARD;
	this->listenersLock_.grab();
	ListenerVector::iterator listIt = std::find(
		this->listeners_.begin(), this->listeners_.end(), listener);

	if( listIt != this->listeners_.end() )
		this->listeners_.erase(listIt);

	this->listenersLock_.give();
}

/**
*	Runs through all loaded effect files, registering 
*	their graphics settings when relevant. 
*
*	@return	False if application was quit during processing.
*/
bool EffectManager::registerGraphicsSettings()
{
	BW_GUARD;
	bool result = true;
	Effects::iterator it = effects_.begin();
	Effects::iterator end = effects_.end();
	while (it != end)
	{
		if (it->second)
		{
			if (!it->second->registerGraphicsSettings(it->first))
			{
				result = false;
				break;
			}
		}
		it++;
	}
	return result;
}


namespace
{
class ReloadContext
{
public:
	ReloadContext( const ManagedEffectPtr& effect ) :
		pEffect_( effect )
	{}

	void reload( ResourceModificationListener::ReloadTask& task )
	{
		BWResource::WatchAccessFromCallingThreadHolder holder( false );
		pEffect_->reload();
	}

private:
	ManagedEffectPtr pEffect_;
};
} // End anon namespace


/**
 *	Notification from the file system that something we care about has been
 *	modified at run time. Let us reload shaders.
 */
void EffectManager::onResourceModified( 
	const BW::StringRef& basePath, const BW::StringRef& resourceIDRef,
	ResourceModificationListener::Action action )
{
	BW_GUARD;

	if (action == ResourceModificationListener::ACTION_DELETED)
	{
		return;
	}

	BW::string fxName;
	if (EffectHelpers::fxName( resourceIDRef, fxName ))
	{
		ManagedEffect* pEffect = this->find( fxName );
		if (NULL != pEffect)
		{
			queueReloadTask( ReloadContext( pEffect ), basePath, 
				resourceIDRef );
		}
	}
}

} // namespace Moo

BW_END_NAMESPACE

// effect_manager.cpp
