#include "pch.hpp"
#include "page_animations_impl.hpp"
#include "page_animations.hpp"

#include "common/user_messages.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "controls/edit_commit.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/slider.hpp"
#include "controls/user_messages.hpp"
#include "controls/utils.hpp"
#include "tools/common/delay_redraw.hpp"
#include "tools/common/python_adapter.hpp"
#include "tools/common/utilities.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"
#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "me_shell.hpp"
#include "moo/interpolated_animation_channel.hpp"
#include "mru.hpp"
#include "shlwapi.h"
#include "ual/ual_manager.hpp"
#include "undo_redo.hpp"
#include "tools/modeleditor_core/App/me_python_adapter.hpp"
#include "tools/modeleditor_core/GUI/panel_manager.hpp"
#include "editor_shared/gui/i_main_frame.hpp"
#include "editor_shared/cursor/wait_cursor.hpp"

#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

PageAnimations* PageAnimationsImpl::s_currPage = NULL;

// PageAnimations

//ID string required for the tearoff tab manager
const BW::wstring PageAnimations::contentID = L"PageAnimationsID";

IMPLEMENT_DYNCREATE(PageAnimations, TreeList)

PageAnimations::PageAnimations():
	TreeList( PageAnimations::IDD, mutant()->animTree(), "MODELEDITOR/ANIMATIONS" )
	
{
	BW_GUARD;

	pImpl_ = new PageAnimationsImpl( this );
}

PageAnimations::~PageAnimations()
{
	BW_GUARD;

	if ( pImpl_->animSubscriber_ )
		GUI::Manager::instance().remove( pImpl_->animSubscriber_ );
	if ( pImpl_->compAnimSubscriber_ )
		GUI::Manager::instance().remove( pImpl_->compAnimSubscriber_ );
}

/*static*/ PageAnimations* PageAnimations::currPage()
{
	return PageAnimationsImpl::s_currPage;
}

void PageAnimations::DoDataExchange(CDataExchange* pDX)
{
	BW_GUARD;

	TreeList::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_ANIM_NAME, pImpl_->name);
	DDX_Control(pDX, IDC_ANIM_SOURCE, pImpl_->source);
	DDX_Control(pDX, IDC_ANIM_CHANGE_ANIM, pImpl_->change_anim);
	DDX_Control(pDX, IDC_ANIM_FRAME_RATE, pImpl_->frameRate);
	DDX_Control(pDX, IDC_ANIM_FRAME_RATE_SLIDER, pImpl_->frameRateSlider);
	DDX_Control(pDX, IDC_ANIM_FIRST, pImpl_->first);
	DDX_Control(pDX, IDC_ANIM_LAST, pImpl_->last);
	DDX_Control(pDX, IDC_ANIM_FRAME, pImpl_->frameNum);
	DDX_Control(pDX, IDC_ANIM_FRAME_SLIDER, pImpl_->frameNumSlider);
	DDX_Control(pDX, IDC_ANIM_SAVE_FRAME_RATE, pImpl_->frameRateSave);

	DDX_Control(pDX, IDC_ANIM_NODES_BOX, pImpl_->nodeBox);
	DDX_Control(pDX, IDC_ANIM_NODES, pImpl_->nodeTree);
	DDX_Control(pDX, IDC_ANIM_BLEND, pImpl_->blend);
	DDX_Control(pDX, IDC_ANIM_BLEND_TEXT, pImpl_->blendText);
	DDX_Control(pDX, IDC_ANIM_BLEND_SLIDER, pImpl_->blendSlider);
	DDX_Control(pDX, IDC_ANIM_REMOVE_BLEND, pImpl_->blendRemove);

	DDX_Control(pDX, IDC_ANIM_COMP_BOX, pImpl_->compBox);
		
	DDX_Control(pDX, IDC_ANIM_COMP_POS_SLDR, pImpl_->compPosSldr);
	DDX_Control(pDX, IDC_ANIM_COMP_ROT_SLDR, pImpl_->compRotSldr);
	DDX_Control(pDX, IDC_ANIM_COMP_SCALE_SLDR, pImpl_->compScaleSldr);

	DDX_Control(pDX, IDC_ANIM_COMP_POS_MINUS, pImpl_->compPosMinus);
	DDX_Control(pDX, IDC_ANIM_COMP_ROT_MINUS, pImpl_->compRotMinus);
	DDX_Control(pDX, IDC_ANIM_COMP_SCALE_MINUS, pImpl_->compScaleMinus);

	DDX_Control(pDX, IDC_ANIM_COMP_POS_PLUS, pImpl_->compPosPlus);
	DDX_Control(pDX, IDC_ANIM_COMP_ROT_PLUS, pImpl_->compRotPlus);
	DDX_Control(pDX, IDC_ANIM_COMP_SCALE_PLUS, pImpl_->compScalePlus);

	DDX_Control(pDX, IDC_ANIM_COMP_POS, pImpl_->compPos);
	DDX_Control(pDX, IDC_ANIM_COMP_ROT, pImpl_->compRot);
	DDX_Control(pDX, IDC_ANIM_COMP_SCALE, pImpl_->compScale);

	DDX_Control(pDX, IDC_ANIM_POSITION_TEXT, pImpl_->compPosText);
	DDX_Control(pDX, IDC_ANIM_ROTATION_TEXT, pImpl_->compRotText);
	DDX_Control(pDX, IDC_ANIM_SCALE_TEXT, pImpl_->compScaleText);

	DDX_Control(pDX, IDC_ANIM_COMP_TOTAL, pImpl_->compTotal);

	if (pImpl_->ready) return;

	pImpl_->frameRateSlider.SetRangeMin(1); 
	pImpl_->frameRateSlider.SetRangeMax(120);

	pImpl_->frameNumSlider.SetRangeMin(0); 
	pImpl_->frameNumSlider.SetRangeMax(100);

	pImpl_->blendSlider.SetRangeMin(0); 
	pImpl_->blendSlider.SetRangeMax(20);

	pImpl_->compPosSldr.SetRangeMin(0); 
	pImpl_->compPosSldr.SetRangeMax(400);

	pImpl_->compRotSldr.SetRangeMin(0); 
	pImpl_->compRotSldr.SetRangeMax(400);

	pImpl_->compScaleSldr.SetRangeMin(0); 
	pImpl_->compScaleSldr.SetRangeMax(400);

	//------
	
	pImpl_->toolbar.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,0,0), this, 0 );

	pImpl_->animSubscriber_ = new GUI::Toolbar( "AnimToolbar", pImpl_->toolbar );
	GUI::Manager::instance().add( pImpl_->animSubscriber_ );

	CWnd toolbarPos;
	DDX_Control(pDX, IDC_ANIM_TOOLBAR, toolbarPos);
	
	CRect toolbarRect;
    toolbarPos.GetWindowRect (&toolbarRect);
    ScreenToClient (&toolbarRect);

	pImpl_->toolbar.MoveWindow(toolbarRect);

	//------
	
	pImpl_->compToolbar.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,0,0), this, 0 );

	pImpl_->compAnimSubscriber_ = new GUI::Toolbar( "AnimCompToolbar", pImpl_->compToolbar );
	GUI::Manager::instance().add( pImpl_->compAnimSubscriber_ );

	CWnd compToolbarPos;
	DDX_Control(pDX, IDC_ANIM_COMP_TOOLBAR, compToolbarPos);
	
	CRect compToolbarRect;
    compToolbarPos.GetWindowRect (&compToolbarRect);
    ScreenToClient (&compToolbarRect);

	pImpl_->compToolbar.MoveWindow(compToolbarRect);

	//------

	pImpl_->ready = true;
}

