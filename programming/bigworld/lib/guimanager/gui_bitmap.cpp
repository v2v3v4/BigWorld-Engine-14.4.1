#include "pch.hpp"
#include "gui_bitmap.hpp"
#include "gui_manager.hpp"
#include "cstdmf/string_utils.hpp"
#include "resmgr/bwresource.hpp"
#include <algorithm>

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

HBITMAP clipBitmap( HBITMAP srcBmp, RECT rect )
{
	BW_GUARD;

	HBITMAP destBmp = NULL;

	BITMAP bitmap;
	HDC dcMem = NULL, dcSrc = NULL;
	HBITMAP bmpOldSrc = NULL, bmpOldMem = NULL;

	if( ( dcMem = CreateCompatibleDC( NULL ) ) && ( dcSrc = CreateCompatibleDC( NULL ) ) )
	{
		GetObject( srcBmp, sizeof( bitmap ), &bitmap );

		if( ( destBmp = CreateBitmap( rect.right - rect.left, rect.bottom - rect.top,
			bitmap.bmPlanes, bitmap.bmBitsPixel, NULL ) ) )
		{
			bmpOldMem = (HBITMAP)SelectObject( dcMem, destBmp );
			bmpOldSrc = (HBITMAP)SelectObject( dcSrc, srcBmp );

			if( bmpOldMem && bmpOldSrc )
			{
				BitBlt( dcMem, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
					dcSrc, rect.left, rect.top, SRCCOPY );
				SelectObject( dcSrc, bmpOldSrc );
				SelectObject( dcMem, bmpOldMem );
			}
		}
	}

	if( dcMem )	DeleteDC( dcMem );
	if( dcSrc )	DeleteDC( dcSrc );
	return destBmp;
}

HBITMAP replaceButtonFace( HBITMAP srcBmp, COLORREF transparent )
{
	BW_GUARD;

	COLORREF buttonFace = GetSysColor( COLOR_BTNFACE );
	HBITMAP destBmp = NULL;

	BITMAP bitmap;
	HDC dcMem = NULL, dcSrc = NULL;
	HBITMAP bmpOldSrc = NULL, bmpOldMem = NULL;

	if( ( dcMem = CreateCompatibleDC( NULL ) ) && ( dcSrc = CreateCompatibleDC( NULL ) ) )
	{
		GetObject( srcBmp, sizeof( bitmap ), &bitmap );

		if( ( destBmp = CreateBitmap( bitmap.bmWidth, bitmap.bmHeight,
			bitmap.bmPlanes, bitmap.bmBitsPixel, NULL ) ) )
		{
			bmpOldMem = (HBITMAP)SelectObject( dcMem, destBmp );
			bmpOldSrc = (HBITMAP)SelectObject( dcSrc, srcBmp );

			if( bmpOldMem && bmpOldSrc )
			{
				BitBlt( dcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight,
					dcSrc, 0, 0, SRCCOPY );
				for( int i = 0; i < bitmap.bmWidth; ++i )
					for( int j = 0; j < bitmap.bmHeight; ++j )
						if( GetPixel( dcMem, i, j ) == transparent )
							SetPixel( dcMem, i, j, buttonFace );
				SelectObject( dcSrc, bmpOldSrc );
				SelectObject( dcMem, bmpOldMem );
			}
		}
	}

	if( dcMem )	DeleteDC( dcMem );
	if( dcSrc )	DeleteDC( dcSrc );
	return destBmp;
}

