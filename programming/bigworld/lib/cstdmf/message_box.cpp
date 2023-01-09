#include "pch.hpp"
#include "message_box.hpp"
#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

namespace
{

static const int TIME_OUT_TIMER = 1;
static const int TICK_TIMER = 2;
static const INT_PTR STATUS_WND_ID = 100;

}


BW::map<HWND, MsgBox*> MsgBox::wndMap_;
BW::map<const MsgBox*, HWND> MsgBox::msgMap_;
BW::map<HWND, WNDPROC> MsgBox::buttonMap_;
HWND MsgBox::defaultParent_;

MsgBox::MsgBox( const BW::wstring& caption, const BW::wstring& text,
	const BW::wstring& buttonText1 /*= L""*/,
	const BW::wstring& buttonText2 /*= L""*/,
	const BW::wstring& buttonText3 /*= L""*/,
	const BW::wstring& buttonText4 /*= L""*/ ) :
	caption_( caption ),
	text_( text ),
	result_( TIME_OUT ),
	model_(),
	topmost_( false ),
	timeOut_( INFINITE ),
	statusWindow_( NULL ),
	savedFocus_( NULL ),
	font_( NULL ),
	tickFunction_( NULL )
{
	BW_GUARD;

	if( !buttonText1.empty() )
		buttons_.push_back( buttonText1 );
	if( !buttonText2.empty() )
		buttons_.push_back( buttonText2 );
	if( !buttonText3.empty() )
		buttons_.push_back( buttonText3 );
	if( !buttonText4.empty() )
		buttons_.push_back( buttonText4 );
}


MsgBox::MsgBox( const BW::wstring& caption, const BW::wstring& text,
	const BW::vector<BW::wstring>& buttonTexts ) :
	caption_( caption ),
	text_( text ),
	buttons_( buttonTexts ),
	result_( TIME_OUT ),
	model_(),
	topmost_( false ),
	timeOut_( INFINITE ),
	statusWindow_( NULL ),
	savedFocus_( NULL ),
	font_( NULL ),
	tickFunction_( NULL )
{}


MsgBox::~MsgBox()
{
	BW_GUARD;

	if( font_ )
		DeleteObject( font_ );
}


void MsgBox::status( const BW::wstring& status )
{
	status_ = status;

	if (statusWindow_)
	{
		SetWindowText( statusWindow_, status.c_str() );
	}
}


void MsgBox::disableButtons()
{
	HWND child = NULL;
	HWND focus = NULL;

	while ((child = FindWindowEx( msgMap_[this],
		child, L"BUTTON", NULL )))
	{
		if (child == GetFocus() && focus == NULL)
		{
			focus = child;
		}

		EnableWindow( child, FALSE );
	}

	if (focus)
	{
		savedFocus_ = focus;
	}
}


void MsgBox::enableButtons()
{
	HWND child = NULL;

	while ((child = FindWindowEx( msgMap_[this],
		child, L"BUTTON", NULL )))
	{
		EnableWindow( child, TRUE );
	}

	if (savedFocus_)
	{
		SetFocus( savedFocus_ );

		savedFocus_ = NULL;
	}
}


unsigned int MsgBox::doModal( HWND parent /* = NULL */, bool topmost /*= false*/, unsigned int timeOutTicks /*= INFINITE*/,
							 TickFunction tickFunction /*= NULL*/ )
{
	BW_GUARD;

	topmost_ = topmost;
	tickFunction_ = tickFunction;

	if( parent == NULL )
		parent = getDefaultParent();
	if( buttons_.empty() && timeOutTicks == INFINITE )
	{
		result_ = TIME_OUT;
		return result_;
	}
	model_ = true;

	DLGTEMPLATE dlg[ 2 ] = { { 0 } };
	dlg->style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
	dlg->dwExtendedStyle = 0;
	dlg->cdit = 0;
	dlg->x = 0;
	dlg->y = 0;
	dlg->cx = 100;
	dlg->cy = 100;

	timeOut_ = timeOutTicks;

	result_ = 
        (unsigned int)
        DialogBoxIndirectParam
        ( 
            GetModuleHandle( NULL ), 
            dlg, 
            parent, 
            staticDialogProc, 
            (LPARAM)this 
        );

	return getResult();
}


