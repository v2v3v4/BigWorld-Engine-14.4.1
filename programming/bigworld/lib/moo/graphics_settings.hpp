#ifndef GRAPHICS_SETTINGS_REGISTRY_HPP
#define GRAPHICS_SETTINGS_REGISTRY_HPP

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_map.hpp"

#include "graphics_settings_picker.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations
typedef SmartPointer<class DataSection> DataSectionPtr;

namespace Moo
{

/**
 *	Represents a set of graphics options in the game. Each GraphicsSetting 
 *	object offers a number of options. There is always one and only one 
 *	active option at any one time. This class is abstract and should be 
 *	derived by classes that want to expose some form of graphics settings 
 *	option. For examples on how to use this class, see moo/managed_effect.hpp 
 *	or the CallbackGraphicsSetting class.
 *
 *	An option can be selected via the selectOption method. Calling this 
 *	method triggers the onOptionSelected virtual function (unless the setting
 *	is flagged as delayed). Client systems must always override this function. 
 *	GraphicsSetting objects should be registered with GraphicsSettings::add().
 *	They can be listed with GraphicsSettings::settings().
 *
 *	Settings flagged with &lt;delayed> do not get their activeOption property and
 *	onOptionSelected virtual method triggered when their active option change. 
 *	Instead, the call is delayed until GraphicsSettings::commitPending() is 
 *	called. Settings should set this flag in two cases: (1) for closely related 
 *	settings that can gain from being processed as a batch (see 
 *	texture_manager.cpp for an example) and (2) for settings that may take a 
 *	while to process. This gives the interface programmer a chance to warn the 
 *	user that processing the new settings may take a while to complete before 
 *	the client application blocks for a few seconds (currently, there is no 
 *	support for displaying a progress bar while the settings are being 
 *	processed).
 *
 *	Settings flagged with &lt;needsRestart>, when their active option change,
 *	will set the global needsRestart flag in GraphicsSetting. Settings that
 *	require the client application to be restarted to come into effect should 
 *	set this flag to true (see managed_effect for an example). The global 
 *	needsRestart flag can be queried with GraphicsSetting::needsRestart().
 */
class GraphicsSetting : public ReferenceCount
{
public:
	typedef std::pair<BW::string, uint32> Setting;
	typedef BW::vector<Setting> SettingsVector;
public:

	//-- Note (b_sviglo): Describes each individual option parameters.
	//-- label	   - the name of the option.
	//-- desc	   - the description of the option.
	//-- supported - is this option supported by the user system?
	//-- advanced  - is this option available only in the advanced renderer pipeline?
	struct Option
	{
		Option(const BW::string& label, const BW::string& desc, bool supported, bool advanced)
			: m_label(label), m_desc(desc), m_supported(supported), m_advanced(advanced) { }

		bool operator == (const Option& rht) const
		{
			return m_label == rht.m_label && m_desc == rht.m_desc &&
				m_supported == rht.m_supported && m_advanced == rht.m_advanced
				; 
		}

		BW::string m_label;
		BW::string m_desc;
		bool		m_supported;
		bool		m_advanced;
	};

	typedef BW::vector<Option>				Options;
	typedef SmartPointer<GraphicsSetting> GraphicsSettingPtr;
	typedef BW::vector<GraphicsSettingPtr> GraphicsSettingVector;

	//-- Note (b_sviglo): Setting marked as advanced controls set of availble options in the all other
	//--				  graphics settings.
	GraphicsSetting(
		const BW::string & label, 
		const BW::string & desc,
		int activeOption, 
		bool delayed,
		bool needsRestart,
		bool advanced = false);

	virtual ~GraphicsSetting();

	const BW::string&	label() const			{	return label_;	}
	const BW::string&	desc() const			{	return desc_;	}
	bool				advanced() const		{	return advanced_;	}
	bool				requiresRestart() const	{	return needsRestart_;	}
	bool				delayed() const			{	return delayed_;	}

	int addOption(const BW::string & label, const BW::string & desc, bool isSupported, bool isAdvanced = false);
	const GraphicsSetting::Options & options() const;

	bool selectOption(int optionIndex, bool force=false);
	virtual void onOptionSelected(int optionIndex) = 0;

	int activeOption() const;

	void needsRestart( bool a_needsRestart );

	void addSlave(GraphicsSettingPtr slave);

	static bool init(DataSectionPtr section, 
		int overrideAutoDetectSettingsGroup = GraphicsSettingsAutoDetector::NO_OVERRIDE);
	static void write(DataSectionPtr section);

