#ifndef PAGE_TERRAIN_RULER_HPP
#define PAGE_TERRAIN_RULER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/gui/pages/page_terrain_base.hpp"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/image_button.hpp"
#include "controls/slider.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxcmn.h>
#include <afxwin.h>

BW_BEGIN_NAMESPACE

// PageTerrainRuler

class PageTerrainRuler : public PageTerrainBase, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( L"Tool: Ruler", L"Tool Options: Ruler", 290, 200, NULL )

public:
	PageTerrainRuler();
	virtual ~PageTerrainRuler();

// Dialog Data
	enum { IDD = IDD_PAGE_TERRAIN_RULER };

private:
	bool pageReady_;
	float worldScale_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_AUTO_TOOLTIP( PageTerrainRuler, PageTerrainBase );
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();

	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedCheckBoxVerticalMode();

	CEdit ctlEditDistance;
	CEdit ctlEditHorzDistance;
	CEdit ctlEditVertDistance;
	CButton ctlCheckBoxVerticalMode;
	controls::Slider ctlRulerWidthSlider;
};

IMPLEMENT_CDIALOG_CONTENT_FACTORY( PageTerrainRuler, PageTerrainRuler::IDD )

BW_END_NAMESPACE

#endif // PAGE_TERRAIN_RULER_HPP
