#ifndef SIZER_HPP
#define SIZER_HPP

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

namespace controls
{
    /**
     *  This is the interface for sizers used by the layout manager.
     */
	class Sizer : public ReferenceCount
    {
    public:
        Sizer();

        virtual ~Sizer();

		/**
		 *	This can be called just after initialization to get the
		 *	controls into their start positions.
		 *
		 *	@param wnd		The window the sizer is operating on.
		 */	
		void onStart(CWnd *wnd);

        /**
         *  This should get called on the WM_SIZE window message on the
         *  root sizer.
         *
         *  @param cx       The width of the parent window.
         *  @param cy       The height of the parent window.
         */
        void onSize(int cx, int cy);

        /**
         *  This is called to do the actual sizing.  Derived classes should
         *  call the base class version.
         *
         *  @param extents  The extents allowed for the Sizer and it's 
         *                  children.
         */
        virtual void onSize(CRect const &extents);

        /**
         *  This should be called in OnGetMinMaxInfo in a CWnd to get the
         *  layout manager to handle a minimum size.
         *
         *  @param mmi      The MINMAXINFO to set.
         */
        void onGetMinMaxInfo(MINMAXINFO *mmi);

        /**
         *  This gets the current extents of the sizer.
         *
         *  @returns        The extents of the sizer.
         */
        CRect extents() const;

        /**
         *  This gets the minimum size of the Sizer.
         *
         *  @returns        The minimum allowable size of the Sizer and it's
         *                  sub-elements.
         */
        virtual CSize minimumSize() const = 0;

        /**
         *  Draw the layout into a CDC.
         *
         *  @param dc       The device context to draw into.
         */
        virtual void draw(CDC *dc) = 0;

    protected:
        /**
         *  This sets the extents.  This is done during onSize.
         */
        void extents(CRect const &ext);

        /**
         *  This draws a rectangle.
         */
        void drawRect(CDC *dc, CRect const &rect, COLORREF clr);

    private:
        Sizer(Sizer const &);               // not allowed
        Sizer &operator=(Sizer const &);    // not allowed

    private:
        CRect               extents_;
    };

	typedef SmartPointer<Sizer>	SizerPtr;
}

BW_END_NAMESPACE

#endif // SIZER_HPP
