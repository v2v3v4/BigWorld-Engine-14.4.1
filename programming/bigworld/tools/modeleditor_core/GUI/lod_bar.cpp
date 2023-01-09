#include "pch.hpp"

#include "resmgr/string_provider.hpp"

#include "me_app.hpp"
#include "me_consts.hpp"
#include "model/model.hpp"

#include "resource.h"

#include "lod_bar.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

BEGIN_MESSAGE_MAP(CLodBar, CWnd)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_WM_PAINT()
END_MESSAGE_MAP()


CLodBar::CLodBar( CWnd* parent ):
	CWnd(),
	parent_(parent),
	mouseInBar_(false),
	mover_(-1),
	maxRange_(1.f),
	locked_( true )
{
	BW_GUARD;

	CRect rect;
	parent->GetWindowRect (&rect);

	width_ = rect.Width();
	height_ = rect.Height();

	this->CreateEx( 0, AfxRegisterWndClass(CS_OWNDC), L"", WS_VISIBLE | WS_CHILD,
		CRect( 0, 0, width_, height_ ),
		parent, 0, 0);

	//Load the patterns we will be using
	lodBmp[0].LoadBitmap( IDB_LOD_PAT_1 );
	lodBmp[1].LoadBitmap( IDB_LOD_PAT_2 );
	lodBmp[2].LoadBitmap( IDB_LOD_PAT_3 );
	
	for (int i=0; i<numPatterns; i++)
	{
		lodPat[i].CreatePatternBrush( &lodBmp[i] );
	}

	//Get the mutants LOD list so we can play with it
	lodList_ = MeApp::instance().mutant()->lodList();

	redraw();
}

CLodBar::~CLodBar()
{
}

void CLodBar::setWidth( int w )
{
	BW_GUARD;

	CRect rect;
	GetWindowRect (&rect);
	ScreenToClient( &rect );
	parent_->SetWindowPos( 0, 0, 0, rect.left + w, rect.Height(), SWP_NOMOVE | SWP_NOZORDER );
	SetWindowPos( 0, 0, 0, rect.left + w, rect.Height(), SWP_NOMOVE | SWP_NOZORDER );
	width_ = w;
	redraw();
}


int CLodBar::mover( int pos )
{
	BW_GUARD;

	int moverIndex = -1;
	int moverDist = 100000;

	for (unsigned i=0; i<lodList_->size(); i++)
	{
		float testDist = (*lodList_)[i].second;
		int testPos = distToPos( testDist );

		// -1.f means infinity
		if (testDist != -1.f)
		{
			int delta = abs( pos - testPos );
			if ((delta <= 8) && (delta <= moverDist))
			{
				moverIndex = i;
				moverDist = delta;
			}
		}
	}

	if ((!locked_) || (moverIndex <= 0))
	{
		return moverIndex;
	}
	else
	{
		return -1;
	}
}

	
void CLodBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

	SetCapture();
	
	mover_ = mover( point.x );

	if (mover_ != -1)
	{
		SetCursor( LoadCursor( NULL, IDC_SIZEWE ));
	}
}

void CLodBar::OnMouseMove(UINT nFlags, CPoint point)
{
	BW_GUARD;

	if (mover_ == -1)
	{
		SetCursor( LoadCursor( NULL, (mover( point.x ) != -1) ? IDC_SIZEWE : IDC_ARROW ));
	}

	if (!mouseInBar_)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = this->GetSafeHwnd();
		mouseInBar_ = ::_TrackMouseEvent(&tme) == TRUE;
	}
	
	//Make sure we are currently capturing
	if (GetCapture() != this)
		return;

	static int s_lastDist = -1;
	float lodDist = posToDist( point.x );
	if ((mover_ != -1) && ( lodDist != s_lastDist))
	{
		if (point.x >= 50000 * distToPos_) return;
		
		(*lodList_)[mover_].second = lodDist;
		MeApp::instance().mutant()->triggerUpdate( "LOD" );
		s_lastDist = (int)lodDist;
	}
}

void CLodBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	BW_GUARD;

	ReleaseCapture();

	if (mover_ != -1)
	{
		SetCursor( LoadCursor( NULL, (mover( point.x ) != -1) ? IDC_SIZEWE : IDC_ARROW ));
		MeApp::instance().mutant()->lodList( lodList_ ); // Commit the new LOD list
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/GUI/LOD_BAR/CHANGING_MODEL_EXTENT"), false );
		mover_ = -1;
	}
}

LRESULT CLodBar::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	BW_GUARD;

	ReleaseCapture();
	SetCursor( LoadCursor( NULL, IDC_ARROW ));
	mouseInBar_ = false;
	return TRUE;
}

void CLodBar::OnPaint()
{
	BW_GUARD;

	redraw();
	CWnd::OnPaint();
}

void CLodBar::redraw()
{
	BW_GUARD;

	CWindowDC dc ( this );

	dc.SetDCBrushColor( 0x00000000 );
	dc.SetDCPenColor( 0x00000000 );

	CRect rect(0,0,width_,height_);
	CBrush whiteBrush( COLORREF( 0x00FFFFFF ));
	CBrush blackBrush( COLORREF( 0x00000000 ));

	maxRange_ = 0.f;
	for (LODList::size_type i = 0; i < lodList_->size(); ++i)
	{
		if (isEqual( (*lodList_)[i].second, Model::LOD_HIDDEN ))
		{
			break;
		}

		if ((*lodList_)[i].second > maxRange_)
		{
			maxRange_ = (*lodList_)[i].second;
		}
	}
	maxRange_ += 10.f;

	posToDist_ = maxRange_ / width_;
	distToPos_ = width_ / maxRange_;

	int left = 0;
	int right = 0;

	for (LODList::size_type i = 0; i < lodList_->size(); ++i)
	{
		if (!isEqual( (*lodList_)[i].second, Model::LOD_HIDDEN) )
		{
			right = (int)(width_ * ((*lodList_)[i].second / maxRange_));
		}
		else
		{
			right = width_;
		}

		if (left < 0)
		{
			left = 0;
		}

		if (right < left)
		{
			right = left;
		}

		CRect test(left,0,right,height_);
		dc.FillRect(test, &lodPat[i%3]);
		dc.FrameRect(test, &blackBrush);

		left = right - 1;
	}

	if (left < 0)
	{
		left = 0;
	}
		
	//Fill in any remaining space with white
	if (right < width_)
	{
		CRect test(left,0,width_,height_);
		dc.FillRect(test, &whiteBrush);
		dc.FrameRect(test, &blackBrush);
	}

	left = 0;

	wchar_t buf[32];

	for (LODList::size_type i = 0; i < lodList_->size(); ++i)
	{
		if (!isEqual( (*lodList_)[i].second, Model::LOD_HIDDEN ))
		{
			right = (int)(width_ * ((*lodList_)[i].second / maxRange_));
		}
		else
		{
			right = width_;
		}

		if (left < 0)
		{
			left = 0;
		}
		
		if (right < left)
		{
			right = left;
		}
		
		int mid = (left + right) / 2;
		if (mid < 6)
		{
			mid = 6;
		}
		if (mid > width_ - 6)
		{
			mid = width_ - 6;
		}
			
		CRect test(mid - 6, height_/2 - 8, mid + 6, height_/2 + 8);
		dc.FillRect(test, &whiteBrush);
		dc.FrameRect(test, &blackBrush);

		dc.SetBkMode( TRANSPARENT );
		bw_snwprintf( buf, ARRAY_SIZE(buf), L"%d", i );
		dc.ExtTextOut( mid - 4, height_/2 - 8, 0, NULL, buf, NULL );

		left = right - 1;
	}
}

BW_END_NAMESPACE

