#ifndef CONTROLS_DIBSECTION32_HPP
#define CONTROLS_DIBSECTION32_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"
#include "controls/dib_section.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
	typedef DibSection<COLORREF>		DibSection32;
	typedef SmartPointer<DibSection32>	DibSection32Ptr;


	inline size_t DibSection32::rowSizeBytes() const
	{
		return width()*sizeof(COLORREF);
	}


	inline void DibSection32::toGreyScale()
	{
		for (uint32 y = 0; y < height(); ++y)
		{
			uint8 *p = (uint8 *)getRow(y);
			uint8 *q = p + rawRowSize();
			for (; p != q; p += 4)
			{
				uint8 r = p[0];
				uint8 g = p[1];
				uint8 b = p[2];
				// Values below are 0.2125*255, 0.7154*255, 0.0721*255 where
				// the fractional parts are the standard weights for converting
				// sRGB to luminance.
				uint8 grey = (uint8)((54*r + 182*g + 19*b)/255);
				p[0] = p[1] = p[2] = grey;
			}
		}
	}

	
	inline /*virtual*/ bool 
	DibSection32::createBuffer
	(
		uint32      w, 
		uint32      h,
		PixelType   *&buffer,
		bool        &owns,
		size_t      &stride,
		bool        &flipped
	)
	{
		destroy();

		if (w != 0 && h != 0)
		{
			// Setup the bitmap info header:
			::ZeroMemory(&bmi_, sizeof(bmi_));
			bmi_.bmiHeader.biSize          = sizeof(bmi_.bmiHeader);
			bmi_.bmiHeader.biWidth         = (LONG)w;
			bmi_.bmiHeader.biHeight        = (LONG)h;
			bmi_.bmiHeader.biPlanes        =  1;
			bmi_.bmiHeader.biBitCount      = 32;
			bmi_.bmiHeader.biCompression   = BI_RGB;
			bmi_.bmiHeader.biSizeImage     = (DWORD)(sizeof(COLORREF)*w*h); 

			// Create the DIBSECTION:
			hbitmap_ =
				::CreateDIBSection
				(
					0,              // Use desktop DC
					&bmi_,
					DIB_RGB_COLORS,
					(void **)&buffer,
					NULL,           // hsection should be null
					0               // offset should be zero
				);

			owns    = false; // don't use delete[] to cleanup.
			stride  = sizeof(COLORREF)*w;
			flipped = true; // the layout in memory is upside down!

			return true;
		}
		else
		{
			return false;
		}
	}


	inline void DibSection32::copyFromLockedRect
	(
		D3DLOCKED_RECT      const &lockedRect,
		uint32              w,
		uint32              h
	)
	{
		for (uint32 y = 0; y < h; ++y)
		{
			COLORREF *p         = getRow(y);
			COLORREF *q         = p + w;
			uint8    const *src = 
				reinterpret_cast<uint8 const *>(lockedRect.pBits) + y*lockedRect.Pitch;
			for ( ; p != q; ++p)
			{
				uint8 r = *src++;
				uint8 g = *src++;
				uint8 b = *src++;
				++src; // skip alpha
				*p = RGB(r, g, b);
			}
		}
	}
}

BW_END_NAMESPACE

#endif // CONTROLS_DIBSECTION32_HPP
