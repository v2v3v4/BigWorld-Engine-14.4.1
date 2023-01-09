#pragma once
#include "resource.h"

#include "editor_shared/pages/gui_tab_content.hpp"

#include "controls/auto_tooltip.hpp"

#include "resmgr/string_provider.hpp"
#include "tools/modeleditor_core/i_model_editor_app.hpp"

BW_BEGIN_NAMESPACE

class UalItemInfo;

// PageLOD

class PageLOD: public CFormView, public GuiTabContent
{
	DECLARE_DYNCREATE(PageLOD)

	IMPLEMENT_BASIC_CONTENT( 
		Localise(L"MODELEDITOR/PAGES/PAGE_LOD/SHORT_NAME"), 
		Localise(L"MODELEDITOR/PAGES/PAGE_LOD/LONG_NAME"),
		285, 350, NULL )

	DECLARE_AUTO_TOOLTIP( PageLOD, CFormView )

public:
	PageLOD();
	virtual ~PageLOD();

	static PageLOD* currPage();
	
	//These are exposed to python as:
	void lodNew();				// newLod()
	void lodChangeModel();		// changeLodModel()
	void lodRemove();			// removeLod()
	void lodMoveUp();			// moveLodUp()
	void lodMoveDown();			// moveLodDown()
	void lodSetToDist();		// setLodToDist()
	void lodExtendForever();	// extendLodForever()

	bool lodIsSelected();		// lodSelected()
	bool lodIsLocked();			// isLockedLod()
	bool lodIsFirst();			// isFirstLod()
	bool lodCanMoveUp();		// canMoveLodUp()
	bool lodCanMoveDown();		// canMoveLodDown()
	bool lodIsMissing();		// isMissingLod()

// Dialog Data
	enum { IDD = IDD_LOD };

protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
public:
	
	virtual BOOL OnInitDialog();

	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	bool locked() const;
	void locked( bool locked );

private:

	SmartPointer< struct PageLODImpl > pImpl_;

	BW::vector<BW::string*> data_;
	void clearData();

	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

	void OnSize( UINT nType, int cx, int cy );

	bool checkModel( const BW::wstring& modelPath );
	
	void addNewModel( const BW::wstring& modelName );
	bool modelDrop( UalItemInfo* ii );
	
	void moveToLodDist( float dist );
	float getLodDist();
		
	void disableField( CEdit& field );
	void enableField( CEdit& field );
	void OnUpdateLODList();

	bool lodIsMissing( int sel );
	
	void setSel( int sel );
	
	afx_msg void OnLvnItemchangedLodList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void updateMin();
	afx_msg void updateMax();
	
	afx_msg void OnBnClickedLodHidden();
	afx_msg void OnBnClickedLodVirtualDist();
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageLOD )
BW_END_NAMESPACE

