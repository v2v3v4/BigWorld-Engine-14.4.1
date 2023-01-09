#pragma once
#include "resource.h"

#include "controls/range_slider_ctrl.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/auto_tooltip.hpp"

#include "resmgr/string_provider.hpp"

#include "tree_list.hpp"
#include "editor_shared/pages/gui_tab_content.hpp"

BW_BEGIN_NAMESPACE

typedef std::pair < BW::string , BW::string > StringPair;



class SliderContainer : public ReferenceCount
{
public:
	SliderContainer( controls::RangeSliderCtrl* slider, controls::EditNumeric* minEd, controls::EditNumeric* maxEd, int group = 0 );

	controls::RangeSliderCtrl* slider_;
	controls::EditNumeric* minEd_;
	controls::EditNumeric* maxEd_;
	int group_;
};

typedef SmartPointer<SliderContainer> SliderContainerPtr;

// PageActions

class PageActions: public TreeList, public GuiTabContent
{
	DECLARE_DYNCREATE(PageActions)

	IMPLEMENT_BASIC_CONTENT( 
		Localise(L"MODELEDITOR/PAGES/PAGE_ACTIONS/SHORT_NAME"), 
		Localise(L"MODELEDITOR/PAGES/PAGE_ACTIONS/LONG_NAME"), 
		285, 886, NULL )

	DECLARE_AUTO_TOOLTIP( PageActions, TreeList )

public:
	PageActions();
	virtual ~PageActions();
	
	static PageActions* currPage();

	//These are exposed to python as:
	bool isLockedAct();	// isActLocked()
	void actNew();		// newAct()
	void actRemove();	// removeAct()
	void actPromote();	// promoteAct()
	void actDemote();	// demoteAct()
	void actPlay();		// playAct()
	void actStop();		// stopAct()

// Dialog Data
	enum { IDD = IDD_ACTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:

	SmartPointer< struct PageActionsImpl > pImpl_;

	BW::vector<BW::string*> data_;

	void clearData();
	virtual void selClick( const StringPair& );
	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

	const StringPair& getAct() const	{	return mutant()->getAct();	}
	SliderContainerPtr find( void* ptr );
	void writeEditNum( controls::EditNumeric* edit, float num, int group = 0 );
	void sliderFromEdits( SliderContainerPtr slider );
	LRESULT OnRangeSliderChanged(WPARAM wParam, LPARAM lParam);
	LRESULT OnRangeSliderTrack(WPARAM wParam, LPARAM lParam);
	
	void initMatcherField( controls::EditNumeric& field, int dec = 0, float min = 0.f, float max = 100.f, bool allowNeg = false );
		
	virtual BOOL OnInitDialog();

	BW::wstring getNamedCaps( const BW::string& capsStr );
	BW::map< int, BW::wstring > capsNames_;

	void OnSize(UINT nType, int cx, int cy);

	void updateCheck( CButton& button, const StringPair& actID, const BW::string& flagName );
	void disableCheck( CButton& button );
	void updateScalePlaybackSpeed();
	void updateMatchField( controls::EditNumeric& field, const StringPair& actID, const BW::string& typeName, const BW::string& flagName, bool lockedParents = false, bool canBeNeg = true );
	void disableField( CEdit& field );
	void removeMRU( const StringPair& action );
	void addMRU( const StringPair& action );
	afx_msg void OnNMClickMRU(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnChangeActName();
	afx_msg void updateName();
	afx_msg void OnChangeActAnim();
	void updateFlags();
	afx_msg void updateTrack();
	BW::string getFieldText( CEdit& field );
	void updateTriggerFlags();
	void updateTriggerFlags( bool& shouldUndo, bool reloadModel = false );
	void updateCancelFlags();
	void updateCancelFlags( bool& shouldUndo, bool reloadModel = false );
	void updateMatchFlags();
	afx_msg void OnBnClickedActOwnTrack();
	afx_msg void OnBnClickedActTriggerCapsOnSel();
	afx_msg void OnBnClickedActTriggerCapsOffSel();
	afx_msg void OnBnClickedActCancelCapsOnSel();
	afx_msg void OnBnClickedActCancelCapsOffSel();
	afx_msg void OnBnClickedActTriggerFalling();
	afx_msg void OnBnClickedActCancelFalling();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageActions )

BW_END_NAMESPACE

