#pragma once
#include "resource.h"

#include "editor_shared/pages/gui_tab_content.hpp"

#include "controls/auto_tooltip.hpp"

#include "common/property_table.hpp"

#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

class UalItemInfo;

// PageObject

class PageObject: public PropertyTable, public GuiTabContent
{
	DECLARE_DYNCREATE(PageObject)

	IMPLEMENT_BASIC_CONTENT( 
		Localise(L"MODELEDITOR/PAGES/PAGE_OBJECT/SHORT_NAME"), 
		Localise(L"MODELEDITOR/PAGES/PAGE_OBJECT/LONG_NAME"), 
		285, 575, NULL )

	DECLARE_AUTO_TOOLTIP( PageObject, PropertyTable )

public:
	PageObject();
	virtual ~PageObject();

	static PageObject* currPage();

// Dialog Data
	enum { IDD = IDD_OBJECT };

protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
public:
	
	virtual BOOL OnInitDialog();
	
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	void OnUpdateThumbnail();
	void OnUpdateList();

private:

	SmartPointer< struct PageObjectImpl > pImpl_;

	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

	void OnSize( UINT nType, int cx, int cy );

	bool changeEditorProxy( const BW::wstring& editorProxyFile );
	bool changeEditorProxyDrop( UalItemInfo* ii ) ;

public:
	afx_msg LRESULT OnChangePropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCbnSelchangeObjectKind();
	afx_msg void OnBnClickedObjectProxySel();
	afx_msg void OnBnClickedObjectProxyRemove();
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageObject )
BW_END_NAMESPACE