void MsgBox::doModalless( HWND parent /* = NULL */, unsigned int timeOutTicks /*= INFINITE*/,
						 TickFunction tickFunction /*= NULL*/ )
{
	BW_GUARD;

	topmost_ = false;
	tickFunction_ = tickFunction;

	if( parent == NULL )
		parent = getDefaultParent();
	if( buttons_.empty() && timeOutTicks == INFINITE )
	{
		result_ = TIME_OUT;
		return;
	}
	model_ = false;

	DLGTEMPLATE dlg[ 2 ] = { { 0 } };
	dlg->style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_SYSMENU;
	dlg->dwExtendedStyle = WS_EX_TOPMOST;
	dlg->cdit = 0;
	dlg->x = 0;
	dlg->y = 0;
	dlg->cx = 100;
	dlg->cy = 100;

	timeOut_ = timeOutTicks;

	CreateDialogIndirectParam( GetModuleHandle( NULL ), dlg, parent, staticDialogProc, (LPARAM)this );
}


unsigned int MsgBox::getResult() const
{
	BW_GUARD;

	if( result_ == IDOK )
		return 0;
	else if( result_ == IDCANCEL )
		return (unsigned int)buttons_.size() - 1;
	return result_ - IDCANCEL;
}


bool MsgBox::stillActive() const
{
	BW_GUARD;

	return msgMap_.find( this ) != msgMap_.end();
}


static void messageBox_CenterWindow( HWND hwnd )
{
	BW_GUARD;

	RECT parentRect;
	RECT selfRect;
	HWND parent = GetParent( hwnd );
	if( !parent )
		parent = GetDesktopWindow();
	GetWindowRect( parent, &parentRect );
	GetWindowRect( hwnd, &selfRect );
	LONG x = ( parentRect.right + parentRect.left ) / 2 - ( selfRect.right - selfRect.left ) / 2;
	LONG y = ( parentRect.bottom + parentRect.top ) / 2 - ( selfRect.bottom - selfRect.top ) / 2;
	MoveWindow( hwnd, x, y, selfRect.right - selfRect.left, selfRect.bottom - selfRect.top, TRUE );
}


INT_PTR CALLBACK MsgBox::staticDialogProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	BW_GUARD;

	if (msg == WM_INITDIALOG)
	{
		wndMap_[ hwnd ] = (MsgBox*)l;
		msgMap_[ (MsgBox*)l ] = hwnd;
	}

	return wndMap_[ hwnd ]->dialogProc( hwnd, msg, w, l );
}


INT_PTR CALLBACK MsgBox::dialogProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		create( hwnd );
		if( timeOut_ != INFINITE )
			SetTimer( hwnd, TIME_OUT_TIMER, timeOut_, NULL );
		messageBox_CenterWindow( hwnd );
		if( tickFunction_ != NULL )
		{
			if (tickFunction_( this, true ))
			{
				SetTimer( hwnd, TICK_TIMER, 1, NULL );
			}
		}

		break;
	case WM_TIMER:
		if (w == TIME_OUT_TIMER)
		{
			result_ = TIME_OUT;
			kill( hwnd );
		}
		else if (w == TICK_TIMER)
		{
			if (!tickFunction_( this, false ))
			{
				KillTimer( hwnd, TICK_TIMER );
			}
		}
		break;
	case WM_DESTROY:
		msgMap_.erase( msgMap_.find( this ) );
		wndMap_.erase( wndMap_.find( hwnd ) );
		break;
	case WM_COMMAND:
		result_ = (unsigned int)w;
		kill( hwnd );
		break;
	}
	return FALSE;
}


INT_PTR CALLBACK MsgBox::buttonProc( HWND hwnd, UINT msg, WPARAM w, LPARAM l )
{
	BW_GUARD;

	INT_PTR result = CallWindowProc( buttonMap_[ hwnd ], hwnd, msg, w, l );

	if (msg == WM_KEYDOWN && w == 'C' && GetKeyState( VK_CONTROL ))
	{
		HWND dialog = GetParent( hwnd );

		if (wndMap_.find( dialog ) != wndMap_.end())
		{
			wndMap_[ dialog ]->copyContent();
		}
	}
	else if (msg == WM_DESTROY)
	{
		SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)buttonMap_[ hwnd ] );
		buttonMap_.erase( hwnd );
	}

	return result;
}


