#pragma once

#include "common/property_table.hpp"

BW_BEGIN_NAMESPACE

class GeneralEditor;
typedef SmartPointer<GeneralEditor> GeneralEditorPtr;


class GraphicsSettingsTable : public PropertyTable
{
public:
	explicit GraphicsSettingsTable( UINT dlgId );
	virtual ~GraphicsSettingsTable();

	bool needsRestart() const { return needsRestart_; }
	virtual void needsRestart( const BW::string& setting );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSelectPropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChangePropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	bool inited_; 
	GeneralEditorPtr editor_;
	bool needsRestart_;
	BW::set<BW::string> changedSettings_;

	bool init();
	void tagNameToString( BW::string& label );
};
BW_END_NAMESPACE

