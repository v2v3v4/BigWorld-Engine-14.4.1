#ifndef CONTROLS_IMAGE_CONTROL_IPP
#define CONTROLS_IMAGE_CONTROL_IPP

BW_BEGIN_NAMESPACE

/**
 *  This is the ImageControl's ctor.
 */
template<typename IMAGETYPE>
inline controls::ImageControl<IMAGETYPE>::ImageControl():
    border_(BS_SUNKEN),
    aspect_(PRESERVE_FUZZY),
    background_(::GetSysColor(COLOR_3DFACE)),
    padding_(0),
    setCount_(0)
{
}


/**
 *  This creates an ImageControl
 *
 *  @param style    The window style to apply.  Typically this is 
 *                  WS_CHILD | WS_VISIBLE.
 *  @param extents  The extents of the window.
 *  @param parent   The parent window.
 *  @param id       The id of this window.
 *  @returns        TRUE if sucessfully created, FALSE if something went wrong.
 */
template<typename IMAGETYPE>
inline BOOL 
controls::ImageControl<IMAGETYPE>::Create
(
    DWORD           style, 
    RECT            const &extents, 
    CWnd            *parent,
    unsigned int    id  /* = 0*/
)
{
	BW_GUARD;

    static bool     registered = false;
    static CString  classname;
    if (!registered)
    {
        classname = AfxRegisterWndClass(NULL, NULL, NULL, NULL);
        registered = true;
    }

    BOOL result = 
        CWnd::Create
        (
            classname,
            L"ImageControl",
            style,
            extents,
            parent,
            id
        );

    return result;
}


/**
 *  This subclasses the control over an existing control.
 *
 *  @param resourceID       The id of the control to take over.
 *  @param parent           The parent window.
 *  @returns                TRUE upon success.
 */
template<typename IMAGETYPE>
inline BOOL 
controls::ImageControl<IMAGETYPE>::subclass(unsigned int resourceID, CWnd *parent)
{
	BW_GUARD;

    if (parent == NULL)
        return FALSE;

    CWnd *wnd = parent->GetDlgItem(resourceID);
    if (wnd == NULL)
        return FALSE;

    CRect rect;
    wnd->GetWindowRect(&rect);
    parent->ScreenToClient(&rect);
    wnd->ShowWindow(SW_HIDE);

    BOOL result = Create(WS_CHILD | WS_VISIBLE, rect, parent, resourceID);
    if (result == FALSE)
        return FALSE;

    return TRUE;
}


/**
 *  This sets the image to draw with.
 *
 *  @param dibSection       The new image to display.
 */
template<typename IMAGETYPE>
inline void 
controls::ImageControl<IMAGETYPE>::image(ImageType const &dibSection)
{
	BW_GUARD;

    image_ = dibSection;
    if (setCount_ == 0)
    {
        Invalidate();
        RedrawWindow();
    }
}


/**
 *  This gets the image that is drawn.
 *
 *  @returns                The image drawn.
 */
template<typename IMAGETYPE>
inline typename controls::ImageControl<IMAGETYPE>::ImageType const &
controls::ImageControl<IMAGETYPE>::image() const
{
    return image_;
}


/**
 *  This function returns the displayed image.
 *
 *  @returns                The displayed image.
 */
template<typename IMAGETYPE>
inline typename controls::ImageControl<IMAGETYPE>::ImageType &
controls::ImageControl<IMAGETYPE>::image()
{
    return image_;
}


/**
 *  This function gets the border style.
 *
 *  @returns                The border style used to draw the control.
 */
template<typename IMAGETYPE>
inline typename controls::ImageControl<IMAGETYPE>::BorderStyle 
controls::ImageControl<IMAGETYPE>::border() const
{
    return border_;
}


/**
 *  This function sets the border style.
 *
 *  @param display          The border style to draw the control with.
 */
template<typename IMAGETYPE>
inline void controls::ImageControl<IMAGETYPE>::border(BorderStyle display)
{
	BW_GUARD;

    border_ = display;
    if (setCount_ == 0)
    {
        Invalidate();
        RedrawWindow();
    }
}


/**
 *  This function returns how the control preserves the aspect ratio of the
 *	image.
 *
 *  @returns                How the control preserves the aspect ratio of the
 *							image.  See preserveAspectRatio below on what the
 *							values do.
 */
template<typename IMAGETYPE>
inline typename controls::ImageControl<IMAGETYPE>::AspectRatio 
controls::ImageControl<IMAGETYPE>::preserveAspectRatio() const
{
    return aspect_;
}


