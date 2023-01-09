#ifndef PAGE_OPTIONS_NAVIGATION_HPP
#define PAGE_OPTIONS_NAVIGATION_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/slider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/edit_commit.hpp"
#include "controls/search_field.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxwin.h>
#include <afxcmn.h>
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

class OptionsLocationsTree : public CTreeCtrl
{
	DECLARE_DYNAMIC(OptionsLocationsTree)

public:
	OptionsLocationsTree();
	virtual ~OptionsLocationsTree() {};

private:
	DataSectionPtr locationData_;
	BW::set< BW::string > locations_;
	BW::string search_str_;

protected:
	DECLARE_MESSAGE_MAP()

public:
	void updateLocationsList( const BW::string& spaceName );
	void redrawLocationsList();
	void setSearchString( const BW::string& searchString );

	void doAdd();
	void doRename();
	void doUpdate();
	void doRemove();
	void doMove();

	afx_msg void OnTvnEndlabeleditOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnKeydownOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult);
};


class PageOptionsNavigation : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/LONG_NAME"), 263, 442, NULL )

public:
	PageOptionsNavigation();
	virtual ~PageOptionsNavigation();

// Dialog Data
	enum { IDD = IDD_PAGE_OPTIONS_NAVIGATION };

	void InitPage();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnChangeEditNumeric(WPARAM mParam, LPARAM lParam);
	afx_msg void OnEnChangeOptionsCameraHeightEdit();
	CButton isPlayerPreviewModeEnabled_;
	afx_msg void OnBnClickedPlayerPreviewMode();
	afx_msg void OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void OnBnClickedOptionsPosMove();
	afx_msg void OnBnClickedOptionsChunkMove();
	afx_msg void OnBnClickedOptionsLocationAdd();
	afx_msg void OnBnClickedOptionsLocationRename();
	afx_msg void OnBnClickedOptionsLocationRemove();
	afx_msg void OnBnClickedOptionsLocationMove();
	afx_msg void OnBnClickedOptionsLocationUpdate();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg LRESULT OnSearchFieldChanged( WPARAM wParam, LPARAM lParam );

	DECLARE_AUTO_TOOLTIP_EX( PageOptionsNavigation )
	DECLARE_MESSAGE_MAP()

private:
	bool pageReady_;
	void updateSliderEdits();

	bool dontUpdateHeightEdit_;
	bool cameraHeightEdited_;

	controls::EditNumeric posXEdit_;
	controls::EditNumeric posYEdit_;
	controls::EditNumeric posZEdit_;
	controls::EditCommit  posChunkEdit_;
	OptionsLocationsTree locationList_;
	controls::Slider farPlaneSlider_;
	controls::EditNumeric farPlaneEdit_;
	controls::Slider cameraHeightSlider_;
	controls::EditNumeric cameraHeightEdit_;

	CButton locationRename_;
	CButton locationUpdate_;
	CButton locationRemove_;
	CButton locationMoveTo_;

	Vector3 lastPos_;

	SearchField search_;

	virtual BOOL PreTranslateMessage( MSG* pMsg );

};


IMPLEMENT_BASIC_CONTENT_FACTORY( PageOptionsNavigation )

BW_END_NAMESPACE

#endif // PAGE_OPTIONS_NAVIGATION_HPP