BOOL PageAnimations::OnInitDialog()
{
	BW_GUARD;

	mutant()->playAnim( true );
	mutant()->loopAnim( true );

	//Add some drop acceptance functors
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageAnimations >( &tree(), "animation", this, &PageAnimations::addAnims ) );

	pImpl_->compPosMinus.setBitmapID( IDB_MINUS, IDB_MINUS, RGB(255, 0, 255) );
	pImpl_->compRotMinus.setBitmapID( IDB_MINUS, IDB_MINUS, RGB(255, 0, 255) );
	pImpl_->compScaleMinus.setBitmapID( IDB_MINUS, IDB_MINUS, RGB(255, 0, 255) );

	pImpl_->compPosPlus.setBitmapID( IDB_PLUS, IDB_PLUS, RGB(255, 0, 255) );
	pImpl_->compRotPlus.setBitmapID( IDB_PLUS, IDB_PLUS, RGB(255, 0, 255) );
	pImpl_->compScalePlus.setBitmapID( IDB_PLUS, IDB_PLUS, RGB(255, 0, 255) );
		
	INIT_AUTO_TOOLTIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(PageAnimations, TreeList)

	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_WM_HSCROLL()
	
	ON_EN_CHANGE(IDC_ANIM_NAME, OnEnChangeAnimName)

	ON_BN_CLICKED(IDC_ANIM_CHANGE_ANIM, OnChangeAnim)

	ON_EN_KILLFOCUS(IDC_ANIM_NAME, updateName)
	ON_EN_KILLFOCUS(IDC_ANIM_FRAME_RATE, updateFrameRate)
	ON_EN_KILLFOCUS(IDC_ANIM_FIRST, updateFirst)
	ON_EN_KILLFOCUS(IDC_ANIM_LAST, updateLast)
	ON_EN_KILLFOCUS(IDC_ANIM_FRAME, updateFrameNum)

	ON_NOTIFY(TVN_SELCHANGED, IDC_ANIM_NODES, OnTvnSelChangedAnimNodes)
	ON_BN_CLICKED(IDC_ANIM_REMOVE_BLEND, OnBnClickedAnimRemoveBlend)
	
	ON_EN_KILLFOCUS(IDC_ANIM_BLEND, updateBlend)

	ON_EN_SETFOCUS(IDC_ANIM_FRAME, animStop)
	ON_BN_CLICKED(IDC_ANIM_SAVE_FRAME_RATE, OnBnClickedAnimSaveFrameRate)

	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)
	
	ON_BN_CLICKED(IDC_ANIM_COMP_POS_MINUS, OnBnClickedAnimCompPosMinus)
	ON_BN_CLICKED(IDC_ANIM_COMP_POS_PLUS, OnBnClickedAnimCompPosPlus)
	ON_BN_CLICKED(IDC_ANIM_COMP_ROT_MINUS, OnBnClickedAnimCompRotMinus)
	ON_BN_CLICKED(IDC_ANIM_COMP_ROT_PLUS, OnBnClickedAnimCompRotPlus)
	ON_BN_CLICKED(IDC_ANIM_COMP_SCALE_MINUS, OnBnClickedAnimCompScaleMinus)
	ON_BN_CLICKED(IDC_ANIM_COMP_SCALE_PLUS, OnBnClickedAnimCompScalePlus)

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)
END_MESSAGE_MAP()

GUITABS::Content::OnCloseAction PageAnimations::onClose( bool isLastContent )
{
	BW_GUARD;

	//Make sure we invalidate the current page if it is being deleted
	if (this == PageAnimationsImpl::s_currPage)
		PageAnimationsImpl::s_currPage = NULL;
	
	if ( isLastContent )
		return CONTENT_HIDE;
	else
	{
		mutant()->stopAnim( (size_t)this );

		return CONTENT_DESTROY;
	}
}

// PageAnimations message handlers

void PageAnimations::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	GUI::Manager::instance().act( nID );
}

void PageAnimations::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	BW_GUARD;

	pImpl_->s_currPage = this;
	if( !cmdUI->m_pMenu )
		GUI::Manager::instance().update( cmdUI->m_nID );
}

afx_msg LRESULT PageAnimations::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	LPTSTR* msg = (LPTSTR*)wParam;
	mainFrame_->setMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageAnimations::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	mainFrame_->setMessageText( L"" );
	return 0;
}

int PageAnimations::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//We might use this later...
	return 1;
}

