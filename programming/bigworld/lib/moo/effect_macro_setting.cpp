#include "pch.hpp"

#include "effect_macro_setting.hpp"
#include "effect_compiler.hpp"


BW_BEGIN_NAMESPACE

namespace {

Moo::EffectMacroSetting::MacroSettingVector* s_pSettings = NULL;

/**
 *	Helper function to work around static-initialisation-order issues.
 */
Moo::EffectMacroSetting::MacroSettingVector & static_settings()
{
	if (!s_pSettings)
	{
		s_pSettings = new Moo::EffectMacroSetting::MacroSettingVector;
		atexit(&Moo::EffectMacroSetting::finiSettings);
	}
	return *s_pSettings;
}

/**
 *	Predicate for sorting macro settings.
 */
bool settingsPred( Moo::EffectMacroSetting::EffectMacroSettingPtr a, 
				  Moo::EffectMacroSetting::EffectMacroSettingPtr b )
{
	BW_GUARD;
	return strcmp( a->macroName().c_str(), b->macroName().c_str() ) < 0;
}

} // namespace anonymous

namespace Moo {

// ----------------------------------------------------------------------------
// Section: EffectMacroSetting
// ----------------------------------------------------------------------------

/**
 *	Constructor.
 *
 *	@param	label		label for setting.
 *	@param	macro		macro to be defined.
 *	@param	setupFunc	callback that will setup this EffectMacroSetting.
 */
EffectMacroSetting::EffectMacroSetting(
		const BW::string & label, 
		const BW::string & desc,
		const BW::string & macro, 
		SetupFunc setupFunc,
		bool advanced) :
	GraphicsSetting(label, desc, -1, false, true, advanced),
	macro_(macro),
	setupFunc_(setupFunc),
	values_()
{
	BW_GUARD;
	EffectMacroSetting::add(this);
}

/**
 *	Adds option to this EffectMacroSetting.
 *
 *	@param	label		label for option.
 *	@param	desc		description for option.
 *	@param	isSupported	true if option is supported in this system.
 *	@param	value		value the macro should be defined to when is 
 *						option is active.
 */
void EffectMacroSetting::addOption(
	const BW::string & label, 
	const BW::string & desc, 
	bool                isSupported, 
	const BW::string & value)
{
	BW_GUARD;
	this->GraphicsSetting::addOption( label, desc, isSupported );
	this->values_.push_back( value );
}

/**
 *	Defines the macro according to the option selected. Implicitly 
 *	called whenever the user changes the current active option.
 */		
void EffectMacroSetting::onOptionSelected(int optionIndex)
{
	BW_GUARD;
	EffectCompiler::setMacroDefinition(
		this->macro_,
		this->values_[optionIndex] );
}

/**
 *	Registers setting to static list of EffectMacroSetting instances.
 */
void EffectMacroSetting::add( EffectMacroSettingPtr setting )
{
	BW_GUARD;
	static_settings().push_back( setting );
}

/**
 *	Update the EffectManager's fxo infix according to the currently 
 *	selected macro effect options.
 */
void EffectMacroSetting::updateManagerInfix()
{
	BW_GUARD;
	BW::stringstream infix;
	EffectMacroSetting::MacroSettingVector& settings = static_settings();
	MacroSettingVector::const_iterator setIt  = settings.begin();
	MacroSettingVector::const_iterator setEnd = settings.end();
	while (setIt != setEnd)
	{
		infix << (*setIt)->activeOption();
		++setIt;
	}
	EffectCompiler::fxoInfix( infix.str() );
}

/**
 *	Sets up registered macro effect instances.
 */	
void EffectMacroSetting::setupSettings()
{
	BW_GUARD;
	EffectMacroSetting::MacroSettingVector& settings = static_settings();
	MacroSettingVector::iterator setIt  = settings.begin();
	MacroSettingVector::iterator setEnd = settings.end();
	while (setIt != setEnd)
	{
		(*setIt)->setupFunc_(**setIt);
		GraphicsSetting::add(*setIt);
		(*setIt)->onOptionSelected((*setIt)->activeOption());
		++setIt;
	}

	// Sort by name, so that the order is deterministic rather than 
	// (un)defined by static init order.
	std::sort( settings.begin(), settings.end(), settingsPred );
}

/**
 *	Cleans up macro settings (generally called on shutdown).
 */	
void EffectMacroSetting::finiSettings()
{
	bw_safe_delete( s_pSettings );
}

/**
 *	Accessor for macro settings.
 */	
void EffectMacroSetting::getMacroSettings( MacroSettingVector& settings )
{
	BW_GUARD;
	settings = static_settings();
}

} // namespace Moo

BW_END_NAMESPACE

// efffect_macro_setting.cpp
