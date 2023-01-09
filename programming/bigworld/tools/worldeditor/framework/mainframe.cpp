#include "pch.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/gui/pages/page_properties.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/gui/dialogs/splash_dialog.hpp"
#include "editor_shared/dialogs/file_dialog.hpp"
#include "pyscript/script.hpp"
#include "appmgr/options.hpp"
#include "controls/show_cursor_helper.hpp"
#include "guimanager/gui_manager.hpp"
#include "common/user_messages.hpp"
#include "common/property_list.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/message_box.hpp"
#include "moo/render_context.hpp"

DECLARE_DEBUG_COMPONENT2( "WorldEditor2", 0 )

BW_BEGIN_NAMESPACE

static DogWatch s_updateStatusBar( "UpdateStatusBar" );
static DogWatch s_winFrameUpdate( "WinFrameUpdate" );
static DogWatch s_updateControls( "UpdateControls" );


static const int c_menuPaneWidth = 307;

// MainFrame

IMPLEMENT_DYNCREATE(MainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(MainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
//	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_WM_SIZE()
	ON_MESSAGE( WM_ENTERSIZEMOVE, OnEnterSizeMove)
	ON_MESSAGE( WM_EXITSIZEMOVE, OnExitSizeMove)
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TRIANGLES, OnUpdateIndicatorTriangles)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SNAPS, OnUpdateIndicatorSnaps)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_POSITION, OnUpdateIndicatorPosition)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_MEMORYLOAD, OnUpdateIndicatorMemoryLoad)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FRAMERATE, OnUpdateIndicatorFrameRate)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CHUNKS, OnUpdateIndicatorChunks)
	ON_COMMAND(ID_PROPERTIES_LIST_POPUP_ADDITEM, OnPopupPropertyListAddItem)
	ON_COMMAND(ID_PROPERTIES_LISTITEM_POPUP_REMOVEITEM, OnPopupPropertyListItemRemoveItem)
	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_WM_MENUSELECT()
	ON_WM_EXITMENULOOP()
	ON_NOTIFY_RANGE( TBN_HOTITEMCHANGE, 0, 0xffffffff, OnToolbarHotItemChange  )
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
//	ID_SEPARATOR,           // status line indicator
//	ID_INDICATOR_MESSAGE,
	ID_INDICATOR_MEMORYLOAD,
	ID_INDICATOR_POSITION,
	ID_INDICATOR_SNAPS,
	ID_INDICATOR_TRIANGLES,
	ID_INDICATOR_FRAMERATE,
	ID_INDICATOR_CHUNKS,
//	ID_INDICATOR_CAPS,
//	ID_INDICATOR_NUM,
//	ID_INDICATOR_SCRL,
};


// MainFrame construction/destruction

MainFrame::MainFrame()
	: pScriptObject_(NULL)
	, resizing_( false )
	, triangles_( "" )
	, initialised_( false )
	, GUI::ActionMaker<MainFrame>( "doSaveSelectionAsPrefab", &MainFrame::saveSelectionAsPrefab )
	, GUI::ActionMaker<MainFrame, 1>( "doShowToolbar", &MainFrame::showToolbar )
	, GUI::ActionMaker<MainFrame, 2>( "doHideToolbar", &MainFrame::hideToolbar )
	, GUI::UpdaterMaker<MainFrame>( "updateToolbar", &MainFrame::updateToolbar )
	, GUI::ActionMaker<MainFrame, 3>( "doShowStatusBar", &MainFrame::showStatusBar )
	, GUI::ActionMaker<MainFrame, 4>( "doHideStatusBar", &MainFrame::hideStatusBar )
	, GUI::UpdaterMaker<MainFrame, 1>( "updateStatusBar", &MainFrame::updateStatusBar )
	, GUI::ActionMaker<MainFrame, 5>( "doShowPlayerPreview", &MainFrame::showPlayerPreview )
	, GUI::ActionMaker<MainFrame, 6>( "doHidePlayerPreview", &MainFrame::hidePlayerPreview )
	, GUI::UpdaterMaker<MainFrame, 2>( "updatePlayerPreview", &MainFrame::updatePlayerPreview )
	, GUI::UpdaterMaker<MainFrame, 3>( "updateToolMode", &MainFrame::updateToolMode )
{
	m_bAutoMenuEnable = FALSE;
}

MainFrame::~MainFrame()
{
}

int MainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	BW_GUARD;

	MsgBox::setDefaultParent( m_hWnd );
	SetWindowLong( m_hWnd, GWL_STYLE, GetStyle() & ~FWS_ADDTOTITLE );
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//Added a splashscreen
	if( !IsDebuggerPresent() )
		CSplashDlg::ShowSplashScreen( this );

	EnableDocking( CBRS_ALIGN_ANY );

	if (!m_wndStatusBar.Create(this,
			WS_CHILD | WS_VISIBLE | CBRS_BOTTOM) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// initialise the size of the indicators
//	int messageWidth = 250;
	int memoryLoadWidth = 160;
	int trianglesWidth = 120;
	int snapsWidth = 140;
	int positionWidth = 380;
	int chunksWidth = 600;
	int frameRateWidth = 60;

//	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_MESSAGE),
//							ID_INDICATOR_MESSAGE, SBPS_NORMAL, messageWidth);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_TRIANGLES),
							ID_INDICATOR_TRIANGLES, SBPS_NORMAL, trianglesWidth);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SNAPS),
							ID_INDICATOR_SNAPS, SBPS_NORMAL, snapsWidth);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_MEMORYLOAD),
							ID_INDICATOR_MEMORYLOAD, SBPS_NORMAL, memoryLoadWidth);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_POSITION),
							ID_INDICATOR_POSITION, SBPS_NORMAL, positionWidth);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_CHUNKS),
							ID_INDICATOR_CHUNKS, SBPS_NORMAL, chunksWidth);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_FRAMERATE),
							ID_INDICATOR_FRAMERATE, SBPS_NORMAL, frameRateWidth);

	SetWindowText( Localise(L"WORLDEDITOR/APPLICATION_NAME" ) );

	return 0;
}

void MainFrame::OnDestroy()
{
	MsgBox::setDefaultParent( NULL );
}

void MainFrame::updateStatusBar()
{
	BW_GUARD;
	s_updateStatusBar.start();

	if (m_wndStatusBar.GetSafeHwnd())
	{
		// update the status bar
		m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_MEMORYLOAD),
			bw_utf8tow(WorldManager::instance().getStatusMessage( 0 )).c_str(), TRUE);
		m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_TRIANGLES),
			bw_utf8tow(WorldManager::instance().getStatusMessage( 1 )).c_str(), TRUE);
		m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SNAPS),
			bw_utf8tow(WorldManager::instance().getStatusMessage( 2 )).c_str(), TRUE);
		m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_POSITION),
			bw_utf8tow(WorldManager::instance().getStatusMessage( 3 )).c_str(), TRUE);
		m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_FRAMERATE),
			bw_utf8tow(WorldManager::instance().getStatusMessage( 4 )).c_str(), TRUE);
		m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_INDICATOR_CHUNKS),
			bw_utf8tow(WorldManager::instance().getStatusMessage( 5 )).c_str(), TRUE);
	}
	s_updateStatusBar.stop();
}

void MainFrame::frameUpdate()
{
	BW_GUARD;
	
	s_winFrameUpdate.start();

	s_updateControls.start();
	// update controls on child windows
	PanelManager::instance().updateControls();
	s_updateControls.stop();

	updateStatusBar();
	// update the scene tab
	//TODO : put back in when page scene is working correctly
	//PageScene::instance().update();

	// remove the focus if over the 3d pane and selection filter not being used
	// (as selection filter drops over the 3d view)
	if (WorldManager::instance().cursorOverGraphicsWnd() &&
		GetCapture() == NULL //&&
//		!m_wndToolBar2.selectionFilter_.GetDroppedState() &&
//		!m_wndToolBar2.coordFilter_.GetDroppedState()
	)
	{
		// TODO: this is a bit of a hack on selected item, 
		// they should be able to look after themselves
		PropertyList::deselectAllItems();
	}

	s_winFrameUpdate.stop();
}

void MainFrame::enableCloseButton( bool enable )
{
	UINT uEnable = MF_BYCOMMAND | MF_ENABLED;
	if ( !enable )
	{
		uEnable = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
	}
	EnableMenuItem( ::GetSystemMenu(m_hWnd, FALSE), SC_CLOSE,
		uEnable );
}

//////////////


// MainFrame message handlers

BOOL MainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	BW_GUARD;

	BOOL result = CFrameWnd::OnCreateClient(lpcs, pContext);

	initialised_ = ( result != 0 );

	return result;
}

void MainFrame::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	if (!initialised_)
		return;

	CFrameWnd::OnSize(nType, cx, cy);	// this calls RecalcLayout()
}

LRESULT MainFrame::OnEnterSizeMove (WPARAM, LPARAM)
{
	// Set the resizing_ flag to true, so the view knows that we are resizing
	// and that it shouldn't change the Moo mode.
	resizing_ = true;
	return 0;
}

