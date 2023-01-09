#ifndef PAGE_PROJECT_HPP
#define PAGE_PROJECT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/slider.hpp"
#include "controls/auto_tooltip.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxwin.h>

BW_BEGIN_NAMESPACE

class PageProject : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_PROJECT/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_PROJECT/LONG_NAME"), 290, 390, NULL )
	DECLARE_AUTO_TOOLTIP(PageProject, CFormView);

public:
	PageProject();
	virtual ~PageProject();

// Dialog Data
	enum { IDD = IDD_PAGE_PROJECT };

private:
	bool pageReady_;
	void InitPage();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedProjectSelectionLock();
	afx_msg void OnBnClickedProjectCommitAll();
	afx_msg void OnBnClickedProjectDiscardAll();
	afx_msg void OnOverlayModeSel();

	CButton selectionLock_;
	CEdit commitMessage_;
	CButton commitKeepLocks_;
	CButton commitAll_;
	CButton discardKeepLocks_;
	CButton discardAll_;
	controls::Slider blendSlider_;
	CStatic mCalculatedMap;
	afx_msg void OnEnChangeProjectCommitMessage();
	CButton update_;
	CComboBox overlayMode_;

public:
	afx_msg void OnBnClickedProjectUpdate();
};


IMPLEMENT_BASIC_CONTENT_FACTORY( PageProject )

BW_END_NAMESPACE

#endif // PAGE_PROJECT_HPP
