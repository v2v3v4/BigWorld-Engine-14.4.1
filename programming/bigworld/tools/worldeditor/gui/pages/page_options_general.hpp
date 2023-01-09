#ifndef PAGE_OPTIONS_GENERAL_HPP
#define PAGE_OPTIONS_GENERAL_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "controls/auto_tooltip.hpp"
#include "controls/slider.hpp"
#include "controls/edit_numeric.hpp"
#include "common/graphics_settings_table.hpp"
#include "guitabs/guitabs_content.hpp"
#include "common/girth.hpp"
#include <afxwin.h>
#include <afxcmn.h>
#include "cstdmf/bw_vector.hpp"

BW_BEGIN_NAMESPACE

class OptionsShowTree : public CTreeCtrl
{
	DECLARE_DYNAMIC(OptionsShowTree)

public:
	OptionsShowTree();
	virtual ~OptionsShowTree();

	void populate( DataSectionPtr data, HTREEITEM item = NULL );
	void execute( HTREEITEM item );
	void update();

private:
	void update(HTREEITEM item, BW::string name);

	BW::vector< BW::string* > itemData_;

	CPoint mousePoint_;

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
};


// PageOptionsGeneral


class PageOptionsGeneral : public GraphicsSettingsTable, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_GENERAL/SHORT_NAME"),
		Localise(L"WORLDEDITOR/GUI/PAGE_OPTIONS_GENERAL/LONG_NAME"), 267, 405, NULL )

public:
	PageOptionsGeneral();
	virtual ~PageOptionsGeneral();

	/*virtual*/ void needsRestart( const BW::string& setting );

// Dialog Data
	enum { IDD = IDD_PAGE_OPTIONS_GENERAL };

private:
	CStatic messageText_;

	bool pageReady_;
	void updateSliderEdits();

	bool dontUpdateFarClipEdit_;
	bool farClipEdited_;

	Girths girths_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_AUTO_TOOLTIP( PageOptionsGeneral, GraphicsSettingsTable )
	DECLARE_MESSAGE_MAP()

public:
	void InitPage();

	OptionsShowTree showTree_;
	CButton standardLighting_;
	CButton dynamicLighting_;
	CButton specularLighting_;
	CButton navVisible_;
	CComboBox navGirth_;
	controls::Slider farPlaneSlider_;
	controls::EditNumeric farPlaneEdit_;
	CButton readOnlyMode_;

	int lastLightsType_;

	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
	afx_msg void OnBnClickedOptionsLightingStandard();
	afx_msg void OnBnClickedOptionsLightingDynamic();
	afx_msg void OnBnClickedOptionsLightingSpecular();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnChangeEditNumeric(WPARAM mParam, LPARAM lParam);
	afx_msg void OnBnClickedOptionsRememberSelectionFilter();
	afx_msg void OnBnClickedReadOnlyMode();
	afx_msg void OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void OnEnChangeOptionsFarplaneEdit();
	afx_msg void OnBnClickedNavMeshVisible();
	afx_msg void OnCbNavMeshGirthChange();
	afx_msg void OnSize( UINT nType, int cx, int cy );

private:
	void onSizeInternal();
};


IMPLEMENT_BASIC_CONTENT_FACTORY( PageOptionsGeneral )

BW_END_NAMESPACE

#endif // PAGE_OPTIONS_GENERAL_HPP

