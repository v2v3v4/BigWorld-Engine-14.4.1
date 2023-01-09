#include "pch.hpp"
#include "managed_effect.hpp"
#include "effect_compiler.hpp"
#include "effect_state_manager.hpp"

#include <d3dx9.h>

#include "resmgr/multi_file_system.hpp"
#include "resmgr/zip_section.hpp"

#include "texture_manager.hpp"
#include "effect_state_manager.hpp"
#include "effect_helpers.hpp"
#include "effect_visual_context.hpp"

#ifdef EDITOR_ENABLED
#include "cstdmf/slow_task.hpp"
#endif//EDITOR_ENABLED

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "managed_effect.ipp"
#endif

namespace Moo {

/*
 *	Helper function to see if a parameter is artist editable
 */
static bool artistEditable( const ComObjectWrap<ID3DXEffect> &pEffect, 
						   D3DXHANDLE hProperty )
{
	BW_GUARD;
	BOOL artistEditable = FALSE;
	D3DXHANDLE hAnnot = pEffect->GetAnnotationByName( hProperty, "artistEditable" );
	if (hAnnot)
		pEffect->GetBool( hAnnot, &artistEditable );	

	hAnnot = pEffect->GetAnnotationByName( hProperty, "UIName" );
	return (artistEditable == TRUE) || (hAnnot != 0);
}

//-- sees is this effect parameter shared among the whole shader's set.
//--------------------------------------------------------------------------------------------------
static bool isShared(ID3DXEffect* pEffect, D3DXHANDLE hProperty)
{
	BW_GUARD;
	BOOL shared = FALSE;
	D3DXHANDLE hAnnot = pEffect->GetAnnotationByName(hProperty, "isShared");
	if (hAnnot)
		pEffect->GetBool(hAnnot, &shared);	

	return (shared == TRUE) || (hAnnot != 0);
}

// -----------------------------------------------------------------------------
// Section: ManagedEffect
// -----------------------------------------------------------------------------

TechniqueInfo::TechniqueInfo() :
	handle_( NULL ),
	settingLabel_( "Invalid Technique" ),
	settingDesc_( "Invalid Technique" ),
	psVersion_( RenderContext::SHADER_VERSION_NOT_INITIALISED ),
	supported_( false ),
	skinned_( false ),
	bumpMapped_( false ),
	dualUV_( false ),
	instanced_( false ),
	renderPassType_( 0 ),
	channelType_( OPAQUE_CHANNEL )
{

}

/**
 *	Constructor.
 */
ManagedEffect::ManagedEffect(bool isEffectPool):
	currentTechnique_( 0 ),
	techniqueExplicitlySet_(),
	settingsListenerId_( -1 ),
	resourceID_( "Not loaded" ),
	settingName_(),
	settingEntry_( 0 ),
	settingsAdded_( false ),
	numPasses_( 0 ),
	compileMark_( 0 ),
	isEffectPool_(isEffectPool)
{
	BW_GUARD;
}

/**
 *	Destructor.
 */
ManagedEffect::~ManagedEffect()
{
	BW_GUARD;

	// Drop smart pointer reference
	if (pEffect_ != NULL)
	{
		pEffect_->SetStateManager( NULL );
	}

	if (EffectManager::pInstance())
	{
		EffectManager::pInstance()->deleteEffect( this );
	}

	// unregister as feature listener from all effects flagged as engine features
	if (graphicsSettingEntry()	!= 0 && 
		settingsListenerId_		!= -1)
	{
		graphicsSettingEntry()->delListener( settingsListenerId_ );
	}
}

/**	
 *	This method sets the automatic constants 
 */
void ManagedEffect::setAutoConstants()
{
	BW_GUARD;
	MappedConstants::iterator it = mappedConstants_.begin();
	MappedConstants::iterator end = mappedConstants_.end();
	while (it != end)
	{
		EffectConstantValue* pValue = (*it->second).getObject();
		if (pValue)
		{
			(*pValue)( pEffect_.pComObject(), it->first );
		}
		++it;
	}
}

/**
 *	Returns the graphics setting entry for this effect, if it has 
 *	been registered as such. Otherwise, returns an empty pointer.
 */
EffectTechniqueSetting * ManagedEffect::graphicsSettingEntry()
{
	BW_GUARD;
	return settingEntry_.getObject(); 
}

/**
 * This method store technique handles and annotation information
 *
 * 
 */
void ManagedEffect::loadTechniqueInfo()
{
	BW_GUARD;

	MF_ASSERT( pEffect_ );

	// Get effect description
	D3DXEFFECT_DESC effectDesc;
	pEffect_->GetDesc( &effectDesc );

	techniques_.clear();
	// Reserve enough assuming all techniques are supported.
	techniques_.reserve( effectDesc.Techniques );

	// For each technique in the effect, add it if returns a valid handle.
	// Also record maximum pixel shader version and "validated" state.
	for (uint32 i = 0; i < effectDesc.Techniques; ++i )
	{
		// get handle
		TechniqueInfo technique;
		technique.handle_=	pEffect_->GetTechnique(i);

		if (technique.handle_ != 0)
		{
			// get name
			D3DXTECHNIQUE_DESC techniqueDesc;
			pEffect_->GetTechniqueDesc( technique.handle_ , &techniqueDesc );
			technique.name_	= techniqueDesc.Name;

			// get channel
			D3DXHANDLE hChannelName	= 
				pEffect_->GetAnnotationByName( technique.handle_, "channel" );

			if ( hChannelName )
			{
				const char* channelName = NULL;
				pEffect_->GetString( hChannelName, &channelName );
				if (!channelName || strcmp(channelName, "none") == 0)
				{
					technique.channelType_ = OPAQUE_CHANNEL;
				}
				else if (strcmp(channelName, "sorted") == 0)
				{
					technique.channelType_ = TRANSPARENT_CHANNEL;
				}
				else if (strcmp(channelName, "internalSorted") == 0)
				{
					technique.channelType_ = TRANSPARENT_INTERNAL_SORT_CHANNEL;
				}
				else if (strcmp(channelName, "shimmer") == 0)
				{
					technique.channelType_ = SHIMMER_CHANNEL;
				}
				else if (strcmp(channelName, "sortedShimmer") == 0)
				{
					technique.channelType_ = SHIMMER_INTERNAL_SORT_CHANNEL;
				}
				else
				{
					technique.channelType_ = OPAQUE_CHANNEL;
					MF_ASSERT_DEV( 0 ); // unsupported channel, crash in debug
				}
			}

			technique.psVersion_ = 2;
			technique.supported_ = true;


			// get skinned annotation
			D3DXHANDLE h = pEffect_->GetAnnotationByName( technique.handle_, 
														"skinned" );

			if (h)
			{
				BOOL b;
				if (SUCCEEDED(pEffect_->GetBool( h, &b )))
				{
					technique.skinned_ = b == TRUE;
				}
			}

			// get bumped annotation
				h = pEffect_->GetAnnotationByName( technique.handle_, 
														"bumpMapped" );
			if (h)
			{
				BOOL b;
				if (SUCCEEDED(pEffect_->GetBool( h, &b )))
				{
					technique.bumpMapped_ = b == TRUE;
				}
			}

			// get dualUV annotation
				h = pEffect_->GetAnnotationByName( technique.handle_, 
														"dualUV" );
			if (h)
			{
				BOOL b;
				if (SUCCEEDED(pEffect_->GetBool( h, &b )))
				{
					technique.dualUV_ = b == TRUE;
				}
			}

			// get instanced annotation
			h = pEffect_->GetAnnotationByName( technique.handle_, "instanced" );
			if (h)
			{
				BOOL b;
				if (SUCCEEDED(pEffect_->GetBool( h, &b )))
				{
					technique.instanced_ = b == TRUE;
				}
			}

			// get render pass type annotation
			h = pEffect_->GetAnnotationByName( technique.handle_, "renderType" );
			if (h)
			{
				INT type = 0;
				if (SUCCEEDED(pEffect_->GetInt( h, &type )))
				{
					technique.renderPassType_ = type;
				}
			}

			techniques_.push_back( technique );
		}
	}
}


namespace 
{
	/**
	 *	Checks if effect parameter is being used by any effect technique
	 *
	 */
	bool isEffectParameterUsed( ID3DXEffect* pEffect, D3DXHANDLE param )
	{
#ifdef EDITOR_ENABLED
		if (artistEditable( pEffect, param ))
		{
			return true;
		}
#endif
		D3DXEFFECT_DESC effectDesc;
		pEffect->GetDesc( &effectDesc );
		UINT numTechniques = effectDesc.Techniques;

		for (UINT techniqueIndex = 0; techniqueIndex < numTechniques; ++techniqueIndex)
		{
			D3DXHANDLE technique = pEffect->GetTechnique( techniqueIndex );
			if (pEffect->IsParameterUsed( param, technique ))
			{
				return true;
			}
		}

		return false;
	}
}
/**
 *	Load effect from file and bind to semantics.
 *
 *	@param effectResource file path.
 *	@return true on success.
 */
bool ManagedEffect::load( const BW::string& effectResource )
{
	BW_GUARD;
	resourceID_ = effectResource;
	return this->reload();
}

/**
 *	Reloads the effect from disk, and binds to semantics.
 *
 *	@return true on success.
 */
bool ManagedEffect::reload()
{
	BW_GUARD;
	MF_ASSERT( !resourceID_.empty() );

	//
	// Load the raw shader binary from disk
	BinaryPtr bin = EffectHelpers::loadEffectBinary( resourceID_ );
	if (!bin)
	{
		ASSET_MSG( "Error loading effect '%s'\n",
			resourceID_.c_str() );

		return false;
	}

	MF_ASSERT( bin != NULL );

	//
	// Create the effect
#ifndef EDITOR_ENABLED
	//This is so we don't create effects while other d3dx functions
	//are creating other resources.
	rc().getD3DXCreateMutex().grab();
#endif

	ID3DXEffect* pEffect = NULL;
	HRESULT hr = D3DXCreateEffect( rc().device(),
		bin->cdata(), bin->len(),
		NULL, NULL, USE_LEGACY_D3DX9_DLL, 
		rc().effectVisualContext().pEffectPool(), 
		&pEffect, NULL );

#ifndef EDITOR_ENABLED
	rc().getD3DXCreateMutex().give();
#endif

	if (FAILED( hr ))
	{
		ASSET_MSG( "Error creating effect for '%s' (reason = %s)\n",
			resourceID_.c_str(), 
			DX::errorAsString( hr ).c_str() );
		if (E_FAIL == hr)
			ERROR_MSG( "Check if DirectX 9 installed on your system!\n" );

		return false;
	}

	//
	// Reload everything from this new effect instance.
	compileMark_++;
	settingsListenerId_ = -1;
	settingName_ = "";
	settingEntry_ = 0;
	settingsAdded_ = false;
	currentTechnique_ = NULL;
	techniques_.clear();

	// Associate our state manager with the effect
	pEffect->SetStateManager( EffectManager::instance().pStateManager() );
	D3DXEFFECT_DESC effectDesc;
	pEffect->GetDesc( &effectDesc );

	// Iterate over the effect parameters.
	defaultProperties_.clear();
	mappedConstants_.clear();
	for (uint32 i = 0; i < effectDesc.Parameters; i++)
	{
		D3DXHANDLE param = pEffect->GetParameter( NULL, i );
		D3DXPARAMETER_DESC paramDesc;
		pEffect->GetParameterDesc( param, &paramDesc );

		// skip effect parameter if it's not being used by any effect technique
		// do not skip if this is an effect pool
		if (!isEffectParameterUsed( pEffect, param ) &&
			!isEffectPool_)
		{
			continue;
		}

		if (artistEditable( pEffect, param ))
		{
			// If the parameter is artist editable find the right 
			// EffectProperty processor and create the default 
			// property for it
			const EffectPropertyFactoryPtr& propertyFactory =
				EffectPropertyFactory::findFactory( &paramDesc );

			if (propertyFactory)
			{
				defaultProperties_[ paramDesc.Name ] =
					propertyFactory->create( paramDesc.Name,
						param, pEffect );
			}
			else
			{
				ERROR_MSG( "Could not find property processor for %s\n",
					paramDesc.Name );
			}
		}
		else if (paramDesc.Semantic)
		{
			//-- If the parameter has a Semantic, set it up as an automatic constant, but if
			//--     it is also shared skip it because shared constants are set by effect pool
			//--	 mechanism only once per frame for all effect files.
			if (!isShared(pEffect, param) || isEffectPool_)
			{
				mappedConstants_.push_back(
					std::make_pair( param,
						rc().effectVisualContext().getMapping( paramDesc.Semantic ) ) );
			}
		}
	}

	if (pEffect_ != NULL)
	{
		pEffect_->SetStateManager( NULL );
	}

 	pEffect_ =  pEffect;
	pEffect->Release();

	// finish init
	this->loadTechniqueInfo();

	this->registerGraphicsSettings( resourceID() );

	EffectTechniqueSetting* pSettingsEntry = 
		this->graphicsSettingEntry();

	if (pSettingsEntry != 0 && 
		pSettingsEntry->activeTechniqueHandle() != 0)
	{
		// first set ourselves as a listener and add listener id
		settingsListenerId_ = pSettingsEntry->addListener( this );

		// set the active technique for our graphics settings
		this->currentTechnique( pSettingsEntry->activeTechniqueHandle(),
			true );
	}
	else if (techniques_.size() > 0)
	{
		this->currentTechnique( techniques_[0].handle_, false );
	}

	return true;
}

/**
 *	Retrieve "graphicsSetting" label and "label" annotation from 
 *	the effect file, if any. Effects labeled as graphics settings will 
 *	be registered into moo's graphics settings registry. Each technique 
 *	tagged by a "label" annotation will be added as an option to the
 *	effect's graphic setting object. Materials and other subsystems can 
 *	then register into the setting entry and select the appropriate 
 *	technique when the selected options/technique changes.
 *
 *	Like any graphics setting, more than one effect can be registered 
 *	under the same setting entry. For this to happen, they must share 
 *	the same label, have the same number of tagged techniques, each
 *	with the same label and appearing in the same order. If two or 
 *	more effects shared the same label, but the above rules are 
 *	not respected, an assertion will fail. 
 *
 *	@param	effectResource	name of the effect file.
 *	@return	False if application was quit during processing.
 *	@see	GraphicsSetting::add
 */
bool ManagedEffect::registerGraphicsSettings(
	const BW::string & effectResource )
{
	BW_GUARD;
 	if (settingsAdded_)
	{
 		return true;
	}

	if (!Moo::rc().checkDeviceYield())
	{
		ERROR_MSG( "ManagedEffect: device lost when registering settings\n" );
		return false;
	}

	D3DXHANDLE feature = pEffect_->GetParameterByName( 0, "graphicsSetting" );

	if (feature != 0)
	{
		bool labelExists = false;
		bool descExists = false;

		D3DXHANDLE nameHandle = 
			pEffect_->GetAnnotationByName( feature, "label" );

		LPCSTR settingName = NULL;
		if (nameHandle != 0)
		{
			labelExists = 
				SUCCEEDED( pEffect_->GetString( nameHandle, &settingName ) );
		}

		D3DXHANDLE descHandle = 
			pEffect_->GetAnnotationByName( feature, "desc" );

		LPCSTR settingDesc = NULL;
		if (descHandle != 0)
		{
			descExists = 
				SUCCEEDED( pEffect_->GetString( descHandle, &settingDesc) );
		}

		if (labelExists && descExists)
		{
			// Set our name and description
			settingName_ = settingName;
			settingDesc_ = settingDesc;

			// For each existing technique, add name based on label.
			for ( uint32 i = 0; i < techniques_.size(); i++ )
			{
				labelExists = false;
				descExists = false;

				TechniqueInfo& technique = techniques_[i];

				IF_NOT_MF_ASSERT_DEV( technique.handle_ != NULL )
				{
					return false;
				}

				LPCSTR techLabel = NULL;
				D3DXHANDLE techLabelHandle = 
					pEffect_->GetAnnotationByName( technique.handle_, "label");

				if (techLabelHandle != 0)
				{
					labelExists = SUCCEEDED(pEffect_->GetString(
						techLabelHandle, &techLabel) );
				}

				LPCSTR techDesc = NULL;
				D3DXHANDLE techDescHandle = 
					pEffect_->GetAnnotationByName( technique.handle_, "desc" );

				if (techDescHandle != 0)
				{
					descExists = SUCCEEDED(pEffect_->GetString(
						techDescHandle, &techDesc) );
				}

				if (labelExists && descExists)
				{
					technique.settingLabel_ = techLabel;
					technique.settingDesc_	= techDesc;
				}
				else
				{
					D3DXTECHNIQUE_DESC desc;
					pEffect_->GetTechniqueDesc( technique.handle_, &desc );
					WARNING_MSG(
						"Effect labeled as a feature but either the"
						"\"label\" or \"desc\" annotations provided for "
						"technique \"%s\" are missing (won't be listed)\n", 
						desc.Name );
				}
			}	
		}
		else 
		{
			ERROR_MSG(
				"Effect \"%s\" labeled as feature but either "
				"the \"label\" or \"desc\" annotations are missing\n", 
				effectResource.c_str() );
		}
	}

	// register engine feature if 
	// effect is labeled as such
	if (!settingName_.empty() && !techniques_.empty())
	{
		settingEntry_ = new EffectTechniqueSetting(
			this, settingName_, settingDesc_ );

		typedef TechniqueInfoCache::const_iterator citerator;
		citerator techIt  = techniques_.begin();
		citerator techEnd = techniques_.end();
		while (techIt != techEnd)
		{
			settingEntry_->addTechnique( *techIt );
			++techIt;
		}

		// register it
		GraphicsSetting::add( settingEntry_ );
	}

	settingsAdded_ = true;
	return true;
}

/**
 * This method returns the name of the current technique.
 */
const BW::string ManagedEffect::currentTechniqueName()
{
	BW_GUARD;
	D3DXHANDLE hTec	= this->currentTechnique();
	TechniqueInfo* entry = this->findTechniqueInfo( hTec );

	if (entry)
	{
		return entry->name_;
	}

	return "Unknown technique";
}

/**
 * This method returns the channel annotation (or null) of the given technique
 * on the effect.
 */
ChannelType ManagedEffect::getChannelType( Handle techniqueOverride )
{
	BW_GUARD;
	Handle hTec = techniqueOverride == NULL ? 
		this->currentTechnique() : techniqueOverride;
	TechniqueInfo* entry = this->findTechniqueInfo( hTec );
	return entry ? entry->channelType_ : NOT_SUPPORTED_CHANNEL;
}

/**
 *	This method returns the skinned status for given technique.
 */
bool ManagedEffect::skinned( D3DXHANDLE techniqueOverride )
{ 
	BW_GUARD;

	D3DXHANDLE hTec	= techniqueOverride == NULL ? 
						this->currentTechnique() : techniqueOverride;
	TechniqueInfo* entry = this->findTechniqueInfo( hTec );
	return entry ? entry->skinned_ : false;
}

/**
 * This method returns the bumped status for given technique.
 */
bool ManagedEffect::bumpMapped( D3DXHANDLE techniqueOverride ) 
{ 
	BW_GUARD;

	D3DXHANDLE hTec	= techniqueOverride == NULL ? 
						this->currentTechnique() : techniqueOverride;
	TechniqueInfo* entry = this->findTechniqueInfo( hTec );
	return entry ? entry->bumpMapped_ : false;
}

/**
 * This method returns the bumped status for given technique.
 */
bool ManagedEffect::dualUV( D3DXHANDLE techniqueOverride ) 
{ 
	BW_GUARD;

	D3DXHANDLE hTec	= techniqueOverride == NULL ? 
						this->currentTechnique() : techniqueOverride;
	TechniqueInfo* entry = this->findTechniqueInfo( hTec );
	return entry ? entry->dualUV_ : false;
}



bool ManagedEffect::begin( bool setAutoConstants )
{
	BW_GUARD;
	MF_ASSERT( pEffect_ != NULL );

	D3DXHANDLE hCurrentTechnique = this->currentTechnique();
	MF_ASSERT( hCurrentTechnique != NULL );

	pEffect_->SetTechnique( hCurrentTechnique );
	if (setAutoConstants)
	{
		this->setAutoConstants();
	}

	numPasses_ = 0;
	return SUCCEEDED( pEffect_->Begin( &numPasses_, D3DXFX_DONOTSAVESTATE ) );
}

bool ManagedEffect::beginPass( uint32 pass )
{
	BW_GUARD;
	MF_ASSERT( pEffect_ != NULL );
	MF_ASSERT( this->beginCalled() );
	MF_ASSERT( pass < numPasses_ );
	return SUCCEEDED( pEffect_->BeginPass( pass ) );
}

bool ManagedEffect::endPass()
{
	BW_GUARD;
	MF_ASSERT( pEffect_ != NULL );
	return SUCCEEDED( pEffect_->EndPass() );
}

bool ManagedEffect::end()
{
	BW_GUARD;
	MF_ASSERT( pEffect_ != NULL );
	MF_ASSERT( this->beginCalled() );

	numPasses_ = 0;
	return SUCCEEDED( pEffect_->End() );
}

bool ManagedEffect::commitChanges()
{
	BW_GUARD;
	MF_ASSERT( pEffect_ != NULL );
	return SUCCEEDED( pEffect_->CommitChanges() );
}

/**
 * This method returns the instanced status for given technique.
 */
bool ManagedEffect::instanced( D3DXHANDLE techniqueOverride ) 
{ 
	BW_GUARD;

	D3DXHANDLE hTec	= techniqueOverride == NULL ?  currentTechnique() : techniqueOverride;
	TechniqueInfo* entry = this->findTechniqueInfo( hTec );

	if ( entry )
	{
		return entry->instanced_;
	}

	ERROR_MSG( "Unknown technique %s (%p) on effect %s.\n", hTec, hTec,
		resourceID_.c_str() );

	return false; 
}


/**
*	Internal method for setting current technique.
*
*	@param hTec handle to the technique to use.
*	@return true if successful
*/
bool ManagedEffect::currentTechnique( Handle hTec, bool setExplicit )
{
	BW_GUARD;

#ifdef _DEBUG
	TechniqueInfo* entry = this->findTechniqueInfo( hTec );
	if (!entry)
	{
		D3DXTECHNIQUE_DESC techniqueDesc;
		if (!SUCCEEDED( pEffect_->GetTechniqueDesc( hTec , &techniqueDesc ) ))
		{
			techniqueDesc.Name = "<invalid>";
		}

		ASSET_MSG("Unknown technique %s on effect %s.\n", techniqueDesc.Name, 
			resourceID_.c_str() );

		return false;
	}
#endif

	techniqueExplicitlySet_ = setExplicit;
	currentTechnique_		= hTec;

	return true;
}

/**
* This method is called when a different technique is selected (eg for a 
* graphics setting change).
*/
void ManagedEffect::onSelectTechnique( ManagedEffect* effect, Handle hTec )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( isRenderThread() == true )
	{
		ERROR_MSG( "ManagedEffect::onSelelctTechnique - not called from "
			"render thread\n" );
		return;
	}

