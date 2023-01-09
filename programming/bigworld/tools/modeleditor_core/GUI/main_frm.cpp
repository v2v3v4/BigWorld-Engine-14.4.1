// MainFrm.cpp : implementation of the CMainFrame class
//

#include "pch.hpp"

#include "model_editor_doc.h"

#include "me_app.hpp"

#include "appmgr/options.hpp"
#include "resmgr/string_provider.hpp"

#include "splash_dialog.hpp"

#include "guitabs/guitabs.hpp"

#include "guimanager/gui_manager.hpp"

#include "common/property_list.hpp"
#include "common/user_messages.hpp"

#include "page_materials.hpp"

#include "panel_manager.hpp"

#include "main_frm.h"

#include "cstdmf/message_box.hpp"
#include "modeleditor_core/App/me_shell.hpp"
#include "modeleditor_core/App/mru.hpp"
#include "modeleditor_core/Models/mutant.hpp"

BW_BEGIN_NAMESPACE

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()

	ON_WM_CLOSE()

	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)

	ON_WM_MENUSELECT()
	ON_WM_EXITMENULOOP()
	ON_NOTIFY_RANGE( TBN_HOTITEMCHANGE, 0, 0xffffffff, OnToolbarHotItemChange  ) 

	ON_MESSAGE( WM_ENTERSIZEMOVE, OnEnterSizeMove)
	ON_MESSAGE( WM_EXITSIZEMOVE, OnExitSizeMove)

END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_TRIANGLES,
	ID_INDICATOR_NODES,
	ID_INDICATOR_BLENDBONE_NODES,
	ID_INDICATOR_TETXURE_MEM,
	ID_INDICATOR_FRAMERATE,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
	GUI::ActionMaker<CMainFrame>( "doShowToolbar|doHideToolbar", &CMainFrame::handleGUIAction ),
	GUI::UpdaterMaker<CMainFrame>( "updateToolbar", &CMainFrame::updateToolbar ),
	resizing_( false )
{
	BW_GUARD;

	m_bAutoMenuEnable = FALSE;
	PropertyList::mainFrame( this );
}

CMainFrame::~CMainFrame()
{
}

LRESULT CMainFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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

bool CMainFrame::handleGUIAction( GUI::ItemPtr item )
{
	BW_GUARD;

	if (item->action() == "doShowToolbar")	return showToolbar( item );
	else if (item->action() == "doHideToolbar")	return hideToolbar( item );

	return false;
}

bool CMainFrame::showToolbar( GUI::ItemPtr item )
{
	BW_GUARD;

	return BaseMainFrame::showToolbar( item );
}

bool CMainFrame::hideToolbar( GUI::ItemPtr item )
{
	BW_GUARD;

	return BaseMainFrame::hideToolbar( item );
}

unsigned int CMainFrame::updateToolbar( GUI::ItemPtr item )
{
	BW_GUARD;

	return BaseMainFrame::updateToolbar( item );
}

Vector2 CMainFrame::currentCursorPosition() const
{
	BW_GUARD;

	POINT pt;
	::GetCursorPos( &pt );
	::ScreenToClient( MeShell::instance().hWndGraphics(), &pt );
	return Vector2( ( float ) pt.x, ( float ) pt.y );
}

Vector3 CMainFrame::getWorldRay(int x, int y) const
{
	BW_GUARD;

	Vector3 v = Moo::rc().invView().applyVector(
		Moo::rc().camera().nearPlanePoint(
			(float(x) / Moo::rc().screenWidth()) * 2.f - 1.f,
			1.f - (float(y) / Moo::rc().screenHeight()) * 2.f ) );
	v.normalise();
	return v;
}

