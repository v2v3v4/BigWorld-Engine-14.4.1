#ifndef EFFECT_MACRO_SETTING_HPP
#define EFFECT_MACRO_SETTING_HPP

#include "graphics_settings.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 * @internal
 *	Used internally to register settings that work by redefining effect files 
 *	compile time macros. This settings require the client application to be 
 *	restarted form them to come into effect.
 *
 *	Note: EffectMacroSetting instances need to be static to make sure it gets 
 *	instantiated before the EffectManager is initialised). For an example on
 *	how to use EffectMacroSetting, see romp/sky_light_map.cpp.
 */
class EffectMacroSetting : public GraphicsSetting
{
	friend class EffectManager;

public:
	typedef SmartPointer<EffectMacroSetting> EffectMacroSettingPtr;
	typedef BW::vector<EffectMacroSettingPtr> MacroSettingVector;
	typedef BW::vector<BW::string> StringVector;
	typedef void(*SetupFunc)(EffectMacroSetting &);

	EffectMacroSetting(
		const BW::string & label, 
		const BW::string & desc, 
		const BW::string & Macro, 
		SetupFunc setupFunc,
		bool advanced = false);

	void addOption(
		const BW::string & label, 
		const BW::string & desc, 
		             bool   isSupported, 
		const BW::string & value);

	virtual void onOptionSelected( int optionIndex );

	size_t numMacroOptions() const { return values_.size(); }
	BW::string macroName() const { return macro_; }
	void macroValues(StringVector& ret) const { ret = values_; }

	static void setupSettings();
	static void getMacroSettings(MacroSettingVector& settings);
	static void updateManagerInfix();
	static void finiSettings();

private:	
	static void add( EffectMacroSettingPtr setting );

	BW::string macro_;
	SetupFunc   setupFunc_;

	StringVector values_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // EFFECT_MACRO_SETTING_HPP
