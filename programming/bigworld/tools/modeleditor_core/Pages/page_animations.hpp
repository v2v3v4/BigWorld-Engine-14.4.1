#pragma once
#include "resource.h"

#include "controls/auto_tooltip.hpp"
#include "guitabs/guitabs_content.hpp"
#include "resmgr/string_provider.hpp"
#include "me_app.hpp"
#include "tree_list.hpp"
#include "tools/modeleditor_core/i_model_editor_app.hpp"
#include "editor_shared/pages/gui_tab_content.hpp"

BW_BEGIN_NAMESPACE

class UalItemInfo;

typedef std::pair < BW::string , BW::string > StringPair;

// PageAnimations
class PageAnimations
	: public TreeList
	, public GuiTabContent
{
	DECLARE_DYNCREATE(PageAnimations)

	IMPLEMENT_ROOT_CONTENT( 
		Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/SHORT_NAME"), 
		Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/LONG_NAME"), 
		285, 833, NULL )
	bool isClonable() { return true; }
	GUITABS::ContentPtr clone()
	{
		GuiTabContent * clone = new PageAnimations;
		clone->setMainFrame( mainFrame_ );
		return clone;
	}

	OnCloseAction onClose( bool isLastContent );

	DECLARE_AUTO_TOOLTIP( PageAnimations, TreeList )

public:
	PageAnimations();
	virtual ~PageAnimations();

	static PageAnimations* currPage();
	
// These are exposed to python as:

	bool isLockedAnim();						// isAnimLocked()
	BW::string animAdd();						// addAnim()
	BW::string animAdd( const BW::vector< BW::string >& fileNames );
	void animPlay();							// playAnim()
	void animStop();							// stopAnim()
	void animLoop();							// loopAnim()
	void animRemove();							// removeAnim()

	bool animPlaying();							// animPlaying()
	bool animLooping();							// animLooping()

	void saveAnimCompression();					// saveAnimComp()
	bool canViewAnimCompression();				// canViewAnimComp()
	bool canSaveAnimCompression();				// canSaveAnimComp()

	bool loadAnim( const BW::string& animName );// loadAnim()
	int  numAnimFrames();						// numAnimFrames()
	bool playAnimFrame( int frameNum );			// playAnimFrame()

// Dialog Data
	enum { IDD = IDD_ANIMATIONS };

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:

	SmartPointer< struct PageAnimationsImpl > pImpl_;
	
	unsigned addNodeTree( DataSectionPtr root, HTREEITEM parent = NULL, float blend = 1.0, unsigned nodeCount = 0 );

	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);
	
	virtual BOOL OnInitDialog();

	void OnSize(UINT nType, int cx, int cy);

	void updateCheck( CButton& button, const BW::string& actionName );
	void disableField( CEdit& field );
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnChangeAnimName();
	afx_msg void updateName();
	afx_msg void OnChangeAnim();
	afx_msg void updateFrameRate();
	afx_msg void updateFirst();
	afx_msg void updateLast();
	afx_msg void updateFrameNum();
	afx_msg void OnTvnSelChangedAnimNodes(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedAnimRemoveBlend();
	afx_msg void OnEnSetfocusAnimFrame();
	afx_msg void OnBnClickedAnimSaveFrameRate();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor ); 

	//Virtual methods which are called when the animation selection is changed or clicked
	void selChange( const StringPair& itemID );
	void selClick( const StringPair& itemID );

	bool addAnims( UalItemInfo* ii );
	const StringPair& getAnim() const {	return mutant()->getAnim( (size_t) this );	}

	void updateBlend();

	// These are the animation compression related methods
	
	void calculateCompressionErrorFactors( float& scale, float& rotation, float& position );
	void setSlidersCompressionErrorFactors( float scale, float rotation, float position );
	
	void currentFrames( int& scale, int& rotation, int& position );
	void updateCompression();

public:
	afx_msg void OnBnClickedAnimCompPosMinus();
	afx_msg void OnBnClickedAnimCompPosPlus();
	afx_msg void OnBnClickedAnimCompRotMinus();
	afx_msg void OnBnClickedAnimCompRotPlus();
	afx_msg void OnBnClickedAnimCompScaleMinus();
	afx_msg void OnBnClickedAnimCompScalePlus();
	afx_msg void OnBnClickedAnimCompPreview();
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageAnimations )

BW_END_NAMESPACE

