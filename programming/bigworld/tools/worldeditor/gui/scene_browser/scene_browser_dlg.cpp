// SceneBrowserScratchDlg.cpp : implementation file
//


#include "pch.hpp"
#include "cstdmf/string_utils.hpp"
#include "common/user_messages.hpp"
#include "controls/user_messages.hpp"
#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "scene_browser.hpp"
#include "scene_browser_dlg.hpp"

BW_BEGIN_NAMESPACE

namespace
{
	const int MIN_WINDOW_WIDTH = 80;
	const int MIN_WINDOW_HEIGHT = 150;
	const int TOOLBAR_GAP_X = 12;
	const uint32 CHECK_UPDATE_LIST_MILLIS = 500;
	const uint32 MAX_DB_MILLIS = 5;
	const uint32 MAX_DB_MILLIS_STEP = 1500;
	const uint32 MAX_DB_MILLIS_MULTIPLIER = 4; // ramp up to 4 * MAX_DB_MILLIS;
} // anonymous namespace


// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
/*static*/ const BW::wstring SceneBrowserDlg::contentID = L"SceneBrowser";


/**
 *	Constructor
 *
 *	@param pParent	CWnd parent
 */
SceneBrowserDlg::SceneBrowserDlg( CWnd* pParent /*=NULL*/ ) :
	CDialog( SceneBrowserDlg::IDD, pParent ),
	lastUpdate_( timestamp() ),
	ignoreGroupByMessages_( false ),
	inited_( false )
{
	BW_GUARD;

	MF_ASSERT( SceneBrowser::pInstance() != NULL );
}


/**
 *	This method is called by the GUITABS framework to let the panel load it's
 *	layout info.
 *
 *	@param section	DataSection for this panel's data.
 *	@return		True on success.
 */
bool SceneBrowserDlg::load( DataSectionPtr section )
{
	BW_GUARD;

	DataSectionPtr pListDS = section->openSection( "List" );

	list_.load( pListDS );

	DataSectionPtr pGroupDS = section->openSection( "GroupBy" );
	ItemInfoDB::Type groupType;
	if (groupType.fromDataSection( pGroupDS ))
	{
		for (ListGroups::iterator it = groups_.begin();
			it != groups_.end(); ++it)
		{
			if ((*it).second == groupType)
			{
				for (int i = 0; i < groupBy_.GetCount(); ++i)
				{
					if (groupBy_.GetItemData( i ) == DWORD_PTR( &(*it) ))
					{
						groupBy_.SetCurSel( i );
						list_.groupBy( &(*it) );
						break;
					}
				}
				break;
			}
		}
	}

	return true;
}


/**
 *	This method is called by the GUITABS framework to let the panel save it's
 *	layout info.
 *
 *	@param section	DataSection for this panel's data.
 *	@return		True on success.
 */
bool SceneBrowserDlg::save( DataSectionPtr section )
{
	BW_GUARD;

	DataSectionPtr pListDS = section->newSection( "List" );

	int idx = groupBy_.GetCurSel();
	if (idx >= 0)
	{
		ListGroup * pGroup = (ListGroup *)groupBy_.GetItemData( idx );
		if (pGroup)
		{
			DataSectionPtr pGroupDS = section->newSection( "GroupBy" );
			pGroup->second.toDataSection( pGroupDS );
		}
	}

	list_.save( pListDS );

	return true;
}


/**
 *	This method returns the currently visible items
 *
 *	@param chunkItems	Container where the items are returned.
 */
void SceneBrowserDlg::currentItems(
							BW::vector< ChunkItemPtr > & chunkItems ) const
{
	BW_GUARD;

	list_.currentItems( chunkItems );
}


/**
 *	This MFC method is overriden to attach member variables to dialog controls
 *
 *	@param pDX	MFC data exchange class pointer
 */
void SceneBrowserDlg::DoDataExchange( CDataExchange* pDX )
{
	BW_GUARD;

	CDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_SB_SEARCHBK, search_ );
	DDX_Control( pDX, IDC_SB_GROUP_BY_LABEL, groupByLabel_ );
	DDX_Control( pDX, IDC_SB_GROUP_BY, groupBy_ );
	DDX_Control( pDX, IDC_SB_LIST, list_ );
	DDX_Control( pDX, IDC_SB_STATUS, statusBar_ );
	DDX_Control( pDX, IDC_SB_WORKING_ANIM, workingAnim_ );
}