bool CMainFrame::cursorOverGraphicsWnd() const
{
	BW_GUARD;

	HWND fore = ::GetForegroundWindow();
	if ( fore != MeShell::instance().hWndApp() && ::GetParent( fore ) != MeShell::instance().hWndApp() )
		return false; // foreground window is not the main window nor a floating panel.

	RECT rt;
	::GetWindowRect( MeShell::instance().hWndGraphics(), &rt );
	POINT pt;
	::GetCursorPos( &pt );

	if ( pt.x < rt.left ||
		pt.x > rt.right ||
		pt.y < rt.top ||
		pt.y > rt.bottom )
	{
		return false;
	}

	HWND hwnd = ::WindowFromPoint( pt );
	if ( hwnd != MeShell::instance().hWndGraphics() )
		return false; // it's a floating panel, return.
	HWND parent = hwnd;
	while( ::GetParent( parent ) != NULL )
		parent = ::GetParent( parent );
	::SendMessage( hwnd, WM_MOUSEACTIVATE, (WPARAM)parent, WM_LBUTTONDOWN * 65536 + HTCLIENT );

	//Lame but we used to cheat and do this through the singleton
	//so this is no worse.
	const_cast< CMainFrame * >( this )->setMessageText( L"" );

	return true;
}


//==============================================================================
bool CMainFrame::isActiveWindow() const
{
	HWND fgWin = ::GetForegroundWindow();
	return fgWin == GetSafeHwnd() || ::GetParent( fgWin ) == GetSafeHwnd();
}


//==============================================================================
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	BW_GUARD;

	MsgBox::setDefaultParent( m_hWnd );
	SetWindowLong( m_hWnd, GWL_STYLE, GetStyle() & ~FWS_ADDTOTITLE );

	SetWindowText( Localise(L"MODELEDITOR/DASH_MODELEDITOR", Localise(L"MODELEDITOR/UNTITLED") ) );

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//Show the splashscreen
	if (!!Options::getOptionInt( "startup/showSplashScreen", 1 ) && !IsDebuggerPresent() )
		CSplashDlg::ShowSplashScreen( this );
	
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
			sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_TRIANGLES),
							ID_INDICATOR_TRIANGLES, SBPS_NORMAL, 128);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_NODES),
							ID_INDICATOR_NODES, SBPS_NORMAL, 128);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_BLENDBONE_NODES),
							ID_INDICATOR_BLENDBONE_NODES, SBPS_NORMAL, 160);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_TETXURE_MEM),
							ID_INDICATOR_TETXURE_MEM, SBPS_NORMAL, 200);
	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_INDICATOR_FRAMERATE),
							ID_INDICATOR_FRAMERATE, SBPS_NORMAL, 128);

	setStatusText( ID_INDICATOR_TRIANGLES, L"" );
	setStatusText( ID_INDICATOR_NODES, L"" );
	setStatusText( ID_INDICATOR_BLENDBONE_NODES, L"" );
	setStatusText( ID_INDICATOR_TETXURE_MEM, L"" );
	setStatusText( ID_INDICATOR_FRAMERATE, L"" );

	EnableDocking(CBRS_ALIGN_ANY);

	return 0;
}

void CMainFrame::OnDestroy()
{
	MsgBox::setDefaultParent( NULL );
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT cs, CCreateContext* pContext)
{
	BW_GUARD;

	CFrameWnd::OnCreateClient(cs, pContext);
	
	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	BW_GUARD;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	return CFrameWnd::PreCreateWindow(cs);
}

void CMainFrame::OnGUIManagerCommand(UINT nID)
{
	BW_GUARD;

	GUI::Manager::instance().act( nID );
}

void CMainFrame::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	BW_GUARD;

	if( !cmdUI->m_pMenu )
		GUI::Manager::instance().update( cmdUI->m_nID );
}

