#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"

BW_BEGIN_NAMESPACE

class SinkPSA;

// PsaSinkProperties form view

class PsaSinkProperties : public PsaProperties
{
    DECLARE_DYNCREATE(PsaSinkProperties)

public:
    enum { IDD = IDD_PSA_SINK_PROPERTIES };

    PsaSinkProperties(); 

    /*virtual*/ ~PsaSinkProperties();

    SinkPSA * action();

    void SetParameters(SetOperation task);

protected:
    /*virtual*/ void OnInitialUpdate();

    /*virtual*/ void DoDataExchange(CDataExchange* pDX); 

	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );

    DECLARE_MESSAGE_MAP()

private:
    controls::EditNumeric    maximumAge_;
    controls::EditNumeric    minimumSpeed_;
    CSliderCtrl     maximumAgeSlider_;
    int             prevSliderPos_;
    CButton			outsideOnly_;
	controls::EditNumeric	delay_;

public:
	afx_msg void OnBnClickedPsaSinkOutsideOnly();
};


BW_END_NAMESPACE

