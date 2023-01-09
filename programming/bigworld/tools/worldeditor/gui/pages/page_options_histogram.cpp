#include "pch.hpp"
#include "worldeditor/gui/pages/page_options_histogram.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/Watcher.hpp"
#include "romp/histogram_provider.hpp"
#include "romp/time_of_day.hpp"

DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

BW_BEGIN_NAMESPACE

// PageOptionsHistogram

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const BW::wstring PageOptionsHistogram::contentID = L"PageOptionsHistogram";


PageOptionsHistogram::PageOptionsHistogram()
	: CDialog(PageOptionsHistogram::IDD),
	userAdded_( false ),
	width_( 0 ),
	height_( 0 ),
	pBmpBits_( NULL ),
	lastBmpBitsSize_( 0 )
{
}

PageOptionsHistogram::~PageOptionsHistogram()
{
	BW_GUARD;

	raw_free( pBmpBits_ );
}

void PageOptionsHistogram::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RED, mRed);
	DDX_Control(pDX, IDC_GREEN, mGreen);
	DDX_Control(pDX, IDC_BLUE, mBlue);
	DDX_Control(pDX, IDC_RANGERATIO, mRangeRatio);
	DDX_Control(pDX, IDC_RANGERATIOSLIDER, mRangeRatioSlider);
}

BOOL PageOptionsHistogram::OnInitDialog()
{
	BW_GUARD;

	CDialog::OnInitDialog();
	INIT_AUTO_TOOLTIP();

	// save original sizes
	CRect dlgRect;
	GetWindowRect( &dlgRect );

	width_ = dlgRect.Width();
	height_ = dlgRect.Height();

	CWnd* win = GetDlgItem( IDC_LUMINANCE );
	CRect rect;
	win->GetWindowRect( &rect );
	lumHeight_ = rect.Height();

	win = GetDlgItem( IDC_RGB );
	win->GetWindowRect( &rect );
	rgbHeight_ = rect.Height();
	rgbPos_ = rect.top - dlgRect.top;

	win = GetDlgItem( IDC_STATIC_RGB );
	win->GetWindowRect( &rect );
	rgbLabelPos_ = rect.top - dlgRect.top;

	win = GetDlgItem( IDC_RED );
	win->GetWindowRect( &rect );
	buttonsPos_ = rect.top - dlgRect.top;
	redBtnPos_ = rect.left - dlgRect.left;
	win = GetDlgItem( IDC_GREEN );
	win->GetWindowRect( &rect );
	greenBtnPos_ = rect.left - dlgRect.left;
	win = GetDlgItem( IDC_BLUE );
	win->GetWindowRect( &rect );
	blueBtnPos_ = rect.left - dlgRect.left;

	win = GetDlgItem( IDC_RANGERATIOSLIDER );
	win->GetWindowRect( &rect );
	sliderPos_ = rect.top - dlgRect.top;
	sliderWidth_ = rect.Width();

	mRed.SetCheck( TRUE );
	mGreen.SetCheck( TRUE );
	mBlue.SetCheck( TRUE );

	mRangeRatio.SetWindowText( L"1" );
	mRangeRatioSlider.SetRangeMin( 1 );
	mRangeRatioSlider.SetRangeMax( 16 );
	mRangeRatioSlider.SetPos( 1 );

	OnUpdateControls(0, 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(PageOptionsHistogram, CDialog)
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_WM_HSCROLL()
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_RED, OnBnClickedRed)
	ON_BN_CLICKED(IDC_GREEN, OnBnClickedGreen)
	ON_BN_CLICKED(IDC_BLUE, OnBnClickedBlue)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_RANGERATIOSLIDER, OnNMCustomdrawRangeratioslider)
END_MESSAGE_MAP()


afx_msg void PageOptionsHistogram::OnShowWindow( BOOL bShow, UINT nStatus )
{
	BW_GUARD;

	CDialog::OnShowWindow( bShow, nStatus );
	
	if ( bShow == FALSE )
	{
		if ( userAdded_ )
		{
			HistogramProvider::instance().removeUser();
			userAdded_ = false;
		}
	}
	else
	{
		if ( !userAdded_ )
		{
			HistogramProvider::instance().addUser();
			userAdded_ = true;
			RedrawWindow();
		}
	}
}