void MsgBox::create( HWND hwnd )
{
	BW_GUARD;

	// this isn't the best layout solution, but should be
	// more than enough for now

	// 0. preparation
	SetWindowText( hwnd, caption_.c_str() );
	font_ = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

	// 1. const definition
	static const int VERTICAL_MARGIN = 10, HORIZONTAL_MARGIN = 10;
	static const int MIN_BUTTON_WIDTH = 81, BUTTON_HEIGHT = 21;
	static const int MIN_BUTTON_MARGIN = 10;
	static const int MIN_BUTTON_SPACE = 10;
	static const int MIN_DIALOG_WIDTH = 324;

	const double PHI = 1.618;

	// 2. static size and status size
	HWND wndStatic = CreateWindow( L"STATIC", text_.c_str(), WS_CHILD | WS_VISIBLE,
		0, 0, 10, 10, hwnd, NULL, GetModuleHandle( NULL ), NULL );
	SendMessage( wndStatic, WM_SETFONT, (WPARAM)font_, FALSE );

	int minimumStaticWidth = MIN_DIALOG_WIDTH - 2 * HORIZONTAL_MARGIN;
	int staticHeight = 0, staticWidth = minimumStaticWidth;

	HDC clientDC = GetDC( wndStatic );
	HFONT fontSaved = (HFONT)SelectObject( clientDC, font_ );

	BW::wstring s = text_;
	if( s.find( L'\n' ) == s.npos )
		s += L"\n.";

	for(;;)
	{
		RECT rect = { 0, 0, staticWidth, staticHeight };
		DrawText( clientDC, s.c_str(), -1, &rect, DT_CALCRECT | DT_WORDBREAK );

		if (staticWidth < rect.right)
		{
			minimumStaticWidth = rect.right;
		}
		else if (rect.bottom <= staticHeight)
		{
			break;
		}

		staticHeight += 20;
		staticWidth = int( staticHeight * PHI );

		if (staticWidth < minimumStaticWidth)
		{
			staticWidth = minimumStaticWidth;
		}
	}

	/*
	BW::string temp = s;
	
	char* resToken = strtok( (char*)temp.c_str(), "\n" );
	while ( resToken != 0 )
	{
		RECT rect = { 0, 0, 0, 0 };
		DrawText( clientDC, resToken, -1, &rect, DT_CALCRECT );
		if (staticWidth < rect.right - rect.left)
			staticWidth = rect.right - rect.left;
		resToken = strtok( 0, "\n" );
	}
	{
		RECT rect = { 0, 0, staticWidth, 512 };
		DrawText( clientDC, s.c_str(), -1, &rect, DT_CALCRECT );
		staticHeight = rect.bottom - rect.top;
	}
	*/

	int statusHeight = 0, statusWidth = 0;
	
	if (!status_.empty())
	{
		RECT rect = { 0, 0, staticWidth, staticHeight };

		statusWindow_ = CreateWindow(L"STATIC", status_.c_str(), WS_CHILD | WS_VISIBLE,
			0, 0, 10, 10, hwnd,
			reinterpret_cast<HMENU>(STATUS_WND_ID),
			GetModuleHandle(NULL), NULL);
		SendMessage( statusWindow_, WM_SETFONT, (WPARAM)font_, FALSE );

		DrawText( clientDC, status_.c_str(), -1, &rect, DT_CALCRECT );

		statusHeight = rect.bottom;
		statusWidth = rect.right;

		if (staticWidth < statusWidth)
		{
			staticWidth = statusWidth;
		}
	}

	SelectObject( clientDC, fontSaved );
	ReleaseDC( wndStatic, clientDC );

	// 3. button size
	HWND wndButton = NULL;
	int buttonWidth = MIN_BUTTON_WIDTH;
	if( !buttons_.empty() )
	{
		wndButton = CreateWindow( L"BUTTON", buttons_[0].c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
			0, 0, 10, 10, hwnd, (HMENU)IDOK, GetModuleHandle( NULL ), NULL );
		SendMessage( wndButton, WM_SETFONT, (WPARAM)font_, FALSE );
		SetFocus( wndButton );

		HDC clientDC = GetDC( wndButton );
		fontSaved = (HFONT)SelectObject( clientDC, font_ );

		for( BW::vector<BW::wstring>::iterator iter = buttons_.begin();
			iter != buttons_.end(); ++iter )
		{
			RECT rect = { 0, 0, buttonWidth, BUTTON_HEIGHT };
			DrawText( clientDC, iter->c_str(), -1, &rect, DT_CALCRECT );
			if( buttonWidth < rect.right + 2 * MIN_BUTTON_MARGIN )
				buttonWidth = rect.right + 2 * MIN_BUTTON_MARGIN;
		}

		SelectObject( clientDC, fontSaved );
		ReleaseDC( wndButton, clientDC );
	}

	// 4. dialog size
	int dialogWidth = MIN_DIALOG_WIDTH, dialogHeight;
	int buttonSpace = MIN_BUTTON_SPACE;

	if( dialogWidth < staticWidth + 2 * HORIZONTAL_MARGIN )
		dialogWidth = staticWidth + 2 * HORIZONTAL_MARGIN;

	if( buttonSpace * ( buttons_.size() + 1 ) + buttonWidth * buttons_.size()
		< unsigned( dialogWidth - HORIZONTAL_MARGIN * 2 ) )
		buttonSpace = ( dialogWidth - HORIZONTAL_MARGIN * 2 - buttonWidth * (int)buttons_.size() )
			/ ( (int)buttons_.size() + 1 );
	else
		dialogWidth = buttonSpace * ( (int)buttons_.size() + 1 ) + buttonWidth * (int)buttons_.size()
			+ HORIZONTAL_MARGIN * 2;

	if( dialogWidth > staticWidth + 2 * HORIZONTAL_MARGIN )
		staticWidth = dialogWidth - 2 * HORIZONTAL_MARGIN;

	if( buttons_.empty() )
		dialogHeight = 2 * VERTICAL_MARGIN + staticHeight;
	else
		dialogHeight = 3 * VERTICAL_MARGIN + staticHeight + BUTTON_HEIGHT;

	if( !status_.empty() )
	{
		dialogHeight += VERTICAL_MARGIN + statusHeight;
	}

	// 5. go
	RECT rect = { 0, 0, dialogWidth, dialogHeight};
	AdjustWindowRect( &rect, GetWindowLong( hwnd, GWL_STYLE ), FALSE );
	rect.right -= rect.left;
	rect.bottom -= rect.top;

	HWND parentWnd = GetParent( hwnd );
	if( GetWindowLong( parentWnd, GWL_STYLE ) & WS_MINIMIZE )
		ShowWindow( parentWnd, SW_RESTORE );
	RECT parentRect;
	GetWindowRect( parentWnd, &parentRect );

	rect.left = ( parentRect.right - parentRect.left - rect.right ) / 2;
	rect.top = ( parentRect.bottom - parentRect.top - rect.bottom ) / 2;

	MoveWindow( wndStatic, HORIZONTAL_MARGIN, VERTICAL_MARGIN,
		staticWidth, staticHeight, FALSE );

	if (statusWindow_)
	{
		MoveWindow( statusWindow_, HORIZONTAL_MARGIN, VERTICAL_MARGIN * 2 + staticHeight,
			staticWidth, statusHeight, FALSE );

		staticHeight += statusHeight + VERTICAL_MARGIN;
	}

	if( buttons_.size() )
	{
		int buttonY = VERTICAL_MARGIN + staticHeight + VERTICAL_MARGIN;
		int buttonX = HORIZONTAL_MARGIN + buttonSpace;
		int i = 0;

		for( BW::vector<BW::wstring>::iterator iter = buttons_.begin();
			iter != buttons_.end(); ++iter )
		{
			if( iter != buttons_.begin() )
			{
				wndButton = CreateWindow( L"BUTTON", iter->c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					buttonX, buttonY, buttonWidth, BUTTON_HEIGHT, hwnd, 
					(HMENU)( iter + 1 == buttons_.end() ? IDCANCEL : std::distance( buttons_.begin(), iter ) + IDCANCEL ),
					GetModuleHandle( NULL ), NULL );
				SendMessage( wndButton, WM_SETFONT, (WPARAM)font_, FALSE );
			}
			else
			{
				MoveWindow( wndButton, buttonX, buttonY,
					buttonWidth, BUTTON_HEIGHT, FALSE );
			}

			buttonX += buttonSpace + buttonWidth;

			buttonMap_[ wndButton ] = (WNDPROC)SetWindowLongPtr( wndButton, GWLP_WNDPROC, (LONG_PTR)buttonProc ); 
		}
	}

	MoveWindow( hwnd, rect.left, rect.top, rect.right, rect.bottom, TRUE );
	if( topmost_ )
		SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

	return;
}


