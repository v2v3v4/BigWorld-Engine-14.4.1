#ifndef PAGE_TERRAIN_IMPORT_HPP
#define PAGE_TERRAIN_IMPORT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "guitabs/guitabs_content.hpp"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/slider.hpp"
#include "controls/range_slider_ctrl.hpp"
#include "controls/dialog_toolbar.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/image_control.hpp"

BW_BEGIN_NAMESPACE

class PageTerrainImport : public CFormView, public GUITABS::Content
{
public:
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_IMPORT/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_IMPORT/LONG_NAME"), 298, 390, NULL )

public:
    enum { IDD = IDD_PAGE_TERRAIN_IMPORT };

	PageTerrainImport();
	/*virtual*/ ~PageTerrainImport();

    void setQueryHeightRange(float minv, float maxv);

    static PageTerrainImport *instance();

protected:
	void setImportFile(char const *filename);

	void InitPage();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
    afx_msg void OnBrowseBtn();
    afx_msg void OnEditMinHeight();
    afx_msg void OnEditMaxHeight();
    afx_msg void OnEditHeightStrength();
    afx_msg void OnHeightModeSel();
	afx_msg void OnEditLayerStrength();
    afx_msg void OnAnticlockwiseBtn();
    afx_msg void OnClockwiseBtn    ();
    afx_msg void OnFlipXBtn        ();
    afx_msg void OnFlipYBtn        ();
    afx_msg void OnFlipHeightBtn   ();
    afx_msg void OnRecalcClrBtn    ();
    afx_msg void OnAnticlockwiseBtnEnable(CCmdUI *cmdui);
    afx_msg void OnClockwiseBtnEnable    (CCmdUI *cmdui);
    afx_msg void OnFlipXBtnEnable        (CCmdUI *cmdui);
    afx_msg void OnFlipYBtnEnable        (CCmdUI *cmdui);
    afx_msg void OnFlipHeightBtnEnable   (CCmdUI *cmdui);
    afx_msg void OnRecalcClrBtnEnable    (CCmdUI *cmdui);
    afx_msg void OnPlaceBtn();
    afx_msg void OnCancelImportBtn();
    afx_msg void OnExportBtn();
    afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT *result);
	afx_msg void UpdateState();
	afx_msg void OnAbsoluteBtn();
	afx_msg void OnEditMinHeightKillFocus();
	afx_msg void OnEditMaxHeightKillFocus();
	afx_msg void OnEditHeightStrengthKillFocus();
    DECLARE_MESSAGE_MAP()

    DECLARE_AUTO_TOOLTIP(PageTerrainImport, CFormView)

    LRESULT OnHeightSliderChanged(WPARAM wParam, LPARAM lParam);
    void OnHeightStrengthSlider(bool endTrack); 

    void updateHMImportOptions();

private:
	bool                           pageReady_;
    CStatic                        importGrp_;
    CStatic                        fileStatic_;
    CEdit                          importFileEdit_;
    CButton                        importButton_;
    CButton                        exportBtn_;
    controls::EditNumeric          minHeightEdit_;
	controls::EditNumeric          maxHeightEdit_;
    controls::RangeLimitSliderCtrl heightSlider_;    
    controls::EditNumeric          heightStrengthEdit_;
    LimitSlider					   heightStrengthSlider_;
    CComboBox                      modeCB_;
	CButton						   absoluteBtn_;
								   
	controls::DialogToolbar        actionTB_;
    CButton                        placeBtn_;
    CButton                        cancelBtn_; 

    CStatic                        handrawnGrp_;
	controls::Slider               blendSlider_;
								   
    size_t                         filterControls_; 
    bool                           loadedTerrain_;
								   
    static PageTerrainImport       *s_instance_;
};								   


IMPLEMENT_BASIC_CONTENT_FACTORY(PageTerrainImport)

BW_END_NAMESPACE

#endif // PAGE_TERRAIN_IMPORT_HPP
