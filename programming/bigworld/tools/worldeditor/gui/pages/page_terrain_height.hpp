#ifndef PAGE_TERRAIN_HEIGHT_HPP
#define PAGE_TERRAIN_HEIGHT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/gui/pages/page_terrain_base.hpp"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/image_button.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxcmn.h>
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class PageTerrainHeight : public PageTerrainBase, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_HEIGHT/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_HEIGHT/LONG_NAME"), 290, 280, NULL )

public:
	PageTerrainHeight();
	virtual ~PageTerrainHeight();

// Dialog Data
	enum { IDD = IDD_PAGE_TERRAIN_HEIGHT };

private:
	bool pageReady_;
	void updateSliderEdits();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_AUTO_TOOLTIP( PageTerrainHeight, PageTerrainBase );
	DECLARE_MESSAGE_MAP()
public:

	LimitSlider sizeSlider_;
    controls::EditNumeric sizeEdit_;
	LimitSlider strengthSlider_;
	controls::EditNumeric strengthEdit_;
	LimitSlider heightSlider_;
	controls::EditNumeric heightEdit_;
	CButton falloffLinear_;
	CButton falloffCurve_;
	CButton falloffFlat_;
	CButton falloffAverage_;
	CButton absolute_;
	CButton relative_;
	uint32 filterControls_;

	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedTerrainFalloffLinear();
	afx_msg void OnBnClickedTerrainFalloffCurve();
	afx_msg void OnBnClickedTerrainFalloffFlat();
	afx_msg void OnBnClickedTerrainFalloffAverage();
	afx_msg LRESULT OnChangeEditNumeric(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	typedef PageTerrainHeight This;
	PY_MODULE_STATIC_METHOD_DECLARE( py_setHeight );
	PY_MODULE_STATIC_METHOD_DECLARE( py_setHeightCursor );
	PY_MODULE_STATIC_METHOD_DECLARE( py_unsetHeightCursor );
	afx_msg void OnBnClickedTerrainAbsoluteheight();
	afx_msg void OnBnClickedTerrainRelativeheight();
	controls::ImageButton mSizeRange;
	controls::ImageButton mStrengthRange;
	afx_msg void OnBnClickedSizerange();
	afx_msg void OnBnClickedStrengthrange();
	controls::ImageButton mHeightRange;
	afx_msg void OnBnClickedHeightrange();
	afx_msg void OnEnChangeTerrainSizeEdit();
	afx_msg void OnEnChangeTerrainStrengthEdit();
	afx_msg void OnEnChangeTerrainHeightEdit();
};


IMPLEMENT_CDIALOG_CONTENT_FACTORY( PageTerrainHeight, PageTerrainHeight::IDD )

BW_END_NAMESPACE

#endif // PAGE_TERRAIN_HEIGHT_HPP