void MsgBox::kill( HWND hwnd )
{
	BW_GUARD;

	if( model_ )
		EndDialog( hwnd, result_ );
	else
		DestroyWindow( hwnd );
}


void MsgBox::copyContent() const
{
	BW_GUARD;

	if (OpenClipboard( NULL ))
	{
		BW::wstring content( L"[Window Title]\r\n" );
		
		content += caption_;
		content += L"\r\n\r\n";

		content += L"[Content]\r\n";
		content += text_;
		content += L"\r\n\r\n";

		for (BW::vector<BW::wstring>::const_iterator iter = buttons_.begin();
			iter != buttons_.end(); ++iter)
		{
			if (iter != buttons_.begin())
			{
				content += ' ';
			}

			content += L'[';
			content += *iter;
			content += L']';
		}

		HGLOBAL mem = GlobalAlloc( GMEM_MOVEABLE, ( content.size() + 1 ) * sizeof( wchar_t ) );

		if (mem)
		{
			LPVOID pv = GlobalLock( mem );

			if (pv)
			{
				memcpy( pv, content.c_str(), ( content.size() + 1 ) * sizeof( wchar_t ) );

				GlobalUnlock( mem );

				SetClipboardData( CF_UNICODETEXT, mem );
			}
		}

		CloseClipboard();

		if (mem)
		{
			GlobalFree( mem );
		}
	}
}

BW_END_NAMESPACE