	this->currentTechnique( hTec, true );
}

void ManagedEffect::onSelectPSVersionCap( int psVerCap )
{
}

// -----------------------------------------------------------------------------
// Section: EffectTechniqueSetting
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param	owner	owning ManagedEffect object.
 *	@param	label	label for setting.
 */
EffectTechniqueSetting::EffectTechniqueSetting(
		ManagedEffect * owner,
		const BW::string & label,
		const BW::string & desc):
	GraphicsSetting(label, desc, -1, false, false),
	owner_(owner),
	techniques_(),
	listeners_()
{}

/**
 *	Adds new technique to effect. This will add a new 
 *	option by the same label into the base GraphicsSetting.
 *
 *	@param	info	technique information.
 */
void EffectTechniqueSetting::addTechnique( const TechniqueInfo& info )
{
	BW_GUARD;
	this->GraphicsSetting::addOption(
		info.settingLabel_, info.settingDesc_, info.supported_ );
	techniques_.push_back(
		std::make_pair( info.handle_, info.psVersion_ ) );
}

/**
 *	Virtual functions called by the base class whenever the current 
 *	option changes. Will notify all registered listeners and slaves.
 *
 *	@param	optionIndex	index of new option selected.
 */
void EffectTechniqueSetting::onOptionSelected( int optionIndex )
{
	BW_GUARD;
	this->listenersLock_.grab();
	ListenersMap::iterator listIt  = this->listeners_.begin();
	ListenersMap::iterator listEnd = this->listeners_.end();
	while (listIt != listEnd)
	{
		listIt->second->onSelectTechnique(
			this->owner_, this->techniques_[optionIndex].first );
		++listIt;
	}
	this->listenersLock_.give();
}		