//A macro for laying out an animation compression slider set
#define SIZE_ANIM_SLIDER( name ) \
	Utilities::stretchToRight( this, pImpl_->comp##name##Sldr, cx, 100 );\
	Utilities::moveToRight( this, pImpl_->comp##name##Minus, cx, 74 );\
	Utilities::moveToRight( this, pImpl_->comp##name##Plus, cx, 48 );\
	Utilities::moveToRight( this, pImpl_->comp##name, cx, 12 );\
	pImpl_->comp##name##Minus.RedrawWindow();\
	pImpl_->comp##name##Plus.RedrawWindow();\
	pImpl_->comp##name.RedrawWindow();

void PageAnimations::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	if (!pImpl_->ready) return;
	
	using namespace controls;

	Utilities::stretchToRight( this, pImpl_->name, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->source, cx, 40 );
	Utilities::moveToRight( this, pImpl_->change_anim, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->frameNumSlider, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->frameRateSlider, cx, 12 );

	// NOTE: there are alot of magic numbers here. These are the taken from 
	// the default separations and heights of the panel layout initially
	const int SLIDER_HEIGHT = 20;
	const int TEXT_BOX_HEIGHT = 13;
	const int MIN_NODEBOX_BOTTOM = 647;
	// first fix up the Node Blending Stuff
	CRect rect1 = childExtents(pImpl_->nodeBox);
	CRect rect2 = childExtents(pImpl_->compBox);

	int height = rect2.Height();
	if ( cy - height - 8 < MIN_NODEBOX_BOTTOM )
		rect1.bottom = MIN_NODEBOX_BOTTOM;
	else
		rect1.bottom = cy - height - 8;
	childResize(pImpl_->nodeBox, rect1);

	rect2.top = rect1.bottom + 8;
	rect2.bottom = rect2.top + height;
	childResize( pImpl_->compBox, rect2 );

	const int DISTANCE_FROM_BOTTOM_TO_TREE = 78;
	rect2 = rect1;
	rect1 = childExtents(pImpl_->nodeTree);
	rect1.bottom = rect2.bottom - DISTANCE_FROM_BOTTOM_TO_TREE;
	childResize(pImpl_->nodeTree, rect1);

	const int DISTANCE_FROM_BOTTOM_TO_SLIDER = 65;
	rect1 = childExtents(pImpl_->blend);
	rect1.top = rect2.bottom - DISTANCE_FROM_BOTTOM_TO_SLIDER;
	rect1.bottom = rect1.top + SLIDER_HEIGHT;
	childResize(pImpl_->blend, rect1);

	rect1 = childExtents(pImpl_->blendText );
	rect1.top = rect2.bottom - DISTANCE_FROM_BOTTOM_TO_SLIDER;
	rect1.bottom = rect1.top + 25;
	childResize(pImpl_->blendText, rect1);

	rect1 = childExtents(pImpl_->blendSlider);
	rect1.top = rect2.bottom - DISTANCE_FROM_BOTTOM_TO_SLIDER;
	rect1.bottom = rect1.top + SLIDER_HEIGHT;
	childResize(pImpl_->blendSlider, rect1);

	const int DISTANCE_FROM_BOTTOM_TO_REMOVE_BUTTON = 36;
	const int REMOVE_BUTTON_HEIGHT = 28;
	rect1 = childExtents(pImpl_->blendRemove);
	rect1.top = rect2.bottom - DISTANCE_FROM_BOTTOM_TO_REMOVE_BUTTON;
	rect1.bottom = rect1.top + REMOVE_BUTTON_HEIGHT;
	childResize(pImpl_->blendRemove, rect1);

	Utilities::stretchToRight( this, pImpl_->nodeBox, cx, 0 );
	Utilities::stretchToRight( this, pImpl_->nodeTree, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->blendSlider, cx, 12 );
	
	// now fix up the Compression Box
	rect2 = childExtents( pImpl_->compBox );

	const int DISTANCE_TO_TEXT = 25;
	rect1 = childExtents( pImpl_->compPosText );
	rect1.top = rect2.top + DISTANCE_TO_TEXT;
	rect1.bottom = rect1.top + TEXT_BOX_HEIGHT;
	childResize(pImpl_->compPosText, rect1);
	rect1.top = rect1.bottom + TEXT_BOX_HEIGHT;
	rect1.bottom = rect1.top + TEXT_BOX_HEIGHT;
	childResize(pImpl_->compRotText, rect1);
	rect1.top = rect1.bottom + TEXT_BOX_HEIGHT;
	rect1.bottom = rect1.top + TEXT_BOX_HEIGHT;
	childResize(pImpl_->compScaleText, rect1);


	rect1 = childExtents( pImpl_->compPos );
	rect1.top = rect2.top + DISTANCE_TO_TEXT;
	rect1.bottom = rect1.top + TEXT_BOX_HEIGHT;
	childResize(pImpl_->compPos, rect1);
	rect1.top = rect1.bottom + TEXT_BOX_HEIGHT;
	rect1.bottom = rect1.top + TEXT_BOX_HEIGHT;
	childResize(pImpl_->compRot, rect1);
	rect1.top = rect1.bottom + TEXT_BOX_HEIGHT;
	rect1.bottom = rect1.top + TEXT_BOX_HEIGHT;
	childResize(pImpl_->compScale, rect1);

	const int SLIDER_SEPERATION = 8;
	const int DISTANCE_TO_SLIDER = 23;

	rect1 = childExtents( pImpl_->compPosSldr );
	rect1.top = rect2.top + DISTANCE_TO_SLIDER;
	rect1.bottom = rect1.top + SLIDER_HEIGHT;
	childResize(pImpl_->compPosSldr, rect1);
	rect1.top = rect1.bottom + SLIDER_SEPERATION;
	rect1.bottom = rect1.top + SLIDER_HEIGHT;
	childResize(pImpl_->compRotSldr, rect1);
	rect1.top = rect1.bottom + SLIDER_SEPERATION;
	rect1.bottom = rect1.top + SLIDER_HEIGHT;
	childResize(pImpl_->compScaleSldr, rect1);

	const int BUTTON_HEIGHT = 23;
	const int BUTTON_SEPERATION = 4;
	const int DISTANCE_TO_BUTTON = 20;

	rect1 = childExtents( pImpl_->compPosMinus );
	rect1.top = rect2.top + DISTANCE_TO_BUTTON;
	rect1.bottom = rect1.top + BUTTON_HEIGHT;
	childResize(pImpl_->compPosMinus, rect1);
	rect1.top = rect1.bottom + BUTTON_SEPERATION;
	rect1.bottom = rect1.top + BUTTON_HEIGHT;
	childResize(pImpl_->compRotMinus, rect1);
	rect1.top = rect1.bottom + BUTTON_SEPERATION;
	rect1.bottom = rect1.top + BUTTON_HEIGHT;
	childResize(pImpl_->compScaleMinus, rect1);

	rect1 = childExtents( pImpl_->compPosPlus );
	rect1.top = rect2.top + DISTANCE_TO_BUTTON;
	rect1.bottom = rect1.top + BUTTON_HEIGHT;
	childResize(pImpl_->compPosPlus, rect1);
	rect1.top = rect1.bottom + BUTTON_SEPERATION;
	rect1.bottom = rect1.top + BUTTON_HEIGHT;
	childResize(pImpl_->compRotPlus, rect1);
	rect1.top = rect1.bottom + BUTTON_SEPERATION;
	rect1.bottom = rect1.top + BUTTON_HEIGHT;
	childResize(pImpl_->compScalePlus, rect1);

	rect1 = childExtents( pImpl_->compToolbar );
	rect1.top = rect2.top + 115;
	rect1.bottom = rect1.top + 26;
	childResize(pImpl_->compToolbar, rect1);

	rect1 = childExtents( pImpl_->compTotal );
	rect1.top = rect2.top + 110;
	rect1.bottom = rect1.top + 45;
	childResize(pImpl_->compTotal, rect1);

	Utilities::stretchToRight( this, pImpl_->compBox, cx, 0 );
	SIZE_ANIM_SLIDER( Pos );
	SIZE_ANIM_SLIDER( Rot );
	SIZE_ANIM_SLIDER( Scale );

	PathSetDlgItemPath( this->GetSafeHwnd(), IDC_ANIM_SOURCE, bw_utf8tow( pImpl_->fileName ).c_str() );

	TreeList::OnSize( nType, cx, cy );
}

void PageAnimations::updateCheck( CButton& button, const BW::string& actionName )
{
	BW_GUARD;

	int enabled = 0;
	int checked = 0;
	PanelManager::instance().getEditorApp()->pythonAdapter()->ActionScriptUpdate( actionName, enabled, checked );
	button.SetCheck( checked ? BST_CHECKED : BST_UNCHECKED );
}

unsigned PageAnimations::addNodeTree( DataSectionPtr root, HTREEITEM parent, float blend, unsigned nodeCount )
{
	BW_GUARD;

	BW::vector<DataSectionPtr> nodeSections;
    root->openSections( "node", nodeSections );
    BW::vector<DataSectionPtr>::iterator it = nodeSections.begin();
    BW::vector<DataSectionPtr>::iterator end = nodeSections.end();

    wchar_t buf[256];
	
	while( it != end )
    {
        nodeCount++;
		
		DataSectionPtr ds = *it++;

		BW::string boneName = ds->readString( "identifier" );

		bool changed = true;
		
		float weight = mutant()->animBoneWeight( getAnim(), boneName );
		if (weight == -1.0)
		{
			weight = blend;
			changed = false;
		}

		bw_snwprintf( buf, ARRAY_SIZE(buf), L" %s (%g)", bw_utf8tow( boneName ).c_str(), weight );
		HTREEITEM child = pImpl_->nodeTree.InsertItem( buf, parent );
		pImpl_->nodeTree.SetItemState( child,
			TVIS_EXPANDED | (changed ? TVIS_BOLD : 0),
			TVIS_EXPANDED | (changed ? TVIS_BOLD : 0) );

		//Select the node if it was previously selected
		if (boneName == pImpl_->nodeName)
		{
			pImpl_->nodeItem = child;
			pImpl_->nodeTree.SelectItem( pImpl_->nodeItem );
		}

		nodeCount = addNodeTree( ds, child, weight, nodeCount );
	}

	return nodeCount;

}

void PageAnimations::disableField( CEdit& field )
{
	BW_GUARD;

	field.SetWindowText( L"" );
	field.SetReadOnly( true );
	field.ModifyStyle( 0, WS_DISABLED );
}

#define compSliderState( name, state ) \
	pImpl_->comp##name##Sldr.ModifyStyle( state ? WS_DISABLED : 0,\
		state ? 0 : WS_DISABLED );\
	pImpl_->comp##name##Minus.ModifyStyle( state ? WS_DISABLED : 0,\
		state ? 0 : WS_DISABLED );\
	pImpl_->comp##name##Plus.ModifyStyle( state ? WS_DISABLED : 0,\
		state ? 0 : WS_DISABLED );\
	if (!state)\
	{\
		pImpl_->comp##name##Sldr.SetPos( 0 );\
		pImpl_->comp##name##.SetWindowText( L"" );\
	}

