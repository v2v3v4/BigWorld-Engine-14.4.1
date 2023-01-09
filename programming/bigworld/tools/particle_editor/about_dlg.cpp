#include "pch.hpp"
#include "about_dlg.hpp"
#include "common/compile_time.hpp"
#include "common/tools_common.hpp"

#include "cstdmf/bwversion.hpp"

#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

CAboutDlg::CAboutDlg() 
: 
CDialog(CAboutDlg::IDD),
m_background()
{
	BW_GUARD;

    m_background.LoadBitmap( IDB_ABOUT );
    m_buildFont.CreatePointFont( 90, L"Arial", NULL );
	m_copyrightFont.CreatePointFont( 70, L"Arial", NULL );
}

BOOL CAboutDlg::OnInitDialog()
{
	BW_GUARD;

    CDialog::OnInitDialog();

    BITMAP bitmap;
    m_background.GetBitmap( &bitmap );
    RECT rect = { 0, 0, bitmap.bmWidth, bitmap.bmHeight };
    AdjustWindowRect( &rect, GetWindowLong( m_hWnd, GWL_STYLE ), FALSE );
    MoveWindow( &rect, FALSE );
    CenterWindow();
    SetCapture();

    return TRUE;  // Don't set focus to first control
}

void CAboutDlg::OnPaint()
{
	BW_GUARD;

	CPaintDC dc(this); // device context for painting
	CDC memDC;
	memDC.CreateCompatibleDC( &dc);
	CBitmap* saveBmp = memDC.SelectObject( &m_background );

	RECT client;
	GetClientRect( &client );

    dc.BitBlt(0, 0, client.right, client.bottom, &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject( &saveBmp );
    
	BW::wstring space = L" ";
	CString builtOn = Localise(L"PARTICLEEDITOR/GUI/ABOUT_BOX/VERSION_BUILT", BWVersion::versionString().c_str(),
		ToolsCommon::isEval() ? space + Localise(L"PARTICLEEDITOR/GUI/ABOUT_BOX/EVAL" ) : L"",
#ifdef _DEBUG
		space + Localise(L"PARTICLEEDITOR/GUI/ABOUT_BOX/DEBUG" ),
#else
		"",
#endif
		aboutCompileTimeString );

    dc.SetBkMode( TRANSPARENT );
	dc.SetTextColor( 0x00808080 );
	CFont* saveFont = dc.SelectObject( &m_buildFont );
	dc.ExtTextOut( 72, 290, 0, NULL, builtOn, NULL );

	dc.SelectObject( &m_copyrightFont );
	dc.ExtTextOut( 72, 366, 0, NULL, TEXT( BW_COPYRIGHT_NOTICE ), NULL );
	dc.SelectObject( &saveFont );
}

void CAboutDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

    CDialog::OnLButtonDown(nFlags, point);
    OnOK();
}

void CAboutDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

    CDialog::OnRButtonDown(nFlags, point);
    OnOK();
}
BW_END_NAMESPACE

