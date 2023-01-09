#ifndef CONTROLS_DIBSECTION8_HPP
#define CONTROLS_DIBSECTION8_HPP

#include "controls/defs.hpp"
#include "controls/fwd.hpp"
#include "controls/dib_section.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{	
    /**
     *  This class makes using win32 DibSections easier.  It only works on 
     *  8 bpp dibsections.  It can be used anywhere a HBITMAP is used, but
     *  allows for access to the underlying image.
     */
	class DibSection8 : public DibSection<uint8>
    {
    public:
		void getPalette(COLORREF *values, uint32 start = 0, uint32 count = 256) const;
		void setPalette(COLORREF const *values, uint32 start = 0, uint32 count = 256);
	};

	typedef SmartPointer<DibSection8>	DibSection8Ptr;
}


namespace controls
{
	inline size_t DibSection<uint8>::rowSizeBytes() const
	{
		return ((width() + 3)/4)*4*sizeof(uint8); // round up to nearest DWORD
	}


    inline /*virtual*/ bool DibSection<uint8>::createBuffer
    (
        uint32      w, 
        uint32      h,
        uint8		*&buffer,
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
			bmi_.bmiHeader.biPlanes        = 1;
			bmi_.bmiHeader.biBitCount      = 8;
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

			// Setup a grey-scale palette
			CDC dc;
			dc.CreateCompatibleDC(NULL);
			dc.SelectObject(hbitmap_);
			RGBQUAD palette[256];
			for (size_t i = 0; i < 256; ++i)
			{
				palette[i].rgbRed	= (uint8)i;
				palette[i].rgbGreen	= (uint8)i;
				palette[i].rgbBlue	= (uint8)i;
			}
			::SetDIBColorTable(dc, 0, 256, palette);

			owns    = false;			// don't use delete[] to cleanup.
			stride  = ((w + 3)/4)*4;	// aligned to four byte boundaries
			flipped = true;				// the layout in memory is upside down!

			return true;
		}
		else
		{
			return false;
		}
	}


    inline void DibSection<uint8>::copyFromLockedRect
    (
        D3DLOCKED_RECT      const &lockedRect,
        uint32              w,
        uint32              h
    )
	{
		for (uint32 y = 0; y < h; ++y)
		{
			uint8 *p	= getRow(y);
			uint8 *q	= p + w;
			uint8 const *src = 
				reinterpret_cast<uint8 const *>(lockedRect.pBits) + y*lockedRect.Pitch;
			for ( ; p != q; ++p)
			{
				uint8 r = *src++;
				uint8 g = *src++;
				uint8 b = *src++;
				++src; // skip alpha
				*p = (uint8)((54*r + 182*g + 19*b)/255); // just get luminance
			}
		}
	}
}

BW_END_NAMESPACE

#endif // CONTROLS_DIBSECTION8_HPP