// Message map
BEGIN_MESSAGE_MAP( SceneBrowserDlg, CDialog )
	ON_WM_SIZE()
	ON_CBN_SELCHANGE( IDC_SB_GROUP_BY, OnGroupBy )
	ON_COMMAND_RANGE( GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand )
	ON_MESSAGE( WM_SEARCHFIELD_CHANGE, OnSearchFieldChanged )
	ON_MESSAGE( WM_SEARCHFIELD_FILTERS, OnSearchFieldFilters )
	ON_MESSAGE( WM_UPDATE_CONTROLS, OnUpdateControls )
	ON_MESSAGE( WM_LIST_GROUP_BY_CHANGED, OnListGroupByChanged )
	ON_MESSAGE( WM_LIST_SCROLL_TO, OnListScrollTo )
END_MESSAGE_MAP()


/**
 *	This MFC event handler initialises the dialog and its components.
 *
 *	@return		TRUE to let windows set focus.
 */
BOOL SceneBrowserDlg::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();

	INIT_AUTO_TOOLTIP();

	DataSectionPtr pDS = BWResource::openSection( SCENE_BROWSER_CONFIG );

	// Init search field
	search_.init( MAKEINTRESOURCE( IDB_SEARCHMAGNIFIER_CLICK ),
		MAKEINTRESOURCE( IDB_SEARCHCLOSE ), list_.searchFilters().filterDesc(),
		Localise( L"SCENEBROWSER/SEARCH_FILTERS" ),
		Localise( L"SCENEBROWSER/SEARCH_TIP" ) );


	// Init toolbar
	CSize tbSize( 0, 0 );
	if (pDS)
	{
		DataSectionPtr pToolBarDS = pDS->openSection( "Toolbar" );
		if (pToolBarDS)
		{
			for (int i = 0; i < pToolBarDS->countChildren(); ++i)
			{
				GUI::Manager::instance().add(
								new GUI::Item( pToolBarDS->openChild( i ) ) );
			}

			toolbar_.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
				TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS |
				CBRS_TOOLTIPS, CRect(0,0,1,1), this, 0 );
			toolbar_.SetBitmapSize( CSize( 16, 16 ) );
			toolbar_.SetButtonSize( CSize( 24, 22 ) );

			CToolTipCtrl* tc = toolbar_.GetToolTips();
			if (tc)
			{
				tc->SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0,
									SWP_NOMOVE | SWP_NOSIZE );
			}

			GUI::Toolbar* guiTB = new GUI::Toolbar( "SBToolbar", toolbar_ );
			GUI::Manager::instance().add( guiTB );

			tbSize = guiTB->minimumSize();
			// place outside the parent's rect, it'll be possitioned correctly
			// on the first resize.
			toolbar_.SetWindowPos( 0, -100, -100, tbSize.cx, tbSize.cy,
									SWP_NOMOVE | SWP_NOZORDER );
		}
	}


	// Init List
	DataSectionPtr pListDS;
	if (pDS)
	{
		pListDS = pDS->openSection( "List" );
	}
	list_.init( pListDS );


	// Init Group By (MUST BE AFTER LIST'S INIT)
	list_.groups( groups_ );

	int idx = groupBy_.AddString( Localise( L"SCENEBROWSER/NO_GROUP_BY" ) );
	groupBy_.SetItemData( idx, NULL );
	groupBy_.SetCurSel( idx );

	for (ListGroups::iterator it = groups_.begin();
		it != groups_.end(); ++it)
	{
		int idx = groupBy_.AddString( bw_utf8tow( (*it).first ).c_str() );
		groupBy_.SetItemData( idx, DWORD_PTR( &(*it) ) );
	}

	workingAnim_.init( IDR_WORKING_ANI, 16, 16, 10, 4 );

	// Calculate offsets in original dialog design for later use on resize.
	CRect clientRect;
	this->GetClientRect( clientRect );

	CRect searchRect;
	search_.GetWindowRect( searchRect );
	this->ScreenToClient( searchRect );

	CRect groupByLabelRect;
	groupByLabel_.GetWindowRect( groupByLabelRect );
	this->ScreenToClient( groupByLabelRect );

	CRect groupByRect;
	groupBy_.GetWindowRect( groupByRect );
	this->ScreenToClient( groupByRect );

	CRect listRect;
	list_.GetWindowRect( listRect );
	this->ScreenToClient( listRect );

	CRect statusRect;
	statusBar_.GetWindowRect( statusRect );
	this->ScreenToClient( statusRect );

	groupByLabelLeft_ = groupByLabelRect.left - searchRect.right;
	groupByLabelTop_ = groupByLabelRect.top;

	groupByLeft_ = groupByRect.left - searchRect.right;
	groupByMinRight_ = groupByRect.right;
	groupByTop_ = groupByRect.top;
	groupByHeight_ = groupByRect.bottom - groupByRect.top;

	toolbarLeft_ = groupByRect.right - searchRect.right + TOOLBAR_GAP_X;
	toolbarMinRight_ = groupByRect.right + tbSize.cx + TOOLBAR_GAP_X;

	listLeft_ = listRect.left;
	listRight_ = clientRect.right - listRect.right;
	listTop_ = listRect.top - groupByRect.bottom;
	listBottom_ = statusRect.top - listRect.bottom;

	statusBarLeft_ = statusRect.left;
	statusBarRight_ = clientRect.right - statusRect.right;
	statusBarHeight_ = statusRect.bottom - statusRect.top;
	statusBarBottom_ = clientRect.bottom - statusRect.bottom;

	inited_ = true;
	return TRUE;  // return TRUE  unless you set the focus to a control
}