/**
 *  This function sets whether the control preserves the image's aspect ratio
 *  when drawing.
 *
 *  @param preserve         If FIT_TO_WINDOW then the image is fitted into the
 *							window and aspect ratios are ignored.
 *							If PRESERVE then the aspect ratio of the image is
 *							preserved and the control displays white around
 *							the edges that don't fit.
 *							If PRESERVE_FUZZY then the aspect ratio is 
 *							preserved unless the difference is only a few 
 *							pixels, in which case the image is fitted to the
 *							window.
 */ 
template<typename IMAGETYPE>
inline void controls::ImageControl<IMAGETYPE>::preserveAspectRatio
(
	AspectRatio preserve
)
{
	BW_GUARD;

    aspect_ = preserve;
    if (setCount_ == 0)
    {
        Invalidate();
        RedrawWindow();
    }
}


/**
 *  This function gets the colour to draw the part of the control that is not
 *  the image or the border.  By default it is COLOR_3DFACE.
 *
 *  @returns                The background colour to draw with.
 */
template<typename IMAGETYPE>
inline COLORREF controls::ImageControl<IMAGETYPE>::backgroundColour() const
{
    return background_;
}


/**
 *  This function sets the colour to draw the part of the control that is not
 *  the image or the border.
 *
 *  @param colour           The new background colour.
 */
template<typename IMAGETYPE>
inline void controls::ImageControl<IMAGETYPE>::backgroundColour(COLORREF colour)
{
	BW_GUARD;

    background_ = colour;
    if (setCount_ == 0)
    {
        Invalidate();
        RedrawWindow();
    }
}


/**
 *  This function gets the padding between the border and the image.
 *
 *  @returns                The distance between the border and the image.
 */
template<typename IMAGETYPE>
inline unsigned int controls::ImageControl<IMAGETYPE>::borderPadding() const
{
    return padding_;
}

/**
 *  This function sets the padding between the border and the image.
 *
 *  @param padding          The new padding distance.
 */
template<typename IMAGETYPE>
inline void controls::ImageControl<IMAGETYPE>::borderPadding(unsigned int padding)
{
	BW_GUARD;

    padding_ = padding;
    if (setCount_ == 0)
    {
        Invalidate();
        RedrawWindow();
    }
}


/**
 *  This function allows batch setting of properties without getting a redraw
 *  between each (i.e. helps prevent flicker).  It is recursive and only 
 *  redraws when a reference count drops to zero.
 */
template<typename IMAGETYPE>
inline void controls::ImageControl<IMAGETYPE>::beginSet()
{
    ++setCount_;
}


/**
 *  This function allows batch setting of properties without getting a redraw
 *  between each (i.e. helps prevent flicker).  It is recursive and only 
 *  redraws when a reference count drops to zero.
 */
template<typename IMAGETYPE>
inline void controls::ImageControl<IMAGETYPE>::endSet()
{
	BW_GUARD;

    if (--setCount_ == 0)
    {
        Invalidate();
        RedrawWindow();
    }
}


/**
 *	This function sets the text that is displayed if there is no image.
 *
 *  @param str		The text to display if there is no image.
 */
template<typename IMAGETYPE>
inline void controls::ImageControl<IMAGETYPE>::text(BW::string const &str)
{
	BW_GUARD;

	bw_utf8tow( str, textNoImage_ );
    if (setCount_ == 0)
    {
        Invalidate();
        RedrawWindow();
    }
}


/**
 *	This function gets the text that is displayed if there is no image.
 *
 *  @returns		The text to display if there is no image.
 */
template<typename IMAGETYPE>
inline BW::string controls::ImageControl<IMAGETYPE>::text() const
{
	BW_GUARD;

	return bw_wtoutf8( textNoImage_ );
}


/**
 *  This function is called to paint the control.
 */