LRESULT MainFrame::OnExitSizeMove (WPARAM, LPARAM)
{
	BW_GUARD;

	// Set the resizing_ flag back to false, so the view knows that it has to
	// change the Moo mode on the next repaint.
	resizing_ = false;
	// And send the repaint message to the view.
	::InvalidateRect( WorldManager::instance().hwndGraphics(), NULL, TRUE );
	::UpdateWindow( WorldManager::instance().hwndGraphics() );
	return 0;
}

BOOL MainFrame::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	BW_GUARD;

	// capture the alt key and stop the menu bar from gaining focus
	if (wParam == SC_KEYMENU)
		return 1;

	return CFrameWnd::OnWndMsg(message, wParam, lParam, pResult);
}


void MainFrame::OnUpdateIndicatorTriangles(CCmdUI *pCmdUI)
{
	BW_GUARD;

	pCmdUI->Enable(); 
}

void MainFrame::OnUpdateIndicatorSnaps(CCmdUI *pCmdUI)
{
	BW_GUARD;

	pCmdUI->Enable(); 
}

void MainFrame::OnUpdateIndicatorPosition(CCmdUI *pCmdUI)
{
	BW_GUARD;

	pCmdUI->Enable(); 
}

void MainFrame::OnUpdateIndicatorMemoryLoad(CCmdUI *pCmdUI)
{
	BW_GUARD;

	pCmdUI->Enable(); 
}

void MainFrame::OnUpdateIndicatorFrameRate(CCmdUI *pCmdUI)
{
	BW_GUARD;

	pCmdUI->Enable(); 
}

void MainFrame::OnUpdateIndicatorChunks(CCmdUI *pCmdUI)
{
	BW_GUARD;

	pCmdUI->Enable(); 
}

void MainFrame::GetMessageString(UINT nID, CString& rMessage) const
{
	BW_GUARD;

	if (nID == AFX_IDS_IDLEMESSAGE)
	{
		rMessage.Empty();
		return;
	}

	return CFrameWnd::GetMessageString(nID, rMessage);
}


// ----------------------------------------------------------------------------
LRESULT MainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if (message == WM_INPUT)
	{
		// In the main frame, only relay raw input messages.
		LRESULT inputResult;
		if (InputDevices::handleWindowsMessage(this->m_hWnd, message, wParam, lParam, inputResult))
		{
			return inputResult;
		}
	}

	return CFrameWnd::WindowProc(message, wParam, lParam);
}

LRESULT MainFrame::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	return CFrameWnd::DefWindowProc(message, wParam, lParam);
}

void MainFrame::OnPopupPropertyListAddItem()
{
	BW_GUARD;

	PageProperties::instance().OnListAddItem();
}

void MainFrame::OnPopupPropertyListItemRemoveItem()
{
	BW_GUARD;

	PageProperties::instance().OnListItemRemoveItem();
}

void MainFrame::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	GUI::Manager::instance().act( nID );
}

bool MainFrame::saveSelectionAsPrefab( GUI::ItemPtr item )
{
	BW_GUARD;

	if (!WorldEditorApp::instance().pythonAdapter()->canSavePrefab())
	{
		WorldManager::instance().addCommentaryMsg( LocaliseUTF8(L"WORLDEDITOR/GUI/MAINFRAME/PREFAB_WARNING") );
		return false;
	}
	// open a save as dialog
	// szFilters is a text string that includes two file name filters:
	// "*.my" for "MyType Files" and "*.*' for "All Files."
	wchar_t szFilters[]=
		L"Prefab Files (*.prefab)|*.prefab|All Files (*.*)|*.*||";

	ShowCursorHelper scopedShowCursor( true );

	// Create an Open dialog; the default file name extension is ".prefab".
	BWFileDialog::FDFlags flags = ( BWFileDialog::FDFlags )
		(	BWFileDialog::FD_OVERWRITEPROMPT |
			BWFileDialog::FD_HIDEREADONLY |
			BWFileDialog::FD_PATHMUSTEXIST );
	BWFileDialog fileDlg(false, L"prefab", L"*.prefab",
		flags, szFilters, this);

	CString path( L"object/prefabs/" );
	BW::wstring initialDir = bw_utf8tow( Options::getOptionString( bw_wtoutf8( ( path + L"directory" ).GetString() ) ) );
	if (initialDir.empty())
		initialDir = bw_utf8tow( BWResource::getDefaultPath() + "/" );
	std::replace(initialDir.begin(), initialDir.end(), L'/', L'\\');
	fileDlg.initialDir( initialDir.c_str() );

	// Display the file dialog. When user clicks OK, fileDlg.DoModal() 
	// returns IDOK.
	if( fileDlg.showDialog() == false)
		return true;

	BW::wstring pathName = fileDlg.getFileName();

	WorldEditorApp::instance().pythonAdapter()->saveSelectionPrefab( bw_wtoutf8( pathName ) );
	return true;
}

