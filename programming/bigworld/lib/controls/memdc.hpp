#ifndef CONTROLS_MEMDC_HPP
#define CONTROLS_MEMDC_HPP

 
//////////////////////////////////////////////////
// CMemDC - memory DC
//
// Author: Keith Rule
// Email:  keithr@europa.com
// Copyright 1996-2002, Keith Rule
//
// You may freely use or modify this code provided this
// Copyright is included in all derived versions.
//
// History - 10/3/97 Fixed scrolling bug.
//               Added print support. - KR
//
//       11/3/99 Fixed most common complaint. Added
//            background color fill. - KR
//
//       11/3/99 Added support for mapping modes other than
//            MM_TEXT as suggested by Lee Sang Hun. - KR
//
//       02/11/02 Added support for CScrollView as supplied
//             by Gary Kirkham. - KR
//
// This class implements a memory Device Context which allows
// flicker free drawing.

BW_BEGIN_NAMESPACE

namespace controls
{ 
    class MemDC : public CDC 
    {
    public:       
        MemDC();        
        ~MemDC();

		void begin(CDC &dc, const CRect *pRect = NULL);
		void end();

	protected:
		void cleanup();
  
    private:       
        CBitmap     m_bitmap;		// Offscreen bitmap
		CBitmap		*m_oldBitmap;	// Old bitmap selected into DC.
        CDC*        m_pDC;          // Saves CDC passed in constructor
        CRect       m_rect;         // Rectangle of drawing area.
        BOOL        m_bMemDC;       // TRUE if CDC really is a Memory DC.
    };

	class MemDCScope 
	{
	public:
		MemDCScope(MemDC &memDC, CDC &dc, const CRect* pRect = NULL);
		~MemDCScope();

	private:
		MemDCScope(MemDCScope const &);
		MemDCScope &operator=(MemDCScope &);

	private:
		MemDC		*memDC_;
	};
}

BW_END_NAMESPACE

#endif // CONTROLS_MEMDC_HPP