template<typename IMAGETYPE>
inline /*afx_msg*/ void controls::ImageControl<IMAGETYPE>::OnPaint()
{
	BW_GUARD;

    CPaintDC dc(this);

    CRect rect;
    GetClientRect(&rect);

    // Draw the border:
    switch (border_)
    {
    case BS_NONE:
        break;

    case BS_BLACKRECT:
        dc.Draw3dRect(rect, RGB(0, 0, 0), RGB(0, 0, 0)); 
        rect.DeflateRect(1, 1);
        break;

    case BS_SUNKEN:
        dc.Draw3dRect
        (
            rect, 
            ::GetSysColor(COLOR_3DSHADOW), 
            ::GetSysColor(COLOR_3DHILIGHT)
        ); 
        rect.DeflateRect(1, 1);
        break;

    case BS_RAISED:
        dc.Draw3dRect
        (
            rect, 
            ::GetSysColor(COLOR_3DHILIGHT), 
            ::GetSysColor(COLOR_3DSHADOW)
        ); 
        rect.DeflateRect(1, 1);
        break;
    }

    // Draw the background:
    dc.FillSolidRect(rect, background_);
    rect.DeflateRect(padding_, padding_);

    // Degenerate case:
    if (image_.isEmpty())
    {
        if (!textNoImage_.empty())
		{
			CWnd *parent = GetParent();	
			CFont *oldFont = NULL;
			if (parent != NULL)
			{
				oldFont = dc.SelectObject(parent->GetFont());
			}
			COLORREF oldColour = dc.SetTextColor(::GetSysColor(COLOR_BTNTEXT));
			int oldBkMode = dc.SetBkMode(TRANSPARENT);
			CRect textRect = rect;
			dc.DrawText
			(
				textNoImage_.c_str(), 
				(int)textNoImage_.length(), 
				textRect, 
				DT_CALCRECT | DT_WORDBREAK
			);
			int th = textRect.Height();
			textRect.left   = rect.left;
			textRect.top    = rect.top + (rect.Height() - th)/2;
			textRect.right  = rect.right;
			textRect.bottom = textRect.top + th;
			dc.DrawText
			(
				textNoImage_.c_str(), 
				(int)textNoImage_.length(), 
				textRect, 
				DT_CENTER | DT_WORDBREAK
			);
			dc.SetBkMode(oldBkMode);
			dc.SetTextColor(oldColour);
			if (oldFont != NULL)
				dc.SelectObject(oldFont);
		}
    }
    // Preserve the image's aspect ratio cases:
    else 
	{
		bool drawn = false;
		if (aspect_ == PRESERVE || aspect_ == PRESERVE_FUZZY)
		{
			float dispAspt = (float)rect.Width()/(float)rect.Height();
			float imgAspt  = (float)image_.width()/(float)image_.height();
			int dx = 0;
			int dy = 0;
			if (dispAspt > imgAspt)
			{
				dx = (int)(0.5f*(rect.Width() - rect.Height()*imgAspt));
			}
			else
			{
				dy = (int)(0.5f*(rect.Height() - rect.Width()/imgAspt));
			}
			if 
			(
				aspect_ == PRESERVE 
				|| 
				dx > FUZZY_ASPECT_PIXELS 
				|| 
				dy > FUZZY_ASPECT_PIXELS
			)
			{
				CDC memDC;
				memDC.CreateCompatibleDC(NULL);
				HGDIOBJ oldBmp = ::SelectObject(memDC, (HBITMAP)image_);
				int oldMode = dc.SetStretchBltMode(HALFTONE);
				dc.StretchBlt
				(
					rect.left + dx,
					rect.top  + dy,
					rect.Width () - 2*dx,
					rect.Height() - 2*dy,
					&memDC,
					0,
					0,
					image_.width (),
					image_.height(),
					SRCCOPY
				);
				dc.SetStretchBltMode(oldMode);
				::SelectObject(memDC, oldBmp);
				drawn = true;
			}
		}
		// Fit into the available area case:
		if (!drawn)
		{
			CDC memDC;
			memDC.CreateCompatibleDC(NULL);
			HGDIOBJ oldBmp = ::SelectObject(memDC, (HBITMAP)image_);
			int oldMode = dc.SetStretchBltMode(HALFTONE);
			dc.StretchBlt
			(
				rect.left,
				rect.top,
				rect.Width (),
				rect.Height(),
				&memDC,
				0,
				0,
				image_.width (),
				image_.height(),
				SRCCOPY
			);
			dc.SetStretchBltMode(oldMode);
			::SelectObject(memDC, oldBmp);
		}
	}
}


/**
 *  This function is called to paint the control's background.  It does nothing
 *  but returns TRUE.  The actual painting is completely done in OnPaint.
 */
template<typename IMAGETYPE>
inline /*afx_msg*/ BOOL controls::ImageControl<IMAGETYPE>::OnEraseBkgnd(CDC * /*dc*/)
{
    return TRUE; // all drawing is in OnPaint
}

BW_END_NAMESPACE


#endif // CONTROLS_IMAGE_CONTROL_IPP