bool MainFrame::showToolbar( GUI::ItemPtr item )
{
	BW_GUARD;

	return BaseMainFrame::showToolbar( item );
}

bool MainFrame::hideToolbar( GUI::ItemPtr item )
{
	BW_GUARD;

	return BaseMainFrame::hideToolbar( item );
}

unsigned int MainFrame::updateToolbar( GUI::ItemPtr item )
{
	BW_GUARD;

	return BaseMainFrame::updateToolbar( item );
}

bool MainFrame::showStatusBar( GUI::ItemPtr item )
{
	BW_GUARD;

	ShowControlBar( &m_wndStatusBar, TRUE, FALSE );
	return true;
}

bool MainFrame::hideStatusBar( GUI::ItemPtr item )
{
	BW_GUARD;

	ShowControlBar( &m_wndStatusBar, FALSE, FALSE );
	return true;
}

unsigned int MainFrame::updateStatusBar( GUI::ItemPtr item )
{
	BW_GUARD;

	return ~m_wndStatusBar.GetStyle() & WS_VISIBLE;
}

bool MainFrame::showPlayerPreview( GUI::ItemPtr item )
{
	BW_GUARD;

	WorldManager::instance().setPlayerPreviewMode( true );
	return true;
}

bool MainFrame::hidePlayerPreview( GUI::ItemPtr item )
{
	BW_GUARD;

	WorldManager::instance().setPlayerPreviewMode( false );
	return true;
}

unsigned int MainFrame::updatePlayerPreview( GUI::ItemPtr item )
{
	BW_GUARD;

	return !WorldManager::instance().isInPlayerPreviewMode();
}

unsigned int MainFrame::updateToolMode( GUI::ItemPtr item )
{
	BW_GUARD;

	if ( PanelManager::pInstance() == NULL )
		return 1;

	return (PanelManager::instance().currentTool() == bw_utf8tow( (*item)["toolMode"] ) ) ? 1 : 0;
}

void MainFrame::OnClose()
{
	BW_GUARD;

	if (WorldManager::instance().canClose( LocaliseUTF8(L"WORLDEDITOR/GUI/MAINFRAME/CAN_CLOSE_EXIT") ))
	{
		// this can take a while, specially if the ThumbnailManager is waiting
		// for a big model to load, but it's the safest way.
		// It is possible that PanelManager does not exist at this point if the
		// application have been exited early.
		if ( PanelManager::pInstance() != NULL )
			PanelManager::instance().onClose();

		CFrameWnd::OnClose();
	}
}

void MainFrame::OnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu )
{
	BW_GUARD;

	BW::string s;
	if( ! ( nFlags & ( MF_DISABLED | MF_GRAYED | MF_SEPARATOR ) ) )
	{
		GUI::Manager::instance().update();
		GUI::ItemPtr item = GUI::Manager::instance().findByCommandID( nItemID );
		if( item )
		{
			s = item->description();
			while( s.find( '&' ) != s.npos )
			{
				s.erase( s.begin() + s.find( '&' ) );
			}
		}
	}
	SetMessageText( bw_utf8tow( s ).c_str() );
}

void MainFrame::OnExitMenuLoop( BOOL bIsTrackPopupMenu )
{
	BW_GUARD;

	SetMessageText( L"" );
}

void MainFrame::OnToolbarHotItemChange( UINT id, NMHDR* pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

	*result = 0;
	BW::string s;
	LPNMTBHOTITEM hotitem = (LPNMTBHOTITEM)pNotifyStruct;
	GUI::ItemPtr item = GUI::Manager::instance().findByCommandID( hotitem->idNew );
	if( item )
	{
		s = item->description();
		while( s.find( '&' ) != s.npos )
		{
			s.erase( s.begin() + s.find( '&' ) );
		}
	}
	SetMessageText( bw_utf8tow( s ).c_str() );
}

void MainFrame::OnSysColorChange()
{
	BW_GUARD;

	CFrameWnd::OnSysColorChange();
	for( unsigned int i = 0; i < numToolbars_; ++i )
		toolbars_[ i ].SendMessage( WM_SYSCOLORCHANGE, 0, 0 );
}

void MainFrame::setMessageText( const wchar_t * pText )
{
	SetMessageText( pText );
}


void MainFrame::setStatusText( UINT id, const wchar_t * text )
{
	m_wndStatusBar.SetPaneText(
		m_wndStatusBar.CommandToIndex(id),
		text,
		TRUE);
}

BW_END_NAMESPACE