afx_msg void PageOptionsHistogram::OnSize( UINT nType, int cx, int cy )
{
	BW_GUARD;

	CDialog::OnSize( nType, cx, cy );

	if ( width_ == 0 && height_ == 0 )
		return; // dialog not initialised

	CRect dlgRect;
	GetWindowRect( &dlgRect );

	CWnd* win = GetDlgItem( IDC_LUMINANCE );
	CRect rect;
	int dy = ( cy - height_ )/2;
	win->SetWindowPos( 0, 0, 0, cx, lumHeight_ + dy, SWP_NOZORDER|SWP_NOMOVE );

	win = GetDlgItem( IDC_STATIC_RGB );
	win->GetWindowRect( &rect );
	win->SetWindowPos( 0, rect.left - dlgRect.left, rgbLabelPos_ + dy, 0, 0, SWP_NOZORDER|SWP_NOSIZE );

	win = GetDlgItem( IDC_RGB );
	win->SetWindowPos( 0, 0, rgbPos_ + dy, cx, rgbHeight_ + dy, SWP_NOZORDER );
	dy *= 2;

	int dx = ( cx - width_ ) / 2;
	win = GetDlgItem( IDC_RED );
	win->SetWindowPos( 0, redBtnPos_, buttonsPos_ + dy, 0, 0, SWP_NOZORDER|SWP_NOSIZE );

	win = GetDlgItem( IDC_GREEN );
	win->SetWindowPos( 0, greenBtnPos_, buttonsPos_ + dy, 0, 0, SWP_NOZORDER|SWP_NOSIZE );

	win = GetDlgItem( IDC_BLUE );
	win->SetWindowPos( 0, blueBtnPos_, buttonsPos_ + dy, 0, 0, SWP_NOZORDER|SWP_NOSIZE );

	dx = cx - width_;
	win = GetDlgItem( IDC_RANGERATIOSLIDER );
	win->GetWindowRect( &rect );
	win->SetWindowPos( 0, rect.left - dlgRect.left, sliderPos_ + dy, sliderWidth_ + dx, rect.Height(), SWP_NOZORDER );

	win = GetDlgItem( IDC_STATIC_RANGE );
	win->GetWindowRect( &rect );
	win->SetWindowPos( 0, rect.left - dlgRect.left, sliderPos_ + dy + 3, 0, 0, SWP_NOZORDER|SWP_NOSIZE );

	win = GetDlgItem( IDC_RANGERATIO );
	win->GetWindowRect( &rect );
	win->SetWindowPos( 0, rect.left - dlgRect.left, sliderPos_ + dy + 3, 0, 0, SWP_NOZORDER|SWP_NOSIZE );

	RedrawWindow();
}