afx_msg LRESULT PageAnimations::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	pImpl_->updating = pImpl_->updateCount != mutant()->updateCount("Anim");
	pImpl_->updateCount = mutant()->updateCount("Anim");

	if (!pImpl_->inited)
	{
		OnInitDialog();
		
		pImpl_->inited = true;
	}
	
	if (mutant())
	{
		//Make sure we mark the current page on any UI activity
		PageAnimationsImpl::s_currPage = this;
		
		bool playing = mutant()->playAnim();
		if (playing != pImpl_->wasPlaying)	
		{
			GUI::Manager::instance().update(); // Update the animation toolbar
			pImpl_->wasPlaying = playing;
		}
		
		//Has the model changed?
		BW::string modelName = mutant()->modelName();
		if (pImpl_->updating || (modelName != pImpl_->modelName))
		{
			OnUpdateTreeList();
			pImpl_->lastAnim == ""; // Reset the starting animation folder

			if ((modelName == "") || (!mutant()->canAnim( selID().second )))
			{
				pImpl_->toolbar.ModifyStyle( 0, WS_DISABLED );
			}
			else
			{
				pImpl_->toolbar.ModifyStyle( WS_DISABLED, 0 );
			}
			GUI::Manager::instance().update();
			pImpl_->toolbar.RedrawWindow();

			// Ensure that the lock state will be updated
			int lockLod = Options::getOptionInt( "settings/lockLodParents", 0 );
			pImpl_->lastLockedParents = !lockLod;

			pImpl_->modelName = modelName;
		}
	}

	// Check if the current frame number has changed
	int animFrameNum = mutant()->frameNum( getAnim() );
	if (pImpl_->updating || (animFrameNum != pImpl_->lastFrameNum))
	{
		pImpl_->frameNum.SetIntegerValue( animFrameNum );
		pImpl_->frameNumSlider.SetPos( animFrameNum );
		pImpl_->lastFrameNum = animFrameNum;
	}

	// Ensure that the parent lock is enabled
	int lockLod = Options::getOptionInt( "settings/lockLodParents", 0 );
	if ( lockLod != pImpl_->lastLockedParents )
	{
		locked( !!lockLod );
		pImpl_->lastLockedParents = lockLod;
	}

	// Check whether either the animation (or model) selection has changed
	if (pImpl_->updating || (pImpl_->selectClicked && pImpl_->lastItem != selItem()))
	{
		StringPair animId = selID();

		pImpl_->selectClicked = false;

		bool animSlidersEnabled = false;

		if ((tree().GetParentItem(selItem())) && (mutant()->hasAnims( animId.second )))
		{
			bool lockedParents = locked();
			
			const BW::string& animName = mutant()->animName( animId );
			pImpl_->name.SetWindowText( bw_utf8tow( animName ).c_str() );

			pImpl_->name.SetReadOnly( lockedParents );
			pImpl_->name.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0);

			pImpl_->fileName = mutant()->animFile( animId );

			pImpl_->change_anim.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0);

			Moo::AnimationPtr anim = mutant()->getMooAnim( animId );
			
			if (anim)
			{
				bool foundInterpolated = false;
				
				for (uint i = 0; i < anim->nChannelBinders(); i++)
				{
					Moo::AnimationChannelPtr channel = anim->channelBinder( i ).channel();

					MF_ASSERT( channel );

					if (channel->type() == 1 || channel->type() == 4)
					{
						Moo::InterpolatedAnimationChannel* iac =
							static_cast<Moo::InterpolatedAnimationChannel*>(&*channel);

						setSlidersCompressionErrorFactors(
							iac->scaleCompressionError(),
							iac->rotationCompressionError(),
							iac->positionCompressionError());

						foundInterpolated = true;
					}
				}

				if (foundInterpolated)
				{
					// Make sure to update the compression settings
					animSlidersEnabled = true;
					pImpl_->compChanged = true; 
					pImpl_->animChanged = true;
				}
			}

			float animFrameRate = mutant()->localFrameRate( animId );

			mutant()->lastFrameRate( animFrameRate );
			pImpl_->frameRate.SetValue( animFrameRate );
			pImpl_->frameRate.SetReadOnly( false );
			pImpl_->frameRate.ModifyStyle(WS_DISABLED, 0);

			pImpl_->frameRateSlider.SetPos( (int)(animFrameRate + 0.5f) );
			pImpl_->frameRateSlider.ModifyStyle(WS_DISABLED, 0);

			wchar_t buf[8];
			int animFirst = mutant()->firstFrame( animId );
			if (animFirst != -1)
			{
				bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d", animFirst );
				pImpl_->first.SetWindowText( buf );
			}
			else
			{
				pImpl_->first.SetWindowText(L"");
			}
			pImpl_->first.SetReadOnly( lockedParents );
			pImpl_->first.EnableWindow( !lockedParents && anim );

			int animLast = mutant()->lastFrame( animId );
			if (animLast != -1)
			{
				bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d", animLast );
				pImpl_->last.SetWindowText( buf );
			}
			else
			{
				pImpl_->last.SetWindowText(L"");
			}
			pImpl_->last.SetReadOnly( lockedParents );
			pImpl_->last.EnableWindow( !lockedParents && anim );

			int animFrameNum = mutant()->frameNum( animId );
			int animNumFrames = mutant()->numFrames( animId );
			pImpl_->frameNum.SetMaximum( (float)(animNumFrames - 1) );
			pImpl_->frameNum.SetIntegerValue( animFrameNum );
			pImpl_->frameNum.SetReadOnly( false );
			pImpl_->frameNum.EnableWindow( !lockedParents && anim );
			pImpl_->frameNumSlider.SetRangeMax( animNumFrames - 1 );
			pImpl_->frameNumSlider.SetPos( animFrameNum );
			pImpl_->frameNumSlider.EnableWindow( !lockedParents && anim );

			pImpl_->frameRateSave.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0);

			pImpl_->blend.SetReadOnly( lockedParents );
			pImpl_->blend.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0);
			pImpl_->blendSlider.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0);

			pImpl_->blendRemove.ModifyStyle( lockedParents ? 0 : WS_DISABLED, lockedParents ? WS_DISABLED : 0);

			pImpl_->toolbar.ModifyStyle( WS_DISABLED, 0 );
			pImpl_->compToolbar.ModifyStyle( WS_DISABLED, 0 );

			mutant()->setAnim( (size_t)this, animId );

			if ( !pImpl_->updating )
				mutant()->playAnim( true );
		}
		else // We are a model...
		{
			HTREEITEM model = tree().GetParentItem( selItem() );
			if (!model) model = selItem();

			disableField( pImpl_->name);
			pImpl_->name.SetWindowText( Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/MODEL", (LPCTSTR)tree().GetItemText(model)) );
				
			pImpl_->fileName = animId.second;

			pImpl_->change_anim.ModifyStyle(0, WS_DISABLED);

			disableField( pImpl_->first );

			disableField( pImpl_->last );

			disableField( pImpl_->frameNum );
			pImpl_->frameNumSlider.ModifyStyle(0, WS_DISABLED);

			disableField( pImpl_->frameRate );
			pImpl_->frameRateSlider.ModifyStyle(0, WS_DISABLED);

			pImpl_->frameRateSave.ModifyStyle(0, WS_DISABLED);

			disableField( pImpl_->blend );
			pImpl_->blendSlider.ModifyStyle(0, WS_DISABLED);

			pImpl_->blendRemove.ModifyStyle(0, WS_DISABLED);

			pImpl_->compToolbar.ModifyStyle( 0, WS_DISABLED );

			mutant()->stopAnim( (size_t)this );
		}

		//Enable or disable the compression sliders
		if (!animSlidersEnabled)
		{
			pImpl_->compTotal.SetWindowText( L"" );
		}
		
		compSliderState( Pos, animSlidersEnabled );
		compSliderState( Rot, animSlidersEnabled );
		compSliderState( Scale, animSlidersEnabled );

		PathSetDlgItemPath( this->GetSafeHwnd(), IDC_ANIM_SOURCE, bw_utf8tow( pImpl_->fileName ).c_str() );
			
		{
			DelayRedraw temp( &(pImpl_->nodeTree) );

			pImpl_->nodeTree.DeleteAllItems();
		
			if (mutant()->visual())
			{
				unsigned numNodes =
					addNodeTree( mutant()->visual()->openSection("node") );

				mainFrame_->setStatusText( ID_INDICATOR_NODES, Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/NODES", numNodes ));

				// Get the number of BlendBone nodes and update the status bar
				// appropriately.
				size_t numBlendBones =
					mutant()->blendBoneCount();

				mainFrame_->setStatusText( ID_INDICATOR_BLENDBONE_NODES, Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/BLENDBONE_NODES", numBlendBones ));
			}
		}

		GUI::Manager::instance().update();
		this->RedrawWindow();
		
		pImpl_->lastItem = selItem();
	}
	
	//Now catch any updates from pressing enter in a field or moving a slider
	if (pImpl_->name.doUpdate())
	{
		updateName();
	}
	if (pImpl_->first.doUpdate())
	{
		updateFirst();
	}
	if (pImpl_->last.doUpdate())
	{
		updateLast();
	}
	if ((pImpl_->frameRate.doUpdate()) || (pImpl_->frameRateSlider.doUpdate()))
	{
		updateFrameRate();
	}
	if ((pImpl_->frameNum.doUpdate()) || (pImpl_->frameNumSlider.doUpdate()))
	{
		updateFrameNum();
	}
	if ((pImpl_->blend.doUpdate()) || (pImpl_->blendSlider.doUpdate()))
	{
		updateBlend();
	}
	if (pImpl_->compChanged)
	{
		// Update the compression
		updateCompression();

		// Reload the Model
		if ( !pImpl_->animChanged )
			mutant()->reloadModel();

		// Clear the compression changed flag
		pImpl_->compChanged = false;

		// Clear the animChanged flag
		pImpl_->animChanged = false;
	}
	
	return 0;
}

void PageAnimations::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	HWND target = pScrollBar->GetSafeHwnd();
	
	int pos, min, max;
	
	//if (CModelEditorApp::instance().pythonAdapter())
	//{
		// We will (probably) do stuff here...
	//}
	
	// Frame Rate Slider
	
	if (target == pImpl_->frameRateSlider.GetSafeHwnd())
	{
		float frameRate = (float)pImpl_->frameRateSlider.GetPos();
		pImpl_->frameRate.SetValue( frameRate );
		mutant()->localFrameRate( getAnim(), frameRate  );
	}

	// Frame Num Slider
	
	if (target == pImpl_->frameNumSlider.GetSafeHwnd())
	{
		animStop(); // Stop the animation
		int frameNum = pImpl_->frameNumSlider.GetPos();
		pImpl_->frameNum.SetIntegerValue( frameNum );
		mutant()->frameNum( getAnim(), frameNum  );
		pImpl_->lastFrameNum = frameNum;
	}

	// Node Blend Slider
	
	if (target == pImpl_->blendSlider.GetSafeHwnd())
	{
		pos = pImpl_->blendSlider.GetPos(); 
		min = pImpl_->blendSlider.GetRangeMin(); 
		max = pImpl_->blendSlider.GetRangeMax();

		float blendVal = (float)(1.f * (pos-min) / (max-min)); // Scale 0.0 -> 1.0

		pImpl_->blend.SetValue( blendVal );
	}

	// Compression Sliders

	if ((target == pImpl_->compPosSldr.GetSafeHwnd()) ||
		(target == pImpl_->compRotSldr.GetSafeHwnd()) ||
		(target == pImpl_->compScaleSldr.GetSafeHwnd()))
	{
		//If they have let go of mouse button or holding shift do an update.
		if ( nSBCode == SB_THUMBPOSITION || GetAsyncKeyState(VK_SHIFT) ) 
		{
			pImpl_->compChanged = true; // We will update the compression next frame
		}
	}

	TreeList::OnHScroll(nSBCode, nPos, pScrollBar);
}