	static void add(GraphicsSettingPtr option);
	static const GraphicsSettingVector & settings();

	static bool hasPending();
	static bool isPending(GraphicsSettingPtr setting, int & o_pendingOption);
	static void commitPending();
	static void rollbackPending();

	static bool needsRestart();

	static GraphicsSettingPtr getFromLabel(
			const BW::StringRef & settingDesc);
	static void revertSettings();
	static bool wereForcedToReset();

	static void finiSettingsData();

	// static
	typedef std::pair<GraphicsSettingPtr, int> SettingIndexPair;
	typedef BW::vector<SettingIndexPair> SettingIndexVector;
	typedef BW::map<BW::string, int> StringIntMap;

	struct GraphicsSettingsData
	{
		GraphicsSettingsData()
			: s_needsRestart( false )
			, s_advanced( false )
			, s_were_forced_to_reset( false )
		{

		}

		GraphicsSetting::GraphicsSettingVector  s_settings;
		GraphicsSetting::SettingIndexVector     s_pending;
		GraphicsSetting::StringIntMap           s_latentSettings;
		bool									s_needsRestart;
		bool					 				s_advanced;
		BW::string								s_filename;
		SettingsVector		 					s_oldSettings;
		bool					 				s_were_forced_to_reset;
	};

private:
	static void commitSettings(SettingsVector& _settings);
	bool selectOptionPriv(int optionIndex, bool force=false);

	BW::string             label_;
	BW::string				desc_;
	Options					options_;
	int                     activeOption_;
	bool                    delayed_;
	bool                    needsRestart_;
	bool					advanced_;
	GraphicsSettingVector   slaves_;

	static bool addPending(GraphicsSettingPtr setting, int optionIndex);

	static struct GraphicsSettingsData & getSettingsData();

};

/**
 *	Implement the GraphicsSetting class, providing a member function 
 *	callback hook that is called whenever the active option changes. 
 *	This class provides the basic graphics settings functionality that 
 *	is needed by most client subsystems (see romp/flora.cpp for an 
 *	example).
 */
template<typename ClassT>
class CallbackGraphicsSetting : public GraphicsSetting
{
public:
	typedef void (ClassT::*Function)(int);

	CallbackGraphicsSetting(
			const BW::string & label, 
			const BW::string & desc, 
			ClassT            & instance, 
			Function            function,
			int                 activeOption,
			bool                delayed,
			bool                needsRestart) :
		GraphicsSetting(label, desc, activeOption, delayed, needsRestart),
		instance_(instance),
		function_(function)
	{}

	virtual void onOptionSelected(int optionIndex)
	{
		((&instance_)->*function_)(optionIndex);
	}

	ClassT & instance_;
	Function function_;
};

/**
 *	Helper template function used to create a CallbackGraphicsSetting object.
 *	Use this template function to avoid having to provide the type parameters
 *	required to instantiate a CallbackGraphicsSetting object.
 */
template<typename ClassT>
GraphicsSetting::GraphicsSettingPtr 
	makeCallbackGraphicsSetting(
		const BW::string & label, 
		const BW::string & desc, 
		ClassT & instance, 
		void(ClassT::*function)(int),
		int activeOption,
		bool delayed,
		bool needsRestart)
{
	BW_GUARD;
	return GraphicsSetting::GraphicsSettingPtr(
		new CallbackGraphicsSetting<ClassT>(
			label, desc, instance, function, 
			activeOption, delayed, needsRestart));
}

/**
 *	Implement the GraphicsSetting class, providing a static function 
 *	callback hook that is called whenever the active option changes. 
 *	This class provides the basic graphics settings functionality that 
 *	is needed by most clientsubsystems (see romp/foot_print_renderer.cpp 
 *	for an example).
 */
class StaticCallbackGraphicsSetting : public GraphicsSetting
{
public:
	typedef void (*Function)(int);

	StaticCallbackGraphicsSetting(
			const BW::string & label, 
			const BW::string & desc, 
			Function            function,
			int                 activeOption,
			bool                delayed,
			bool                needsRestart) :
		GraphicsSetting(label, desc, activeOption, delayed, needsRestart),
		function_(function)
	{}

	virtual void onOptionSelected(int optionIndex)
	{
		(*function_)(optionIndex);
	}

	Function function_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // GRAPHICS_SETTINGS_REGISTRY_HPP
