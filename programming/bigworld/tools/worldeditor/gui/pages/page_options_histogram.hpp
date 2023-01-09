#ifndef PAGE_OPTIONS_HISTOGRAM_HPP
#define PAGE_OPTIONS_HISTOGRAM_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/slider.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxwin.h>
#include <afxcmn.h>
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class PageOptionsHistogram : public CDialog, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_HISTOGRAM/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_HISTOGRAM/LONG_NAME"), 290, 250, NULL )

public:
	PageOptionsHistogram();
	virtual ~PageOptionsHistogram();

// Dialog Data
	enum { IDD = IDD_PAGE_OPTIONS_HISTOGRAM };

protected:
	DECLARE_AUTO_TOOLTIP(PageOptionsHistogram,CDialog);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();

	afx_msg void OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDrawItem( int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct );
	afx_msg void OnBnClickedRed();
	afx_msg void OnBnClickedGreen();
	afx_msg void OnBnClickedBlue();
	afx_msg void OnNMCustomdrawRangeratioslider(NMHDR *pNMHDR, LRESULT *pResult);

private:
	CButton mRed;
	CButton mGreen;
	CButton mBlue;
	CStatic mRangeRatio;
	controls::Slider mRangeRatioSlider;

	bool userAdded_;
	int width_;
	int height_;
	int lumHeight_;
	int rgbLabelPos_;
	int rgbPos_;
	int rgbHeight_;
	int buttonsPos_;
	int redBtnPos_;
	int greenBtnPos_;
	int blueBtnPos_;
	int sliderPos_;
	int sliderWidth_;

	BYTE * pBmpBits_;
	int lastBmpBitsSize_;
};


IMPLEMENT_CDIALOG_CONTENT_FACTORY( PageOptionsHistogram, PageOptionsHistogram::IDD )

BW_END_NAMESPACE

#endif // PAGE_OPTIONS_HISTOGRAM_HPP
