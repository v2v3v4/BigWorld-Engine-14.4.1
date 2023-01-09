#ifndef PSA_TINT_SHADER_PROPERTIES_HPP
#define PSA_TINT_SHADER_PROPERTIES_HPP

#include "afxwin.h"
#include "fwd.hpp"
#include "resource.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/color_timeline.hpp"
#include "controls/color_picker.hpp"

BW_BEGIN_NAMESPACE

class PsaTintShaderProperties : public PsaProperties
{
    DECLARE_DYNCREATE(PsaTintShaderProperties)

public:
    enum { IDD = IDD_PSA_TINT_SHADER_PROPERTIES };

    PsaTintShaderProperties(); 

    /*virtual*/ ~PsaTintShaderProperties();

    /*virtual*/ void OnInitialUpdate();

    TintShaderPSA *action();

    void SetParameters(SetOperation task);  

protected:
    /*virtual*/ void DoDataExchange(CDataExchange* pDX);

    afx_msg void OnBnLoop();

    afx_msg LRESULT OnColorPickerSelMove(WPARAM mParam, LPARAM lParam);

    afx_msg LRESULT OnColorPickerSelUp(WPARAM mParam, LPARAM lParam);

    afx_msg LRESULT OnUpdateTints(WPARAM mParam, LPARAM lParam);

    afx_msg LRESULT OnAddedTint(WPARAM mParam, LPARAM lParam);

    afx_msg LRESULT OnNewTimelineSel(WPARAM mParam, LPARAM lParam);

    afx_msg LRESULT OnEditClrText(WPARAM mParam, LPARAM lParam);    

    afx_msg void OnBnDeleteTint();

    afx_msg void OnBnAddNewTint();

	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );

    DECLARE_MESSAGE_MAP()

private:
    controls::ColorTimeline colorTimelineWnd_;
    controls::ColorPicker   colorPicker_;
    controls::EditNumeric	period_;
    CButton                 repeat_;    
    controls::EditNumeric	fogAmount_;
    CSliderCtrl				fogAmountSlider_;
    controls::EditNumeric	pickerRed_;
    controls::EditNumeric	pickerGreen_;
    controls::EditNumeric	pickerBlue_;
    controls::EditNumeric	pickerAlpha_;
    CButton                 addTintButton_;
    size_t                  filterChange_;
	controls::EditNumeric	delay_;
};

BW_END_NAMESPACE

#endif // PSA_TINT_SHADER_PROPERTIES_HPP