BW::string PageAnimations::animAdd()
{
	BW_GUARD;

	static wchar_t szFilter[] =	L"Animation (*.animation)|*.animation||";
	BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
		( BWFileDialog::FD_FILEMUSTEXIST |
		BWFileDialog::FD_ALLOWMULTISELECT );
	BWFileDialog fileDlg(
		true,
		L"", L"",
		flags, szFilter );

	if (pImpl_->lastAnim == "")
	{
		MRU::instance().getDir( "models", pImpl_->lastAnim );
	}

	BW::wstring wlastAnim = bw_utf8tow( pImpl_->lastAnim );
	fileDlg.initialDir( wlastAnim.c_str() );

	if ( fileDlg.showDialog() )
	{
		WaitCursor wait;

		BW::vector< BW::string > fileNames;
		const BW::vector< BW::wstring >& fileNamesW = fileDlg.getFileNames();
		std::transform(
			fileNamesW.begin(),
			fileNamesW.end(),
			std::back_inserter( fileNames ),
			static_cast<BW::string( * )( const BW::wstring& )>( bw_wtoutf8 ) );

		return animAdd( fileNames );
	}

	return "";
}

BW::string PageAnimations::animAdd( const BW::vector< BW::string >& fileNames )
{
	BW_GUARD;

	BW::string animAdded;

	for( BW::vector< BW::string >::const_iterator it = fileNames.begin();
		it != fileNames.end(); ++it )
	{
		BW::string animPath = BWResource::dissolveFilename( *it );

		BW::string::size_type first = animPath.rfind("/") + 1;
		BW::string::size_type last = animPath.rfind(".");
		BW::string animName = animPath.substr( first, last-first );
		animPath = animPath.substr( 0, last );

		const StringPair& animID = mutant()->createAnim( 
			StringPair( animName , mutant()->modelName() ), animPath );

		selID( animID );

		pImpl_->lastAnim = animPath;

		animAdded = animID.first;
	}

	OnUpdateTreeList();

	return animAdded;
}