/**
 *	This MFC event handler changes the layout of the dialog's controls to best
 *	fit the window shape and size.
 *
 *	@param nType	MFC resize type
 *	@param cx		New dialog width.
 *	@param cy		New dialog height.
 */
void SceneBrowserDlg::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	if (!inited_)
		return;

	CDialog::OnSize( nType, cx, cy );

	CRect clientRect;
	this->GetClientRect( clientRect );

	clientRect.right = std::max( MIN_WINDOW_WIDTH, (int)clientRect.right );
	clientRect.bottom = std::max( MIN_WINDOW_HEIGHT, (int)clientRect.bottom );

	CRect searchRect;
	search_.GetWindowRect( searchRect );
	this->ScreenToClient( searchRect );

	workingAnim_.SetWindowPos( 0, searchRect.right + 2, searchRect.top + 2,
						16, 16, SWP_NOZORDER );

	int actualGroupByLabelLeft = groupByLabelLeft_ + searchRect.right;
	int actualGroupByLabelTop = groupByLabelTop_;

	int actualGroupByLeft = groupByLeft_ + searchRect.right;
	int actualGroupByTop = groupByTop_;

	int actualToolbarLeft = toolbarLeft_ + searchRect.right;
	int actualToolbarTop = groupByTop_;

	if (clientRect.right <= groupByMinRight_)
	{
		// put "Group By" one row below
		actualGroupByLabelLeft = searchRect.left;
		actualGroupByLabelTop = searchRect.bottom + groupByLabelTop_;
		actualGroupByLeft = searchRect.left + (groupByLeft_ - groupByLabelLeft_);
		actualGroupByTop = searchRect.bottom + groupByTop_;

		// And put the toolbar two rows below
		actualToolbarLeft = searchRect.left;
		actualToolbarTop = searchRect.bottom + groupByHeight_ + groupByTop_ * 2;
	}
	else if (clientRect.right <= toolbarMinRight_)
	{
		// Put the toolbar one rows below
		actualToolbarLeft = searchRect.left;
		actualToolbarTop = searchRect.bottom + groupByTop_;
	}

	int statusTop = clientRect.bottom - statusBarBottom_ - statusBarHeight_;

	int actualListTop = actualToolbarTop + groupByHeight_ + listTop_;

	groupByLabel_.SetWindowPos( 0,
								actualGroupByLabelLeft, actualGroupByLabelTop,
								0, 0, SWP_NOZORDER | SWP_NOSIZE );

	groupBy_.SetWindowPos( 0, actualGroupByLeft, actualGroupByTop,
							  0, 0, SWP_NOZORDER | SWP_NOSIZE );

	toolbar_.SetWindowPos( 0, actualToolbarLeft, actualToolbarTop,
								0, 0, SWP_NOZORDER | SWP_NOSIZE );

	list_.SetWindowPos( 0,
		listLeft_, actualListTop,
		clientRect.right - listLeft_ - listRight_, statusTop - listBottom_ - actualListTop,
		SWP_NOZORDER );

	statusBar_.SetWindowPos( 0,
		statusBarLeft_, statusTop,
		clientRect.right - statusBarLeft_ - statusBarRight_, statusBarHeight_,
		SWP_NOZORDER );
}