HBITMAP createGrayBitmap( HBITMAP srcBmp, COLORREF transparent )
{
	BW_GUARD;

	HBITMAP destBmp = NULL;

	BITMAP bitmap;
	HDC dcMem = NULL, dcSrc = NULL;
	HBITMAP bmpOldSrc = NULL, bmpOldMem = NULL;

	if( ( dcMem = CreateCompatibleDC( NULL ) ) && ( dcSrc = CreateCompatibleDC( NULL ) ) )
	{
		GetObject( srcBmp, sizeof( bitmap ), &bitmap );

		if( ( destBmp = CreateBitmap( bitmap.bmWidth, bitmap.bmHeight,
			bitmap.bmPlanes, bitmap.bmBitsPixel, NULL ) ) )
		{
			bmpOldMem = (HBITMAP)SelectObject( dcMem, destBmp );
			bmpOldSrc = (HBITMAP)SelectObject( dcSrc, srcBmp );

			if( bmpOldMem && bmpOldSrc )
			{
				BitBlt( dcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight,
					dcSrc, 0, 0, SRCCOPY );
				for( int i = 0; i < bitmap.bmWidth; ++i )
					for( int j = 0; j < bitmap.bmHeight; ++j )
					{
						COLORREF color = GetPixel( dcMem, i, j );
						if( color != transparent )
						{
							double grayScale = GetRValue( color ) * 0.3 +
								GetGValue( color ) * 0.59 +
								GetBValue( color ) * 0.11;
							grayScale = grayScale / 2 + 128;
							color = RGB( grayScale, grayScale, grayScale );
							SetPixel( dcMem, i, j, color );
						}
					}
				SelectObject( dcSrc, bmpOldSrc );
				SelectObject( dcMem, bmpOldMem );
			}
		}
	}

	if( dcMem )	DeleteDC( dcMem );
	if( dcSrc )	DeleteDC( dcSrc );
	return destBmp;
}

Bitmap::Bitmap( const BW::wstring& name, COLORREF transparent,
			   const BW::string& type/* = "NORMAL" */,	unsigned int width/* = SIZE_DEFAULT*/,
			   unsigned int height/* = SIZE_DEFAULT */)
			   : type_( type ), defaultSize_( width == SIZE_DEFAULT && height == SIZE_DEFAULT )
{
	BW_GUARD;

	RECT rect = { SIZE_DEFAULT, SIZE_DEFAULT, SIZE_DEFAULT, SIZE_DEFAULT };
	BW::wstring bmpName = name.substr( 0, name.find( L':' ) );
	BW::string utf8BmpName;
	bw_wtoutf8WS( bmpName, utf8BmpName );
	BWResource::resolveToAbsolutePath( utf8BmpName );
	bw_utf8towSW( utf8BmpName, bmpName );
	if( name.find( L':' ) != name.npos )
	{
		BW::wstring rectDesc = name.substr( name.find( L':' ) + 1 );
		if( std::count( rectDesc.begin(), rectDesc.end(), L',' ) == 3 )
		{
			BW::string::size_type second = rectDesc.find( L',' );
			BW::string::size_type third = rectDesc.find( L',', second + 1 );
			BW::string::size_type fourth = rectDesc.find( L',', third + 1 );
			rect.left = _wtoi( rectDesc.c_str() );
			rect.top = _wtoi( rectDesc.c_str() + second + 1 );
			rect.right = _wtoi( rectDesc.c_str() + third + 1 );
			rect.bottom = _wtoi( rectDesc.c_str() + fourth + 1 );
		}
	}
	bitmap_ = (HBITMAP)LoadImage( GetModuleHandle( NULL ), bmpName.c_str(), IMAGE_BITMAP,
		width == SIZE_DEFAULT	?	0	:	width,
		height == SIZE_DEFAULT	?	0	:	height,
		0 );
	if( !bitmap_ )
	{
		if( !bmpName.empty() && bmpName[0] == L'#' )
		{
			bitmap_ = (HBITMAP)LoadImage( GetModuleHandle( NULL ), 
				MAKEINTRESOURCE( _wtoi( bmpName.c_str() + 1 ) ), IMAGE_BITMAP,
				width == SIZE_DEFAULT	?	0	:	width,
				height == SIZE_DEFAULT	?	0	:	height,
				0 );			
		}
	}
	if( !bitmap_ )
	{
		BW::string fileName;
		bw_wtoutf8( bmpName, fileName );
		if( GUI::Manager::resolveFileName( fileName ) )
		{
			bitmap_ = (HBITMAP)LoadImage( GetModuleHandle( NULL ), bmpName.c_str(), IMAGE_BITMAP,
				width == SIZE_DEFAULT	?	0	:	width,
				height == SIZE_DEFAULT	?	0	:	height,
				LR_LOADFROMFILE );
		}
	}
	if( !bitmap_ )
	{
		;	//	throw 1;
	}
	if( rect.left != SIZE_DEFAULT )
	{
		HBITMAP temp = clipBitmap( bitmap_, rect );
		DeleteObject( bitmap_ );
		bitmap_ = temp;
	}
	if( type_ == "NORMAL" )
	{
	}
	else if( type_ == "HIGHLIGHT" )
	{
	}
	else if( type_ == "DISABLED" )
	{
		HBITMAP temp = createGrayBitmap( bitmap_, transparent );
		DeleteObject( bitmap_ );
		bitmap_ = temp;
	}
	HBITMAP temp = replaceButtonFace( bitmap_, transparent );
	DeleteObject( bitmap_ );
	bitmap_ = temp;
}

