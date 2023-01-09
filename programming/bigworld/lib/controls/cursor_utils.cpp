#include "pch.hpp"
#include "cursor_utils.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

/**
 *	This method returns a new cursor that looks like the original cursor but
 *	with a small "+" sign in the bottom-right corner, useful when performing
 *	operations such as add instead of move, etc.
 *	IMPORTANT: The caller is responsible for deleting the returned HCURSOR.
 *
 *	@param origCursor	Cursor to put the plus sign overlay over.
 *	@return				The new cursor, or NULL on failure. The caller is
 *						responsible for deleting the returned HCURSOR.
 */
HCURSOR addPlusSignToCursor( HCURSOR origCursor )
{
	BW_GUARD;

	unsigned char bits[] = {
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 3, 254,	// 00000000 00000000 00000011 11111110
		0, 0, 3, 254,	// 00000000 00000000 00000011 11111110
		0, 0, 3, 222,	// 00000000 00000000 00000011 11011110
		0, 0, 3, 222,	// 00000000 00000000 00000011 11011110
		0, 0, 3, 6,		// 00000000 00000000 00000011 00000110
		0, 0, 3, 222,	// 00000000 00000000 00000011 11011110
		0, 0, 3, 222,	// 00000000 00000000 00000011 11011110
		0, 0, 3, 254,	// 00000000 00000000 00000011 11111110
		0, 0, 3, 254,	// 00000000 00000000 00000011 11111110
		0, 0, 0, 0 };	// 00000000 00000000 00000000 00000000

	unsigned char mask[] = {
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 0, 0,		// 00000000 00000000 00000000 00000000
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255,	// 00000000 00000000 00000111 11111111
		0, 0, 7, 255 };	// 00000000 00000000 00000111 11111111

	return addOverlayToCursor( origCursor, bits, mask );
}


/**
 *	This method returns a new cursor that looks like the original cursor but
 *	with the bitmap specified overlaid. The overlay is monochrome but it can
 *	be applied on top of monochrome or colour cursors.
 *	IMPORTANT: The caller is responsible for deleting the returned HCURSOR.
 *
 *	@param origCursor	Cursor to put the overlay over.
 *	@param bitmap		32x32 Monochrome bitmap (1 BPP) where 1 means white and
 *						0 means black. Must be a total of 128 bytes in size.
 *	@param mask			32x32 Monochrome mask (1 BPP) where 1 means overlay and
 *						0 means keep original pixel. Must be 128 bytes in size.
 *	@return				The new cursor, or NULL on failure. The caller is
 *						responsible for deleting the returned HCURSOR.
 */
HCURSOR addOverlayToCursor( HCURSOR origCursor, unsigned char * bitmap, unsigned char * mask )
{
	BW_GUARD;

	if (!bitmap || !mask)
	{
		return NULL;
	}

	HCURSOR overlayCursor = NULL;

	ICONINFO curInfo;
	if (GetIconInfo( HICON( origCursor ), &curInfo ) && curInfo.hbmMask)
	{
		if (!curInfo.hbmColor)
		{
			// The original cursor is monochrome
			// Should be 32 x 64 bitmap with 1 BPP = 256 bytes.
			unsigned char pMasks[ 256 ];
			memset( pMasks, 0, 256 );

			BITMAP bmpInfo;
			GetObject( curInfo.hbmMask, sizeof( BITMAP ), &bmpInfo );
			if (bmpInfo.bmWidth == 32 && bmpInfo.bmHeight == 64 &&
				GetBitmapBits( curInfo.hbmMask, 256, pMasks ) == 256)
			{
				for (int i = 0; i < 32*4; ++i)
				{
					// Overlay the mask
					pMasks[ i ] &= ~mask[ i ];
					// Overlay the bitmap
					pMasks[ i + 32*4 ] &= ~mask[ i ]; // clear the masked bits
					pMasks[ i + 32*4 ] |= (bitmap[ i ] & ~pMasks[ i ]); // put the bitmap bits
				}

				overlayCursor = CreateCursor( AfxGetInstanceHandle(),
					curInfo.xHotspot, curInfo.yHotspot, 32, 32, pMasks, pMasks + 128 );
			}
		}
		else
		{
			// The original cursor has colour, add the overlay using SetPixel
			BITMAP rgbInfo;
			GetObject( curInfo.hbmColor, sizeof( BITMAP ), &rgbInfo );
			BITMAP maskInfo;
			GetObject( curInfo.hbmMask, sizeof( BITMAP ), &maskInfo );
			if (rgbInfo.bmWidth == 32 && rgbInfo.bmHeight == 32 &&
				maskInfo.bmWidth == 32 && maskInfo.bmHeight == 32 &&
				maskInfo.bmBitsPixel == 1)
			{
				// The mask is 32*32*1BPP = 128 bytes.
				unsigned char pMask[ 128 ];
				memset( pMask, 0, 128 );
				if (GetBitmapBits( curInfo.hbmMask, 128, pMask ) == 128)
				{
					// Overlay the mask
					for (int i = 0; i < 32*4; ++i)
					{
						pMask[ i ] &= ~mask[ i ];
					}
					SetBitmapBits( curInfo.hbmMask, 128, pMask );

					// Mask copied successfuly to "pMasks".
					// Now place our monochrome overlay on top of the colour cursor
					CDC * desktopCDC = CDC::FromHandle( GetDC( GetDesktopWindow() ) );
					CDC bmpDC;
					bmpDC.CreateCompatibleDC( desktopCDC );
					CBitmap * oldBmp = bmpDC.SelectObject( CBitmap::FromHandle( curInfo.hbmColor ) );
					for (int y = 0; y < 32; ++y)
					{
						int byteIdx = y * 4;
						for (int x = 0; x < 32; x += 8, ++byteIdx)
						{
							unsigned char byte = 0;
							for (int bit = 0; bit < 8; ++bit)
							{
								unsigned char thisBitMask = (1 << (7 - bit));
								if (mask[ byteIdx ] & thisBitMask)
								{
									if (bitmap[ byteIdx ] & thisBitMask)
									{
										bmpDC.SetPixelV( x + bit, y, RGB( 255, 255, 255 ) );
									}
									else
									{
										bmpDC.SetPixelV( x + bit, y, RGB( 0, 0, 0 ) );
									}
								}
								else
								{
									// We have to overwrite unmodified pixels to clear the
									// alpha channel (GetPixel/SetPixel don't support alpha values)
									bmpDC.SetPixelV( x + bit, y, bmpDC.GetPixel( x + bit, y ) );
								}
							}
						}
					}
					bmpDC.SelectObject( oldBmp );

					overlayCursor = CreateIconIndirect( &curInfo );
				}
			}
			DeleteObject( curInfo.hbmColor );
		}
		DeleteObject( curInfo.hbmMask );
	}

	return overlayCursor;
}

} // namespace controls
BW_END_NAMESPACE