/**
 *	This MFC event handler changes the layout of the dialog's controls to best
 *	fit the window shape and size.
 *
 *	@param nType	MFC resize type
 *	@param cx		New dialog width.
 *	@param cy		New dialog height.
 */
void SceneBrowserDlg::OnGroupBy()
{
	BW_GUARD;

	if (!ignoreGroupByMessages_)
	{
		int idx = groupBy_.GetCurSel();
		ListGroup * pGroup = (ListGroup *)groupBy_.GetItemData( idx );
		list_.groupBy( pGroup );
	}
}


/**
 *	This method forwards toolbar WM_COMMAND messages to the GUI Manager.
 *
 *	@param nID	Control ID of the control issuing the command
 */
void SceneBrowserDlg::OnGUIManagerCommand( UINT nID )
{
	BW_GUARD;

	GUI::Manager::instance().act( nID );
}


/**
 *	This control event handler is called when the search field is changed.
 *
 *	@param wParam	SearchField* of the control that generated the event.
 *	@param lParam	unused.
 *	@return		Ignored.
 */
LRESULT SceneBrowserDlg::OnSearchFieldChanged( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	// new search.
	list_.search( bw_wtoutf8( search_.searchText() ) );
	return 0;
}


/**
 *	This control event handler is called when the search field filter icon is
 *	clicked.
 *
 *	@param wParam	SearchField* of the control that generated the event.
 *	@param lParam	unused.
 *	@return		Ignored.
 */
LRESULT SceneBrowserDlg::OnSearchFieldFilters( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	// Tell the list to show its filters.
	CRect rect;
	search_.GetWindowRect( rect );
	CPoint pt( rect.left, (rect.top + rect.bottom) / 2 );

	list_.searchFilters().doModal( *this, pt );
	
	// Redo the search so new filters take effect.
	list_.search( bw_wtoutf8( search_.searchText() ) );

	search_.idleText( list_.searchFilters().filterDesc() );

	return 0;
}


/**
 *	This message is sent from the app once each frame.
 *
 *	@param wParam	unused.
 *	@param lParam	unused.
 *	@return		Ignored.
 */
