
#include "pch.hpp"
#include "drag_image.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{


DragImage::DragImage( CImageList & imgList, const CPoint & pos, const CPoint & offset ) :
	imgList_( imgList ),
	offset_( offset )
{
	BW_GUARD;

	IMAGEINFO imgInfo;
	imgList.GetImageInfo( 0, &imgInfo );
	rect_ = imgInfo.rcImage;
	rect_.OffsetRect( -rect_.TopLeft() );
	rect_.OffsetRect( pos );

	hwnd_ = CreateWindowEx( WS_EX_LAYERED | // Layered Windows
				WS_EX_TRANSPARENT | // Don't hittest this window
				WS_EX_TOPMOST | WS_EX_TOOLWINDOW, 
				AfxRegisterWndClass( NULL ), L"BWLayeredImage",
				WS_POPUP | WS_VISIBLE,
				rect_.left - offset_.x, rect_.top - offset_.y, rect_.Width(), rect_.Height(),
				NULL, (HMENU)0, AfxGetInstanceHandle(), NULL );
	this->update( rect_.TopLeft() );
}


DragImage::~DragImage()
{
	BW_GUARD;

	DestroyWindow( hwnd_ );
}


void DragImage::update( const CPoint & pos, BYTE alpha )
{
	BW_GUARD;

	SetWindowPos( hwnd_, 0, pos.x - offset_.x, pos.y - offset_.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER );

	rect_.OffsetRect( -rect_.TopLeft() );
	CRect localRect( rect_ );
	rect_.OffsetRect( pos - offset_ );

	POINT srcPt = { 0, 0 };
	SIZE srcSize = { rect_.right - rect_.left, rect_.bottom - rect_.top };

	HDC hdcScreen = GetDC( NULL );
	HDC hdc = CreateCompatibleDC( hdcScreen );
	HBITMAP memBmp = CreateCompatibleBitmap( hdcScreen, rect_.Width(), rect_.Height() );
	HBITMAP oldBmp = (HBITMAP)SelectObject( hdc, memBmp );
	CDC::FromHandle( hdc )->FillSolidRect( localRect, RGB( 255, 0, 255 ) );
	imgList_.Draw( CDC::FromHandle( hdc ), 0, CPoint( 0, 0 ), ILD_TRANSPARENT );

	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.AlphaFormat = 0;
	blend.SourceConstantAlpha = alpha;
 
	UpdateLayeredWindow( hwnd_, NULL, NULL, &srcSize, hdc, &srcPt, RGB( 255, 0, 255 ),
			&blend, ULW_COLORKEY | ULW_ALPHA );

	SelectObject( hdc, oldBmp );
	DeleteObject( memBmp );
	DeleteDC( hdc );
	ReleaseDC( NULL, hdcScreen );
}


} //namespace controls
BW_END_NAMESPACE

