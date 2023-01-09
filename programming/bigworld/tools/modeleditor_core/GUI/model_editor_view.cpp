// ModelEditorView.cpp : implementation of the CModelEditorView class
//

#include "pch.hpp"
#include "main_frm.h"

#include "model_editor_doc.h"
#include "model_editor_view.h"

#include "common/cooperative_moo.hpp"
#include "moo/render_context.hpp"
#include "input/input.hpp"
#include "i_model_editor_app.hpp"

DECLARE_DEBUG_COMPONENT2( "ModelEditor", 0 )

BW_BEGIN_NAMESPACE

// CModelEditorView

IMPLEMENT_DYNCREATE(CModelEditorView, CView)

BEGIN_MESSAGE_MAP(CModelEditorView, CView)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

// CModelEditorView construction/destruction

CModelEditorView::CModelEditorView()
	: lastRect_( 0, 0, 0, 0 )
{
	// TODO: add construction code here

}

CModelEditorView::~CModelEditorView()
{
}

BOOL CModelEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	BW_GUARD;

   cs.lpszClass = AfxRegisterWndClass(
	   CS_OWNDC | CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW), 0 );
   cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
   cs.style &= ~WS_BORDER;

   return CView::PreCreateWindow(cs);
} 

void CModelEditorView::OnSize(UINT nType, int cx, int cy)
{
	BW_GUARD;

	CView::OnSize( nType, cx, cy );

//	No longer changing Moo mode here, since it's too slow
	//if ((cx > 0 && cy > 0) && Moo::rc().device() && Moo::rc().windowed())
	//{
	//	Moo::rc().changeMode(Moo::rc().modeIndex(), Moo::rc().windowed(), true);
	//}
}

void CModelEditorView::OnPaint()
{
	BW_GUARD;

	CView::OnPaint();

	CRect rect;
	GetClientRect( &rect );

	// changing mode when a paint message is received and the size of the
	// window is different than last stored size.
	bool resize = editorApp_->initDone() &&
		( lastRect_ != rect &&
		Moo::rc().device() && Moo::rc().windowed() &&
		rect.Width() && rect.Height() &&
		!((CMainFrame*) editorApp_->mainWnd())->resizing() );

	if (editorApp_->initDone() == false ||
		paint( resize ) == false)
	{
		lastRect_ = rect;
		CWindowDC dc( this );
		dc.FillSolidRect( rect, ::GetSysColor( COLOR_BTNFACE ) );
	}
} 

void CModelEditorView::OnKillFocus( CWnd *pNewWnd )
{
	BW_GUARD;

	// Make sure we flush any input events to avoid stuck keys.
	InputDevices::setFocus( false, NULL );
	CView::OnKillFocus( pNewWnd );
}

// CModelEditorView drawing

void CModelEditorView::OnDraw(CDC* /*pDC*/)
{
	BW_GUARD;

	CModelEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}


// CModelEditorView diagnostics

#ifdef _DEBUG
void CModelEditorView::AssertValid() const
{
	BW_GUARD;

	CView::AssertValid();
}

void CModelEditorView::Dump(CDumpContext& dc) const
{
	BW_GUARD;

	CView::Dump(dc);
}

CModelEditorDoc* CModelEditorView::GetDocument() const // non-debug version is inline
{
	BW_GUARD;

	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CModelEditorDoc)));
	return (CModelEditorDoc*)m_pDocument;
}
#endif //_DEBUG


LRESULT CModelEditorView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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


void CModelEditorView::resizeWindow()
{
	CWaitCursor wait;
	GLView::resizeWindow();
}

BW_END_NAMESPACE


// CModelEditorView message handlers