void CMainFrame::updateGUI( bool force /* = false */)
{
	BW_GUARD;

	//Update the Title bar name if the model is modified
	bool dirty = MeApp::instance().mutant()->dirty();
	static bool s_wasDirty = !dirty;
	if ((force) || (s_wasDirty != dirty))
	{
		BW::wstring titleText;
		bw_utf8tow( MeApp::instance().mutant()->modelName(), titleText );

		BW::wstring::size_type first = titleText.rfind(L"/") + 1;
		BW::wstring::size_type last = titleText.rfind(L".");
		titleText = titleText.substr( first, last-first );

		if (titleText == L"")
			titleText = Localise(L"MODELEDITOR/UNTITLED");

		if (dirty)
			titleText = titleText + L" *";

		titleText = Localise(L"MODELEDITOR/DASH_MODELEDITOR", titleText);

		AfxGetApp()->m_pMainWnd->SetWindowText( titleText.c_str() ); 

		GUI::Manager::instance().update();

		s_wasDirty = dirty;
	}
		
	//Update the undo/redo buttons if needed
	static bool firstTime = true;
	static bool canUndo, canRedo;
	if( force || firstTime ||
		(canUndo != UndoRedo::instance().canUndo()) ||
		(canRedo != UndoRedo::instance().canRedo()) )
	{
		firstTime = false;
		canUndo = UndoRedo::instance().canUndo();
		canRedo = UndoRedo::instance().canRedo();
		GUI::Manager::instance().update();
	}
	
	//Update all the Panels
	PanelManager::instance().updateControls();
}

void CMainFrame::OnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu )
{
	BW_GUARD;

   BW::wstring s;
   if( ! ( nFlags & ( MF_DISABLED | MF_GRAYED | MF_SEPARATOR ) ) )
   {
       GUI::Manager::instance().update();
       GUI::ItemPtr item = GUI::Manager::instance().findByCommandID( nItemID );
       if( item )
       {
           bw_utf8tow( item->description(), s );
           while( s.find( L'&' ) != s.npos )
           {
               s.erase( s.begin() + s.find( L'&' ) );
           }
       }
   }
   SetMessageText( s.c_str() );
}

void CMainFrame::OnExitMenuLoop( BOOL bIsTrackPopupMenu )
{
	BW_GUARD;

   SetMessageText( L"" );
}

void CMainFrame::OnToolbarHotItemChange( UINT id, NMHDR* pNotifyStruct, LRESULT* result )
{
	BW_GUARD;

   *result = 0;
   BW::wstring s;
   LPNMTBHOTITEM hotItem = (LPNMTBHOTITEM)pNotifyStruct;
   GUI::ItemPtr item = GUI::Manager::instance().findByCommandID( hotItem->idNew );
   if( item )
   {
       bw_utf8tow( item->description(), s );
       while( s.find( L'&' ) != s.npos )
       {
           s.erase( s.begin() + s.find( L'&' ) );
       }
   }
   SetMessageText( s.c_str() );
} 

LRESULT CMainFrame::OnEnterSizeMove (WPARAM, LPARAM)
{
	// Set the resizing_ flag to true, so the view knows that we are resizing
	// and that it shouldn't change the Moo mode.
	resizing_ = true;
	return 0;
}

LRESULT CMainFrame::OnExitSizeMove (WPARAM, LPARAM)
{
	BW_GUARD;

	// Set the resizing_ flag back to false, so the view knows that it has to
	// change the Moo mode on the next repaint.
	resizing_ = false;
	// And send the repaint message to the view.
	::InvalidateRect( MeShell::instance().hWndGraphics(), NULL, TRUE );
	::UpdateWindow( MeShell::instance().hWndGraphics() );
	return 0;
}

void CMainFrame::OnClose()
{
	BW_GUARD;

	if (MeApp::instance().canExit( true ))
	{
		PageMaterials::currPage()->restoreView();
		MeApp::instance().mutant()->unregisterModelChangeCallback( PageMaterials::currPage() );

		//Make sure the current model is at the head of the MRU list
		BW::string modelName = MeApp::instance().mutant()->modelName();
		if (modelName != "")
		{
			MRU::instance().update( "models", MeApp::instance().mutant()->modelName(), true );
		}

		MeApp::instance().camera()->save();
			
		Options::save();

		PanelManager::instance().onClose();

		CFrameWnd::OnClose();
	}
}

void CMainFrame::grabFocus()
{
	SetFocus();
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

BW_END_NAMESPACE