afx_msg LRESULT PageOptionsHistogram::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	if( IsWindowVisible() && userAdded_ )
	{
		// Recalc buffer size and do some initialisation
		RECT lumRect;
		GetDlgItem( IDC_LUMINANCE )->GetClientRect( &lumRect );
		DWORD lumPitch = ( ( lumRect.right * 24 + 31 ) & ( ~31 ) ) / 8;

		RECT rgbRect;
		GetDlgItem( IDC_RGB )->GetClientRect( &rgbRect );
		DWORD rgbPitch = ( ( rgbRect.right * 24 + 31 ) & ( ~31 ) ) / 8;

		int ratio = mRangeRatioSlider.GetPos();

		int lumBitsSize = lumPitch * lumRect.bottom;
		int rgbBitsSize = rgbPitch * rgbRect.bottom;
		int neededBmpBitsSize = std::max( lumBitsSize, rgbBitsSize );

		if (!pBmpBits_ || lastBmpBitsSize_ != neededBmpBitsSize )
		{
			raw_free( pBmpBits_ );
			if (neededBmpBitsSize > 0)
			{
				pBmpBits_ = (BYTE*)raw_malloc( neededBmpBitsSize );
			}
			else
			{
				pBmpBits_ = NULL;
			}
			lastBmpBitsSize_ = neededBmpBitsSize;
		}
		
		// Draw Luminance
		if (pBmpBits_)
		{
			CClientDC dc( GetDlgItem( IDC_LUMINANCE ) );

			memset( pBmpBits_, 0, lumBitsSize );

			BITMAPINFOHEADER bmpInfo = {
				sizeof( bmpInfo ), lumRect.right, lumRect.bottom, 1, 24, BI_RGB, 0, 0, 0, 0, 0 };

			HistogramProvider::Histogram hist =
				HistogramProvider::instance().get( HistogramProvider::HT_LUMINANCE );
			unsigned int max = 0;
			for( unsigned int i = 0; i < HistogramProvider::HISTOGRAM_LEVEL; ++i )
			{
				if( hist.value_[ i ] > max )
				{
					max = hist.value_[ i ];
				}
			}
			int localRatio = ratio * lumRect.bottom;

			BW::vector<int> indices( lumRect.right );
			for( int i = 0; i < lumRect.right; ++i )
			{
				indices[ i ] = i * HistogramProvider::HISTOGRAM_LEVEL / lumRect.right;
			}

			for( int i = 0; i < lumRect.bottom; ++i )
			{
				BYTE* color = pBmpBits_ + lumPitch * i;
				unsigned int level = i * max / localRatio;
				for( int j = 0; j < lumRect.right; ++j )
				{
					int index = indices[ j ];
					if( level <= hist.value_[ index ] )
					{
						color[ 0 ] = 0xff, color[ 1 ] = 0xff, color[ 2 ] = 0xff;
					}
					color += 3;
				}
			}

			SetDIBitsToDevice( dc.m_hDC, 0, 0, lumRect.right, lumRect.bottom, 0, 0, 0, lumRect.bottom,
				pBmpBits_, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS );
		}
		
		// Draw RGB
		if (pBmpBits_)
		{
			CClientDC dc( GetDlgItem( IDC_RGB ) );
	
			memset( pBmpBits_, 0, rgbBitsSize );

			BOOL R = mRed.GetCheck(),
				G = mGreen.GetCheck(),
				B = mBlue.GetCheck();

			BITMAPINFOHEADER bmpInfo = {
				sizeof( bmpInfo ), rgbRect.right, rgbRect.bottom, 1, 24, BI_RGB, 0, 0, 0, 0, 0 };

			HistogramProvider::Histogram red =
				HistogramProvider::instance().get( HistogramProvider::HT_R );
			HistogramProvider::Histogram green =
				HistogramProvider::instance().get( HistogramProvider::HT_G );
			HistogramProvider::Histogram blue =
				HistogramProvider::instance().get( HistogramProvider::HT_B );
			unsigned int max = 0;
			for( unsigned int i = 0; i < HistogramProvider::HISTOGRAM_LEVEL; ++i )
			{
				if( red.value_[ i ] > max && R )
					max = red.value_[ i ];
				if( green.value_[ i ] > max && G )
					max = green.value_[ i ];
				if( blue.value_[ i ] > max && B )
					max = blue.value_[ i ];
			}
			BW::vector<int> indices( rgbRect.right );
			for( int i = 0; i < rgbRect.right; ++i )
			{
				indices[ i ] = i * HistogramProvider::HISTOGRAM_LEVEL / rgbRect.right;
			}

			int localRatio = ratio * rgbRect.bottom;
			if( R && G && B )
			{
				for( int i = 0; i < rgbRect.bottom; ++i )
				{
					BYTE* color = pBmpBits_ + rgbPitch * i;
					unsigned int level = i * max / localRatio;
					for( int j = 0; j < rgbRect.right; ++j )
					{
						int index = indices[ j ];

						if( level <= blue.value_[ index ] )
						{
							*color = 255;
						}
						++color;

						if( level <= green.value_[ index ] )
						{
							*color = 255;
						}
						++color;

						if( level <= red.value_[ index ] )
						{
							*color = 255;
						}
						++color;
					}
				}
			}
			else
			{
				for( int i = 0; i < rgbRect.bottom; ++i )
				{
					BYTE* color = pBmpBits_ + rgbPitch * i;
					unsigned int level = i * max / localRatio;
					for( int j = 0; j < rgbRect.right; ++j )
					{
						int index = indices[ j ];

						if( B && level <= blue.value_[ index ] )
						{
							*color = 255;
						}
						++color;

						if( G && level <= green.value_[ index ] )
						{
							*color = 255;
						}
						++color;

						if( R && level <= red.value_[ index ] )
						{
							*color = 255;
						}
						++color;
					}
				}
			}

			SetDIBitsToDevice( dc.m_hDC, 0, 0, rgbRect.right, rgbRect.bottom, 0, 0, 0, rgbRect.bottom,
				pBmpBits_, (BITMAPINFO*)&bmpInfo, DIB_RGB_COLORS );
		}
	}

	return 0;
}

void PageOptionsHistogram::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	BW_GUARD;

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageOptionsHistogram::OnDrawItem( int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct )
{
	if( nIDCtl == IDC_LUMINANCE )
	{
	}
	else if( nIDCtl == IDC_RGB )
	{
	}
}

void PageOptionsHistogram::OnBnClickedRed()
{
}

void PageOptionsHistogram::OnBnClickedGreen()
{
}

void PageOptionsHistogram::OnBnClickedBlue()
{
}

void PageOptionsHistogram::OnNMCustomdrawRangeratioslider(NMHDR *pNMHDR, LRESULT *pResult)
{
	BW_GUARD;

	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);

	CString s;
	s.Format( L"%d", mRangeRatioSlider.GetPos() );
	mRangeRatio.SetWindowText( s );

	*pResult = 0;
}
BW_END_NAMESPACE

