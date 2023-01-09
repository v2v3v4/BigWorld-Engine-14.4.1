#pragma once

#include "math/rectt.hpp"
#include "common/property_table.hpp"
#include "gizmo/general_properties.hpp"
#include "math/colour.hpp"
#include "data_section_proxy.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class sub-classes the PropertyTable and provides specific
 *	property instantiation for weather systems.
 */
class WeatherSettingsTable : public PropertyTable
{
public:
	explicit WeatherSettingsTable( UINT dlgId );
	virtual ~WeatherSettingsTable();

protected:
	bool init( DataSectionPtr pSection, bool reset );
	void initDragDrop();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSelectPropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChangePropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:	
	DataSectionPtr pSystem_;
	GeneralEditorPtr editor_;
	void onSettingChanged();
	RectInt	dropTest( UalItemInfo* ii );
	bool	doDrop( UalItemInfo* ii );
	void tagNameToString( BW::string& label );
};
BW_END_NAMESPACE

