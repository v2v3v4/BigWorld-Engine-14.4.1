#ifndef PAGE_FENCES
#define PAGE_FENCES

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/gui/controls/limit_slider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/image_button.hpp"
#include "controls/slider.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxcmn.h>
#include <afxwin.h>

BW_BEGIN_NAMESPACE

// PageFences

class PageFences : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( L"Tool: Fences", L"Tool Options: Fences", 290, 200, NULL )

public:
	PageFences();
	virtual ~PageFences();

// Dialog Data
	enum { IDD = IDD_PAGE_TERRAIN_FENCES };

	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedSelectionToFenceModelName();
	afx_msg void OnBnClickedStartNewSequence();
	afx_msg void OnCbClickedSelectWholeFences();
	afx_msg void OnCbClickedAlignToTerrainNormal();
	afx_msg void OnEditChangedFenceStep();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_AUTO_TOOLTIP( PageFences, CFormView );
	DECLARE_MESSAGE_MAP()

private:
	bool pageReady_;
	void InitPage();

	CEdit ctlEditFenceModelName;
	CEdit ctlEditFenceStep;
	CButton ctlCbSelectWholeFences;
	CButton ctlCbAlignToTerrainNormal;
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageFences )

BW_END_NAMESPACE

#endif // PAGE_FENCES
