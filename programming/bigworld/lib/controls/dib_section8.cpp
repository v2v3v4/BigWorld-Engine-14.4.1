#include "pch.hpp"
#include "controls/dib_section8.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{


/**
 *	This function gets (part of) the palette of the DibSection8.
 *
 *  @param values		The values in the palette.
 *	@param start		The index of the start value to get.
 *	@param count		The number of palette values to retrieve.
 */
void DibSection8::getPalette
(
	COLORREF			*values, 
	uint32				start /*= 0*/, 
	uint32				count /*= 256*/
) const
{
	BW_GUARD;

	CDC dc;
	dc.CreateCompatibleDC(NULL); // Create a DC based on the screen
	dc.SelectObject((HBITMAP)*this);
	RGBQUAD palette[256];
	::GetDIBColorTable(dc, start, count, palette);
	for (uint32 i = 0; i < count; ++i)
	{
		values[i] = 
			RGB
			(
				palette[i].rgbRed,
				palette[i].rgbGreen,
				palette[i].rgbBlue
			);
	}
}


/**
 *	This function sets (part of) the palette of the DibSection8.
 *
 *  @param values		The new values of the palette.
 *	@param start		The index of the start value to set.
 *	@param count		The number of palette values to set.
 */
void DibSection8::setPalette
(
	COLORREF const		*values, 
	uint32				start /*= 0*/, 
	uint32				count /*= 256*/
)
{
	BW_GUARD;

	CDC dc;
	dc.CreateCompatibleDC(NULL); // Create a DC based on the screen
	dc.SelectObject((HBITMAP)*this);
	RGBQUAD palette[256];
	for (uint32 i = 0; i < count; ++i)
	{
		palette[i].rgbRed	= GetRValue(values[i]);
		palette[i].rgbGreen = GetGValue(values[i]);
		palette[i].rgbBlue	= GetBValue(values[i]);
	}
	::SetDIBColorTable(dc, start, count, palette);
}

} // namespace controls
BW_END_NAMESPACE