/**
 *	Sets the pixel shader version cap. Updates effect to use a supported 
 *	technique that is still within the new cap. If none is found, use the
 *	last one available (should be the less capable).
 *
*	@param	psVersionCap	maximum major version number.
 */
void EffectTechniqueSetting::setPSCapOption(int psVersionCap)
{
	BW_GUARD;
	typedef D3DXHandlesPSVerVector::const_iterator citerator;
	citerator techBegin = this->techniques_.begin();
	citerator techEnd   = this->techniques_.end();
	citerator techIt    = techBegin;
	while (techIt != techEnd)
	{
		if (techIt->second <= psVersionCap)
		{
			size_t optionIndex = std::distance(techBegin, techIt);
			MF_ASSERT( optionIndex <= INT_MAX );
			if (this->GraphicsSetting::selectOption( ( int ) optionIndex ) )
				break;
		}
		++techIt;
	}
	if (techIt == techEnd)
	{
		this->GraphicsSetting::selectOption(static_cast<int>(
												this->techniques_.size()-1));
	}
}

/**
 *	Returns handle to currently active technique.
 *
 *	@return	handle to active technique.
 */
D3DXHANDLE EffectTechniqueSetting::activeTechniqueHandle() const
{
	BW_GUARD;
	int activeOption = this->GraphicsSetting::activeOption();
	if (activeOption == -1)
	{
		return 0;
	}

	if (activeOption >= (int)(this->techniques_.size()))
	{
		return 0;
	}

	return this->techniques_[activeOption].first;
}

/**
 *	Registers an EffectTechniqueSetting listener instance.
 *
 *	@param	listener	listener to register.
 *	@return				id of listener (use this to unregister).
 */
int EffectTechniqueSetting::addListener( IListener* listener )
{
	BW_GUARD;
	++EffectTechniqueSetting::listenersId;

	listenersLock_.grab();
	listeners_.insert(
		std::make_pair( EffectTechniqueSetting::listenersId, listener ) );
	listenersLock_.give();

	return EffectTechniqueSetting::listenersId;
}

/**
 *	Unregisters EffectTechniqueSetting listener identified by given id.
 *
 *	@param	listenerId	id of listener to be unregistered.
 */
void EffectTechniqueSetting::delListener(int listenerId)
{
	BW_GUARD;
	this->listenersLock_.grab();
	ListenersMap::iterator listIt = this->listeners_.find(listenerId);
	MF_ASSERT_DEV(listIt !=  this->listeners_.end());
	if( listIt != this->listeners_.end() )
		this->listeners_.erase(listIt);
	this->listenersLock_.give();
}

int EffectTechniqueSetting::listenersId = 0;

} // namespace Moo

BW_END_NAMESPACE

// managed_effect.cpp