/*~ function ModelEditor.addAnim
 *	@components{ modeleditor }
 *
 *	This function adds an animation to the list.
 *	If the filename is not specified it shows the Open File dialog,
 *	which allows an animation(s) to be added.
 *
 *	@param	fileName	(optional) The filename of the animation to add.
 *
 *	@return	Returns the name of the added animation.
 */
static BW::string addAnim( const BW::string& fileName )
{
	BW_GUARD;

	BW::string res;

	if (PageAnimations::currPage())
	{
		if ( fileName.empty() )
		{
			res = PageAnimations::currPage()->animAdd();
		}
		else
		{
			BW::vector< BW::string > fileNames;
			fileNames.push_back( fileName );
			res = PageAnimations::currPage()->animAdd (fileNames);
		}
	}

	return res;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, addAnim, OPTARG( BW::string, "", END ),
						 ModelEditor );

bool PageAnimations::isLockedAnim()
{
	BW_GUARD;

	return locked();
}

/*~ function ModelEditor.isAnimLocked
 *	@components{ modeleditor }
 *
 *	This function returns whether the selected animation is locked.
 *	If the 'Lock LOD Parents' option is enabled in the Preferences dialog, then animations
 *	that belong to the LOD parents of the currently visible LOD level will be locked
 *	from modifications.
 *
 *	@return Returns True (1) if the animation is locked, False (0) otherwise.
 */
