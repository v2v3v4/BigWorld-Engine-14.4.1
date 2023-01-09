//---------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop
//---------------------------------------------------------------------------
#include "common_utility.h"
#include "moo/moo_math.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/debug.hpp"
#include <stdio.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)

DECLARE_DEBUG_COMPONENT2( "commonUtility", 2 );

//---------------------------------------------------------------------------
/**
 * 	This method loads a DDS image into the given Bitmap controls.
 *
 *	Note : you must have a valid Moo::rc().device(), and the hardware
 *	you are running on must support the dds' compression mode.
 */
void __fastcall CommonUtility::loadDDSImage(
		AnsiString filename,
        Graphics::TBitmap* rgbDest,
        Graphics::TBitmap* alphaDest )
{
	if ( !Moo::rc().device() )
    {
    	MF_ASSERT( "Cannot loadDDSImage without a valid device" );
        return;
    }

	//load dds into a texture
    IDirect3DTexture8* pTexture;
    HRESULT hr = D3DXCreateTextureFromFileEx(
		Moo::rc().device(),
        filename.c_str(),
        rgbDest->Width,
        rgbDest->Height,
        1,
        0,
        D3DFMT_A8R8G8B8,
        D3DPOOL_SYSTEMMEM,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0xFF000000,
        NULL,
        NULL,
        &pTexture );

    if ( FAILED( hr ) )
    {
    	CRITICAL_MSG( "CommonUtility::loadDDSImage - Could not create texture from file %s\n", filename.c_str() );
    }
    else
    {
    	//now, lock the texture and copy the pixels to the destination images.
        D3DLOCKED_RECT lockedRect;
        ZeroMemory( &lockedRect, sizeof( lockedRect ) );
        HRESULT hr = pTexture->LockRect( 0, &lockedRect, NULL, D3DLOCK_READONLY );
        if ( SUCCEEDED( hr ) )
        {
        	//Setup the destination bitmaps
            rgbDest->PixelFormat = pf32bit;
            alphaDest->PixelFormat = pf32bit;

			//Loop through each scanline
            int width = rgbDest->Width;
            int height = rgbDest->Height;

            for ( int y = 0; y < height; y++ )
            {
            	//grab a pointer to the current scanline
                char* pBytes = (char*)lockedRect.pBits;
                uint32 *pixels = (uint32*)( pBytes + y * lockedRect.Pitch );

            	//copy RGB
                uint32 *rgbScanline = (uint32 *)rgbDest->ScanLine[y];
                memcpy( rgbScanline, pixels, width * 4 );

                //copy Alpha
                if ( alphaDest )
                {
                    uint32 *alphaScanline = (uint32 *)alphaDest->ScanLine[y];

                    for( uint32 x = 0; x < width; x++)
                    {
                        Byte a = (Byte)((pixels[x] & 0xff000000) >> 24);
                        uint32 a32 = (a << 16) | (a << 8) | a;
                        *alphaScanline++ = a32;
                    }
                }
            }

            pTexture->UnlockRect( 0 );
        }
        else
        {
        	CRITICAL_MSG( "CommonUtility::loadDDSImage - Could not lock texture resource %s\n", filename.c_str() );
        }

        //release the texture
        pTexture->Release();
    }
}