Bitmap::~Bitmap()
{
	BW_GUARD;

	DeleteObject( bitmap_ );
}

const BW::string& Bitmap::type() const
{
	return type_;
}

unsigned int Bitmap::width() const
{
	BW_GUARD;

	BITMAP bitmap;
	GetObject( bitmap_, sizeof( bitmap ), &bitmap );
	return bitmap.bmWidth;
}

unsigned int Bitmap::height() const
{
	BW_GUARD;

	BITMAP bitmap;
	GetObject( bitmap_, sizeof( bitmap ), &bitmap );
	return bitmap.bmHeight;
}

bool Bitmap::defaultSize() const
{
	return defaultSize_;
}

Bitmap::operator HBITMAP()
{
	return bitmap_;
}

BitmapPtr BitmapManager::find( const BW::string& name, const BW::string& type/* = "NORMAL" */,
	unsigned int width/* = SIZE_DEFAULT*/, unsigned int height/* = SIZE_DEFAULT */)
{
	BW_GUARD;

	BitmapPtr result( NULL );
	if( bitmaps_.find( name ) != bitmaps_.end() )
	{
		BW::vector<BitmapPtr>& bitmaps = bitmaps_[ name ];
		bool isDefault = ( width == SIZE_DEFAULT && height == SIZE_DEFAULT );
		for( BW::vector<BitmapPtr>::iterator iter = bitmaps.begin(); iter != bitmaps.end();
			++iter )
		{
			if( (*iter)->type() == type )
			{
				if( (*iter)->defaultSize() && isDefault ||
					(*iter)->width() == width && (*iter)->height() == height )
				{
					result = (*iter);
					break;
				}
			}
		}
	}
	return result;
}

BitmapPtr BitmapManager::get( const BW::string& name, COLORREF transparent,
	const BW::string& type/* = "NORMAL" */, unsigned int width/* = SIZE_DEFAULT*/, unsigned int height/* = SIZE_DEFAULT */)
{
	BW_GUARD;

	BitmapPtr result( NULL );
	if( bitmaps_.find( name ) != bitmaps_.end() )
	{
		BW::vector<BitmapPtr>& bitmaps = bitmaps_[ name ];
		bool isDefault = ( width == SIZE_DEFAULT && height == SIZE_DEFAULT );
		for( BW::vector<BitmapPtr>::iterator iter = bitmaps.begin(); iter != bitmaps.end();
			++iter )
		{
			if( (*iter)->type() == type )
			{
				if( (*iter)->defaultSize() && isDefault ||
					(*iter)->width() == width && (*iter)->height() == height )
				{
					result = (*iter);
					break;
				}
			}
		}
	}
	if( !result )
	{
		try
		{
			BW::wstring wname;
			bw_utf8tow( name, wname );
			result = new Bitmap( wname, transparent, type, width, height );
		}
		catch(...)//TODO: refine exceptions
		{}
	}
	if( result )
		bitmaps_[ name ].push_back( result );

	return result;
}

void BitmapManager::free( BitmapPtr& bitmap )
{
	BW_GUARD;

	for( BW::map<BW::string, BW::vector<BitmapPtr> >::iterator miter = bitmaps_.begin();
		miter != bitmaps_.end(); ++miter )
	{
		BW::vector<BitmapPtr>& bitmaps = miter->second;
		for( BW::vector<BitmapPtr>::iterator iter = bitmaps.begin(); iter != bitmaps.end();
			++iter )
		{
			if( (*iter) == bitmap )
			{
				bitmap = NULL;
				if( (*iter)->refCount() == 1 )
				{
					bitmaps.erase( iter );
					return;
				}
			}
		}
	}
}

void BitmapManager::clear()
{
	BW_GUARD;

	bitmaps_.clear();
}


END_GUI_NAMESPACE
BW_END_NAMESPACE

