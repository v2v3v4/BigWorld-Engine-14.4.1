#ifndef PAGE_TERRAIN_FILTER_HPP
#define PAGE_TERRAIN_FILTER_HPP


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

class PageTerrainFilter : public PageTerrainBase, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_FILTER/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_TERRAIN_FILTER/LONG_NAME"),290, 280, NULL )

	int lastFilterIndex_;
public:
	PageTerrainFilter();
	virtual ~PageTerrainFilter();

// Dialog Data
	enum { IDD = IDD_PAGE_TERRAIN_FILTER };

private:
	bool pageReady_;
	void updateSliderEdits();
	void loadFilters();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_AUTO_TOOLTIP_EX( PageTerrainFilter );
	DECLARE_MESSAGE_MAP()
public:
	LimitSlider sizeSlider_;
    controls::EditNumeric sizeEdit_;
	CButton falloffLinear_;
	CButton falloffCurve_;
	CButton falloffFlat_;
	CListBox filtersList_;

	virtual BOOL OnInitDialog();
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLbnSelchangeTerrainFiltersList();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnEnChangeTerrainSizeEdit();
	afx_msg void OnBnClickedSizerange();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );

	controls::ImageButton mSizeRange;
};


IMPLEMENT_CDIALOG_CONTENT_FACTORY( PageTerrainFilter, PageTerrainFilter::IDD )

BW_END_NAMESPACE

#endif // PAGE_TERRAIN_FILTER_HPP