static PyObject * py_isAnimLocked( PyObject * args )
{
	BW_GUARD;

	if (PageAnimations::currPage())
		return PyInt_FromLong( PageAnimations::currPage()->isLockedAnim() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( isAnimLocked, ModelEditor )


void PageAnimations::animPlay()
{
	BW_GUARD;

	mutant()->playAnim( true );
}
/*~ function ModelEditor.playAnim
 *	@components{ modeleditor }
 *
 *	This function plays the currently selected animation.
 */
static PyObject * py_playAnim( PyObject * args )
{
	BW_GUARD;

	if (PageAnimations::currPage())
		PageAnimations::currPage()->animPlay();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( playAnim, ModelEditor )

void PageAnimations::animStop()
{
	BW_GUARD;

	mutant()->playAnim( false );
}
/*~ function ModelEditor.stopAnim
 *	@components{ modeleditor }
 *
 *	This function stops the currently selected animation.
 */
static PyObject * py_stopAnim( PyObject * args )
{
	BW_GUARD;

	if (PageAnimations::currPage())
		PageAnimations::currPage()->animStop();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( stopAnim, ModelEditor )

void PageAnimations::animLoop()
{
	BW_GUARD;

	mutant()->loopAnim( ! mutant()->loopAnim() );
}
/*~ function ModelEditor.loopAnim
 *	@components{ modeleditor }
 *
 *	This function toggles whether the animation should loop play.
 */
static PyObject * py_loopAnim( PyObject * args )
{
	BW_GUARD;

	if (PageAnimations::currPage())
		PageAnimations::currPage()->animLoop();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( loopAnim, ModelEditor )

void PageAnimations::animRemove()
{
	BW_GUARD;

	mutant()->removeAnim( getAnim() );

	OnUpdateTreeList();
}
/*~ function ModelEditor.removeAnim
 *	@components{ modeleditor }
 *
 *	This function removes the currently selected animation.
 */
static PyObject * py_removeAnim( PyObject * args )
{
	BW_GUARD;

	if (PageAnimations::currPage())
		PageAnimations::currPage()->animRemove();

	Py_RETURN_NONE;
}
PY_MODULE_FUNCTION( removeAnim, ModelEditor )

bool PageAnimations::animPlaying()
{
	BW_GUARD;

	return mutant()->playAnim();
}

/*~ function ModelEditor.animPlaying
 *	@components{ modeleditor }
 *
 *	This function returns whether the currently selected animation is playing.
 *
 *	@return Returns True (1) if the animation is playing, False (0) otherwise.
 */
static PyObject * py_animPlaying( PyObject * args )
{
	BW_GUARD;

	if (PageAnimations::currPage())
		return PyInt_FromLong( PageAnimations::currPage()->animPlaying() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( animPlaying, ModelEditor )

bool PageAnimations::animLooping()
{
	BW_GUARD;

	return mutant()->loopAnim();
}

/*~ function ModelEditor.animLooping
 *	@components{ modeleditor }
 *
 *	This function returns whether the currently selected animation is looping.
 *
 *	@return Returns True (1) if the animation is looping, False (0) otherwise.
 */
static PyObject * py_animLooping( PyObject * args )
{
	BW_GUARD;

	if (PageAnimations::currPage())
		return PyInt_FromLong( PageAnimations::currPage()->animLooping() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( animLooping, ModelEditor )

void PageAnimations::OnEnChangeAnimName()
{
	BW_GUARD;

	//Do nothing if there is no parent item (i.e. we are a model, not an animation)
	//This is not necessary since the field should be readonly (but lets be safe)
	if (tree().GetParentItem(selItem()) == NULL)
		return;
	
	CString new_name;
	pImpl_->name.GetWindowText(new_name);
	tree().SetItemText( selItem(), new_name );
	
}

void PageAnimations::updateName()
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	//Do nothing if there is no parent item (i.e. we are a model, not an animation)
	if (tree().GetParentItem(selItem()) == NULL)
		return;
	
	CString new_name_cstr;
	pImpl_->name.GetWindowText(new_name_cstr);
	BW::string new_name = bw_wtoutf8( new_name_cstr.GetString() );

	BW::string::size_type first = new_name.find_first_not_of(" ");
	BW::string::size_type last = new_name.find_last_not_of(" ") + 1;
	if (first != BW::string::npos)
	{
		new_name = new_name.substr( first, last-first );
	}
	else
	{
		pImpl_->name.SetWindowText( bw_utf8tow( getAnim().first ).c_str() );
		tree().SetItemText( selItem(), bw_utf8tow( getAnim().first ).c_str() );
		
		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/NO_RENAME_ANIMATION"),
			Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/INVALID_ANIMATION_NAME"), MB_OK | MB_ICONERROR );

		return;
	}
	
	if (mutant()->animName( getAnim(), new_name ))
	{
		pImpl_->name.SetWindowText( bw_utf8tow( new_name ).c_str() );
		tree().SetItemText( selItem(), bw_utf8tow( new_name ).c_str() );
		const int diff = static_cast<int>(last-first);
		pImpl_->name.SetSel( diff, diff, 0 );
		
		selID( StringPair( new_name, getAnim().second ) );
	}
	else
	{
		pImpl_->name.SetWindowText( bw_utf8tow( getAnim().first ).c_str() );
		tree().SetItemText( selItem(), bw_utf8tow( getAnim().first ).c_str() );
		
		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/ANIMATION_NAME_USED"),
			Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/ANIMATION_NAME_EXISTS"), MB_OK | MB_ICONERROR );
	}
}

void PageAnimations::OnChangeAnim()
{
	BW_GUARD;

	static wchar_t BASED_CODE szFilter[] =	L"Animation (*.animation)|*.animation||";
	BWFileDialog fileDlg ( true, L"", L"", BWFileDialog::FD_FILEMUSTEXIST, szFilter );

	if (pImpl_->lastAnim == "")
	{
		MRU::instance().getDir( "models", pImpl_->lastAnim );
	}

	BW::wstring wlastAnim = bw_utf8tow( pImpl_->lastAnim );
	fileDlg.initialDir( wlastAnim.c_str() );

	if (fileDlg.showDialog())
	{
		WaitCursor wait;

		BW::string animPath = BWResource::removeExtension(
			BWResource::dissolveFilename( bw_wtoutf8( fileDlg.getFileName() ))).to_string();
				
		mutant()->changeAnim( getAnim(), animPath );

		pImpl_->lastAnim = animPath;

		OnUpdateTreeList();
	}
}

void PageAnimations::updateFrameRate()
{
	BW_GUARD;

	//Do nothing if we are currently updating
	if (pImpl_->updating) return;

	float rate = pImpl_->frameRate.GetValue();

	pImpl_->frameRateSlider.SetPos( (int)rate );
	
	mutant()->localFrameRate( getAnim(), rate, true );
}

void PageAnimations::updateFirst()
{
	BW_GUARD;

	//Do nothing if we are currently updating
	if (pImpl_->updating) return;
	
	//First lets get the value entered
	CString frame;
	pImpl_->first.GetWindowText( frame );

	int frameNum = -1;
	if (frame != L"") // If something was entered
	{
		swscanf( frame.GetString(), L"%d", &frameNum );
	}
	mutant()->firstFrame( getAnim(), frameNum );

	int animNumFrames = mutant()->numFrames( getAnim() );
	pImpl_->frameNumSlider.SetRangeMax(animNumFrames - 1);
}

void PageAnimations::updateLast()
{
	BW_GUARD;

	//Do nothing if we are currrently updating
	if (pImpl_->updating) return;
	
	//First lets get the value entered
	CString frame;
	pImpl_->last.GetWindowText( frame );

	int frameNum = -1;
	if (frame != L"" ) // If something was entered
	{
		swscanf( frame.GetString(), L"%d", &frameNum );
	}
	mutant()->lastFrame( getAnim(), frameNum );

	int animNumFrames = mutant()->numFrames( getAnim() );
	pImpl_->frameNumSlider.SetRangeMax(animNumFrames - 1);
}

void PageAnimations::updateFrameNum()
{
	BW_GUARD;

	int frame = pImpl_->frameNum.GetIntegerValue();

	pImpl_->frameNumSlider.SetPos( frame );

	mutant()->frameNum( getAnim(), frame );
}

void PageAnimations::OnTvnSelChangedAnimNodes(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	*pResult = 0;

	if (pImpl_->reentryFilter == 0)
	{
		++(pImpl_->reentryFilter);
		LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
		//Save the node item index
		pImpl_->nodeItem = pImpl_->nodeTree.GetSelectedItem();
	
		CString itemText = pImpl_->nodeTree.GetItemText( pImpl_->nodeItem );
	
		//Convert the CString to a char*
		BW::string utf8buf;
		bw_wtoutf8( itemText, utf8buf );
		size_t size = utf8buf.length() + 1;
		char* buf = new char[ size ];
		memcpy( buf, utf8buf.c_str(), size );
	
		//Remove the leading space
		char* text = buf + 1;
		
		text = strtok( text, "(");
	
		//Remove the trailing space
		text[strlen(text) - 1] = 0; 
	
		//Save the node name
		pImpl_->nodeName = BW::string( text );
	
		text = strtok( NULL, ")");
	
		float blendVal = 1.0;
		
		sscanf( text, "%f", &blendVal );
	
		//Set the edit box
		pImpl_->blend.SetValue( blendVal );
	
		//Set the slider position
		int min = pImpl_->blendSlider.GetRangeMin(); 
		int max = pImpl_->blendSlider.GetRangeMax();
		int pos = (int)(min + blendVal*(max-min));
		pImpl_->blendSlider.SetPos( pos ); 
	
		bw_safe_delete_array( buf );
	
		--(pImpl_->reentryFilter);
	}
}

void PageAnimations::OnBnClickedAnimRemoveBlend()
{
	BW_GUARD;

	if ((getAnim().first != "") &&
		(getAnim().second != "") &&
		(pImpl_->nodeName != ""))
	{
		DelayRedraw temp( &(pImpl_->nodeTree) );
		
		mutant()->removeAnimNode( getAnim(), pImpl_->nodeName );

		pImpl_->nodeTree.DeleteAllItems();
			
		addNodeTree( mutant()->visual()->openSection("node") );
	}
}

void PageAnimations::updateBlend()
{
	BW_GUARD;

	//First lets get the value entered
	CString blend;
	pImpl_->blend.GetWindowText( blend );

	float blendVal = -1;
	if ( blend != "" ) // If something was entered
	{
		// Temporarily increment the reentry filter so changing the list doesn't
		// call the "OnTvnSelChangedAnimNodes", which would mess up with the
		// current selection, blend node, etc.
		++(pImpl_->reentryFilter);

		DelayRedraw temp( &(pImpl_->nodeTree) );
		
		swscanf( blend.GetString(), L"%f", &blendVal );
		mutant()->animBoneWeight( getAnim(), pImpl_->nodeName, blendVal );

		pImpl_->nodeTree.DeleteAllItems();
			
		addNodeTree( mutant()->visual()->openSection("node") );

		// Restore reentry filter
		--(pImpl_->reentryFilter);
	}
}

void PageAnimations::OnBnClickedAnimSaveFrameRate()
{
	BW_GUARD;

	mutant()->saveFrameRate( getAnim() );
}


/*afx_msg*/ HBRUSH PageAnimations::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor ) 
{
	BW_GUARD;

	HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor ); 

	pImpl_->frameNum.SetBoundsColour( pDC, pWnd, 
		pImpl_->frameNum.GetMinimum(), pImpl_->frameNum.GetMaximum() ); 

	pImpl_->frameRate.SetBoundsColour( pDC, pWnd, 
		pImpl_->frameRate.GetMinimum(), pImpl_->frameRate.GetMaximum() ); 


	return brush; 
} 

void PageAnimations::selChange( const StringPair& itemID ) 
{ 
	BW_GUARD;

	if ( !pImpl_->updating )
		mutant()->playAnim( true ); 
}

void PageAnimations::selClick( const StringPair& itemID ) 
{ 
	BW_GUARD;

	pImpl_->selectClicked = true;

	if ( !pImpl_->updating )
		mutant()->playAnim( true ); 
}

bool PageAnimations::addAnims( UalItemInfo* ii )
{
	BW_GUARD;

	if (locked())
	{
		::MessageBox( this->GetSafeHwnd(),
			Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/UNABLE_ADD_ANIM"),
			Localise(L"MODELEDITOR/PAGES/PAGE_ANIMATIONS/MODEL_LOCKED"), MB_OK | MB_ICONERROR );
		return false;
	}
	
	WaitCursor wait;
	
	while( ii )
	{
		BW::string animPath = BWResource::dissolveFilename( bw_wtoutf8( ii->longText() ) );

		BW::string::size_type first = animPath.rfind("/") + 1;
		BW::string::size_type last = animPath.rfind(".");
		BW::string animName = animPath.substr( first, last-first );
		animPath = animPath.substr( 0, last );

		StringPair animID ( animName , selID().second );
		
		selID( mutant()->createAnim( animID, animPath ) );

		ii = ii->getNext();
	}
	OnUpdateTreeList();
	return true;
}


bool PageAnimations::loadAnim( const BW::string& animName )
{
	BW_GUARD;

	StringPair animID( animName, mutant()->modelName() );

	if (!MeApp::instance().mutant()->hasAnims( animID.second ))
	{
		return false;
	}

	mutant()->setAnim( (size_t)this, animID );

	selectItem( animID );
			
	return true;
}
/*~ function ModelEditor.loadAnim
 *	@components{ modeleditor }
 *
 *	This function loads an animation by its name.
 * 
 *	@param	animName	The name of the animation to load.
 *
 *	@return	Returns True if the animation has been loaded, False otherwise.
 */
static bool loadAnim( const BW::string& animName )
{
	BW_GUARD;

	bool res = false;
	if (PageAnimations::currPage())
	{
		res = PageAnimations::currPage()->loadAnim( animName );
	}

	return res;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, loadAnim, 
						 ARG( BW::string, END ), ModelEditor );


int PageAnimations::numAnimFrames()
{
	BW_GUARD;

	return mutant()->numFrames( getAnim() );
}
/*~ function ModelEditor.numAnimFrames
 *	@components{ modeleditor }
 *
 *	This function returns the number of the loaded animation frames.
 *
 *	@return	Returns the number of the loaded animation frames.
 */
static int numAnimFrames()
{
	BW_GUARD;

	int numFrames = 0;
	if (PageAnimations::currPage())
	{
		numFrames = PageAnimations::currPage()->numAnimFrames();
	}

	return numFrames;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, numAnimFrames, END, ModelEditor );


bool PageAnimations::playAnimFrame( int frameNum )
{
	BW_GUARD;

	if (frameNum < 0 || frameNum >= mutant()->numFrames( getAnim() ))
	{
		return false;
	}

	mutant()->frameNum( getAnim(), frameNum );

	return true;
}
/*~ function ModelEditor.playAnimFrame
 *	@components{ modeleditor }
 *
 *	This function displays the specified animation frame.
 *
 *	@param	frameNum	The number of the frame to display.
 *
 *	@return	Returns True if the frame has been displayed, False otherwise.
 */
static bool playAnimFrame( int frameNum )
{
	BW_GUARD;

	bool res =  false;
	if (PageAnimations::currPage())
	{
		res = PageAnimations::currPage()->playAnimFrame( frameNum );
	}

	return res;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, playAnimFrame, ARG( int, END ), ModelEditor );

BW_END_NAMESPACE
