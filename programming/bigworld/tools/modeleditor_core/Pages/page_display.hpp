#pragma once
#include "resource.h"

#include "editor_shared/pages/gui_tab_content.hpp"

#include "controls/auto_tooltip.hpp"

#include "resmgr/string_provider.hpp"
#include "tools/modeleditor_core/i_model_editor_app.hpp"

BW_BEGIN_NAMESPACE

class UalItemInfo;

// PageDisplay

class PageDisplay: public CFormView, public GuiTabContent
{
	DECLARE_DYNCREATE(PageDisplay)

	IMPLEMENT_BASIC_CONTENT( 
		Localise(L"MODELEDITOR/PAGES/PAGE_DISPLAY/SHORT_NAME"), 
		Localise(L"MODELEDITOR/PAGES/PAGE_DISPLAY/LONG_NAME"),
		285, 540, NULL )

	DECLARE_AUTO_TOOLTIP( PageDisplay, CFormView )

public:
	PageDisplay();
	virtual ~PageDisplay();

// Dialog Data
	enum { IDD = IDD_DISPLAY };

private:
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:

	SmartPointer< struct PageDisplayImpl > pImpl_;
	
	BOOL OnInitDialog();

	void updateCheck( CButton& button, const BW::string& actionName );
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);

	void OnSize( UINT nType, int cx, int cy );
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	afx_msg void OnBnClickedShowAxes();
	afx_msg void OnBnClickedCheckForSparkles();
	afx_msg void OnBnClickedEnableFog();
	afx_msg void OnBnClickedEnableHDR();
	afx_msg void OnBnClickedEnableSSAO();
	afx_msg void OnBnClickedShowModel();
	afx_msg void OnBnClickedShowWireframe();
	afx_msg void OnBnClickedShowSkeleton();
	afx_msg void OnCbnChangeShadowing();
	afx_msg void OnBnClickedShowBsp();
	afx_msg void OnBnClickedShowBoundingBox();
	afx_msg void OnBnClickedShowVertexNormals();
	afx_msg void OnBnClickedShowVertexBinormals();
	afx_msg void OnBnClickedShowCustomHull();
	afx_msg void OnBnClickedShowPortals();
	afx_msg void OnBnClickedShowHardPoints();

	afx_msg void OnBnClickedGroundModel();
	afx_msg void OnBnClickedCentreModel();

	afx_msg void OnCbnSelchangeDisplayFlora();

	afx_msg void OnBnClickedDisplayChooseBkgColour();
	afx_msg void OnCbnSelchangeDisplayBkg();
	afx_msg void OnBnClickedDisplayChooseFloorTexture();
	afx_msg void OnBnClickedShowEditorProxy();
private:
	void setFloorTexture( const BW::wstring& texture );
	bool floorTextureDrop( UalItemInfo* ii );
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageDisplay )

BW_END_NAMESPACE