LRESULT SceneBrowserDlg::OnUpdateControls( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	static DogWatch dw( "SceneBrowserTick" );
	ScopedDogWatch sdw( dw );

	uint64 startTick = timestamp();

	bool needsUpdateStatusBar = false;

	bool forceListTick = false;
	if (ItemInfoDB::instance().needsTick())
	{
		// The user must have changed something, try to spend some time updating
		// the item db, and make sure the list updated on the next frame.
		ItemInfoDB::instance().tick( MAX_DB_MILLIS * MAX_DB_MILLIS_MULTIPLIER );
		forceListTick = true;
	}

	bool showWorkingAnim = ItemInfoDB::instance().hasChanged();

	uint64 timeSinceLastUpdate = 
		(timestamp() - lastUpdate_) * 1000 / stampsPerSecond();

	if (forceListTick || list_.needsTick() || timeSinceLastUpdate > CHECK_UPDATE_LIST_MILLIS)
	{
		// Time to update the list.
		if (list_.tick( forceListTick ))
		{
			needsUpdateStatusBar = true;
			showWorkingAnim = true;
		}
		lastUpdate_ = timestamp();
	}

	if (showWorkingAnim)
		workingAnim_.show();
	else
		workingAnim_.hide();

	workingAnim_.update();

	if (list_.hasSelectionChanged())
	{
		static DogWatch dw( "SetAppSelection" );
		ScopedDogWatch sdw( dw );

		// Selection changed by the list after user interaction
		if (SceneBrowser::instance().callbackSelChange())
		{
			CWaitCursor wait;
			(*SceneBrowser::instance().callbackSelChange())(
														list_.selection() );
		}

		if (SceneBrowser::instance().callbackCurrentSel())
		{
			const BW::vector<ChunkItemPtr> & cbSelection =
							(*SceneBrowser::instance().callbackCurrentSel())();
			lastSelection_ = cbSelection;
		}

		needsUpdateStatusBar = true;
		list_.clearSelectionChanged();
	}
	else if (SceneBrowser::instance().callbackCurrentSel())
	{
		static DogWatch dw( "GetAppSelection" );
		ScopedDogWatch sdw( dw );

		// Selection changed from the outside
		const BW::vector<ChunkItemPtr> & cbSelection =
							(*SceneBrowser::instance().callbackCurrentSel())();
		if (lastSelection_ != cbSelection)
		{
			lastSelection_ = cbSelection;

			// Make sure the list's and DB's items are to date.
			ItemInfoDB::instance().tick();
			list_.tick();
			lastUpdate_ = timestamp();

			list_.selection( lastSelection_ );
			needsUpdateStatusBar = true;
		}
	}
	
	if (needsUpdateStatusBar)
	{
		static DogWatch dw( "UpdateStatusBar" );
		ScopedDogWatch sdw( dw );

		updateStatusBar();
	}

	// Update the database if there's still remaining tick time. We do this to
	// reduce spikes in the frame rate.
	int ellapsedTickMillis =
			int( (timestamp() - startTick) * 1000 / stampsPerSecond() );

	int multiplier = std::min(
			1 + ItemInfoDB::instance().numPending() / MAX_DB_MILLIS_STEP,
			MAX_DB_MILLIS_MULTIPLIER );
	int maxMillis = MAX_DB_MILLIS * multiplier;
	int dbTickMaxMillis = std::max( 0, maxMillis - ellapsedTickMillis );

	ItemInfoDB::instance().tick( dbTickMaxMillis );

	return 0;
}


/**
 *	This message is sent when the list's groupBy changes (for example when
 *	loading a column layout preset).
 *
 *	@param wParam	unused.
 *	@param lParam	unused.
 *	@return		Ignored.
 */
LRESULT SceneBrowserDlg::OnListGroupByChanged( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	// Update the Group By combo with the new group by from the list.
	const ListGroup * pNewGroup = list_.groupBy();

	for (int i = 0; i < groupBy_.GetCount(); ++i)
	{
		ListGroup * pCurGroup = (ListGroup *)groupBy_.GetItemData( i );

		if ((pCurGroup == NULL && pNewGroup == NULL) ||
			(pCurGroup != NULL && pNewGroup != NULL &&
					pCurGroup->second == pNewGroup->second))
		{
			bool oldIgnore = ignoreGroupByMessages_;
			ignoreGroupByMessages_ = true;
			
			groupBy_.SetCurSel( i );
	
			ignoreGroupByMessages_ = oldIgnore;
			break;
		}
	}

	return 0;
}


/**
 *	This message is sent to tell the list to scroll to make the item visible.
 *
 *	@param wParam	unused.
 *	@param lParam	Chunk item pointer.
 *	@return		Ignored.
 */
LRESULT SceneBrowserDlg::OnListScrollTo( WPARAM wParam, LPARAM lParam )
{
	BW_GUARD;

	list_.scrollTo( (ChunkItem *)lParam );

	return 0;
}


/**
 *	This method updates the status bar text.
 */
void SceneBrowserDlg::updateStatusBar()
{
	BW_GUARD;

	bool searching = !search_.searchText().empty();

	if (list_.numSelectedItems() == 0)
	{
		wchar_t * localiseTag = searching ?
			L"SCENEBROWSER/STATUS_NO_SELECT_SEARCH" :
			L"SCENEBROWSER/STATUS_NO_SELECT";

		statusBar_.SetWindowText( Localise( localiseTag,
			list_.numItems(),
			list_.numTris(), list_.numPrimitives() ) );
	}
	else
	{
		wchar_t * localiseTag = searching ?
			L"SCENEBROWSER/STATUS_SELECT_SEARCH" :
			L"SCENEBROWSER/STATUS_SELECT";

		statusBar_.SetWindowText( Localise( localiseTag,
			list_.numSelectedItems(), list_.numItems(),
			list_.numSelectedTris(), list_.numTris(),
			list_.numSelectedPrimitives(), list_.numPrimitives() ) );
	}
}
BW_END_NAMESPACE

