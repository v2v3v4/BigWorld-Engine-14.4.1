#pragma once

#include "afxwin.h"
#include "common/property_table.hpp"
#include "controls/auto_tooltip.hpp"

BW_BEGIN_NAMESPACE

class ParticleSystem;

// MpsProperties form view

class MpsProperties : public PropertyTable
{
	DECLARE_DYNCREATE(MpsProperties)

public:
    enum { IDD = IDD_MPS_PROPERTIES };

	MpsProperties();  

	/*virtual*/ ~MpsProperties();

	virtual void DoDataExchange(CDataExchange* pDX); 

    MetaParticleSystemPtr metaPS();

	virtual void OnInitialUpdate();

	afx_msg LRESULT OnChangePropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
	DECLARE_AUTO_TOOLTIP(MpsProperties, PropertyTable)

private:
	GeneralEditorPtr editor_;
	bool elected_;

	CButton ctlCheckDrawHelperModel_;
	CEdit ctlEditHelperModelName_;
	CComboBox ctlComboHelperModelHardpoint_;
	bool ignoreControlChanges_;

	afx_msg void OnCbClickedDrawHelperModel();
	afx_msg void OnBnClickedChooseHelperModel();
	afx_msg void OnCbnSelchangeCenterOnHardpoint();
};


BW_END_NAMESPACE

