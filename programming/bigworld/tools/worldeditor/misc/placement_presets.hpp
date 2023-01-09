#ifndef PLACEMENT_PRESETS_HPP
#define PLACEMENT_PRESETS_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "guimanager/gui_functor_cpp.hpp"

BW_BEGIN_NAMESPACE

class PlacementPresetsCallback
{
public:
	virtual void currentPresetChanged( const BW::string& presetName ) = 0;
};

class PlacementPresets : 
	GUI::ActionMaker<PlacementPresets>,
	GUI::UpdaterMaker<PlacementPresets>,
	GUI::ActionMaker<PlacementPresets, 1>
{
public:
	enum DataType {
		ROTATION,
		SCALE
	};
	enum Axis {
		X_AXIS,
		Y_AXIS,
		Z_AXIS
	};
	enum Value {
		MIN,
		MAX
	};
	static PlacementPresets* instance();
	static void fini();

	void init( const BW::string& placementXML );

	void readPresets();

	BW::string xmlFilePath() { return placementXML_; };

	DataSectionPtr rootSection() { return section_; };
	DataSectionPtr presetsSection() { return presetsSection_; };

	DataSectionPtr getCurrentPresetData();
	float getCurrentPresetData( DataType type, Axis axis, Value value );
	bool isCurrentDataUniform( DataType type );
	DataSectionPtr getPresetData( const BW::string& presetName );
	
	void addComboBox( CComboBox* cbox );
	void removeComboBox( CComboBox* cbox );

	BW::string currentPreset();
	void currentPreset( const BW::string& preset );
	void currentPreset( CComboBox* cbox );
	BW::string defaultPreset();
	bool defaultPresetCurrent();
	bool currentPresetStock();
	void presetNames(BW::vector<BW::wstring> &names) const;

	void dialog( PlacementPresetsCallback* dialog ) { dialog_ = dialog; };

	void deleteCurrentPreset();
	void save();
	BW::string getNewPresetName();

	void showDialog();

private:
	PlacementPresets();
	static PlacementPresets* instance_;

	BW::vector<CComboBox*> comboBoxes_;

	BW::string currentPreset_;
	PlacementPresetsCallback* dialog_;
	DataSectionPtr section_;
	DataSectionPtr presetsSection_;
	BW::string placementXML_;

	bool presetChanged( GUI::ItemPtr item );
	unsigned int presetUpdate( GUI::ItemPtr item );
	bool showDialog( GUI::ItemPtr item );
};

BW_END_NAMESPACE

#endif // PLACEMENT_PRESETS_HPP
