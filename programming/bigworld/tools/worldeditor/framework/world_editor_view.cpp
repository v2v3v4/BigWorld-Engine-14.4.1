#include "pch.hpp"
#include "worldeditor/framework/world_editor_view.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/framework/world_editor_doc.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "appmgr/app.hpp"
#include "appmgr/module_manager.hpp"
#include "common/cooperative_moo.hpp"
#include "input/input.hpp"

BW_BEGIN_NAMESPACE

IMPLEMENT_DYNCREATE(WorldEditorView, CView)


BEGIN_MESSAGE_MAP(WorldEditorView, CView)
	ON_WM_PAINT()
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


WorldEditorView::WorldEditorView() :
	lastRect_( 0, 0, 0, 0 )
{
}


WorldEditorView::~WorldEditorView()
{
}


BOOL WorldEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	BW_GUARD;

	// Change style to no background to avoid flicker in the 3D view:
	cs.lpszClass = AfxRegisterWndClass(
		CS_OWNDC | CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW), 0 );
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	return CView::PreCreateWindow(cs);
}

void WorldEditorView::OnKillFocus( CWnd *pNewWnd )
{
	BW_GUARD;

	// Make sure we flush any input events to avoid stuck keys.
	InputDevices::setFocus( false, NULL );
	CView::OnKillFocus( pNewWnd );
}

void WorldEditorView::OnDraw(CDC* /*pDC*/)
{
}


WorldEditorDoc* WorldEditorView::GetDocument() const 
{
	BW_GUARD;

	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(WorldEditorDoc)));
	return (WorldEditorDoc*)m_pDocument;
}


void WorldEditorView::OnPaint()
{
	BW_GUARD;

	CView::OnPaint();

	CRect rect;
	GetClientRect( &rect );

	if (WorldEditorApp::instance().mfApp() &&
		ModuleManager::instance().currentModule() &&
		!WorldManager::criticalMessageOccurred())
	{
		if ( CooperativeMoo::beginOnPaint( &WorldEditorApp::instance() ) )
		{		
			// Change mode when a paint message is received and the size of the
			// window is different than last stored size:
			if 
			( 
				lastRect_ != rect 
				&&
				Moo::rc().device() 
				&& 
				Moo::rc().windowed() 
				&&
				rect.Width() && rect.Height() 
				&&
				!((MainFrame*)WorldEditorApp::instance().mainWnd())->resizing() 
			)
			{
				CWaitCursor wait;
				Moo::rc().changeMode(Moo::rc().modeIndex(), Moo::rc().windowed(), true);
				lastRect_ = rect;
			}
			WorldEditorApp::instance().mfApp()->updateFrame( false );
			CooperativeMoo::endOnPaint( &WorldEditorApp::instance() );
		}
	}
	else
	{
		CWindowDC dc( this );
		dc.FillSolidRect( rect, ::GetSysColor( COLOR_BTNFACE ) );
	}
}


void WorldEditorView::OnActivateView
(
	BOOL		bActivate, 
	CView		*pActivateView, 
	CView		*pDeactiveView
)
{
	BW_GUARD;

	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


BOOL WorldEditorView::OnSetCursor(CWnd *wnd, UINT, UINT)
{
	BW_GUARD;

	::SetCursor( WorldManager::instance().cursor() );
	return TRUE;
}


LRESULT WorldEditorView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	LRESULT inputResult;

	if (InputDevices::handleWindowsMessage(this->m_hWnd, message, wParam, lParam, inputResult))
	{
		if (message != WM_SYSKEYDOWN && message != WM_SYSKEYUP && message != WM_SYSCHAR)
		{
			return inputResult;
		}
	}

	return CView::WindowProc(message, wParam, lParam);
}

BW_END_NAMESPACE

