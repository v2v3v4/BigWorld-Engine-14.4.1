#pragma once

#include "afxwin.h"
#include "controls/edit_numeric.hpp"
#include "gui/gui_utilities.hpp"
#include "controls/auto_tooltip.hpp"

BW_BEGIN_NAMESPACE

class ParticleSystem;

// PsProperties form view

class PsProperties : public CFormView
{
	DECLARE_DYNCREATE(PsProperties)

public:
    enum { IDD = IDD_PS_PROPERTIES };

	PsProperties();  

	/*virtual*/ ~PsProperties();

	virtual void DoDataExchange(CDataExchange* pDX); 

	afx_msg LRESULT OnUpdatePsProperties(WPARAM mParam, LPARAM lParam);

    ParticleSystemPtr action();

	void SetParameters(SetOperation task);

	void OnUpdatePsProperties();

	virtual void OnInitialUpdate();

	afx_msg void OnBnClickedPsButton();    

    DECLARE_MESSAGE_MAP()

    DECLARE_AUTO_TOOLTIP(PsProperties, CFormView)

private:
	bool                initialised_;
    CStatic             nameInvalidMessage_;
	controls::EditNumeric        capacity_;
	controls::EditNumeric        windFactor_;
	controls::EditNumeric        maxLod_;
    CToolTipCtrl        tooltips_;
};


BW_END_NAMESPACE

