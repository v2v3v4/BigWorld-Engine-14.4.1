#include "pch.hpp"
#include "controls/color_picker.hpp"
#include "controls/user_messages.hpp"

BW_BEGIN_NAMESPACE
namespace controls
{

namespace
{
    // Conversion between the HSL (Hue, Saturation, and Luminosity in float)
    // and RBG (Red, Green, Blue in ranges 0-255) color model.
    // see:
    // Fundamentals of Interactive Computer Graphics by Foley and van Dam. 
    float HuetoRGB(float m1, float m2, float h )
    {
		BW_GUARD;

		if (h < 0)
			h += 1.0f;
		if (h > 1)
			h -= 1.0f;
		if (6.0f*h < 1)
			return (m1+(m2-m1)*h*6.0f);
		if (2.0f*h < 1)        return m2;
		if (3.0f*h < 2.0f)
			return (m1+(m2-m1)*((2.0f/3.0f)-h)*6.0f);
		return m1;
    }

    void HLStoRGB(float H, float L, float S, float & r, float & g, float & b)
    {
		BW_GUARD;

        float m1, m2;

        if (S == 0)
        {
            r = g = b = L;
        } 
        else
        {
            if(L <= 0.5f)
                m2 = L * (1.0f + S);
            else
                m2 = L + S - L * S;

            m1 = 2.0f * L - m2;
            r = HuetoRGB(m1, m2, H+1.0f/3.0f);
            g = HuetoRGB(m1, m2, H);
            b = HuetoRGB(m1, m2, H-1.0f/3.0f);
        }
    }


    COLORREF HLStoRGB(float H, float L, float S)
    {
		BW_GUARD;

        float r, g, b;
        HLStoRGB(H, L, S, r, g, b);
        return RGB((BYTE)(r*255), (BYTE)(g*255), (BYTE)(b*255));
    }


    void RGBtoHLS(COLORREF rgb, float & H, float & L, float & S)
    {   
		BW_GUARD;

        float delta;
        float r = (float)GetRValue(rgb) / 255;
        float g = (float)GetGValue(rgb) / 255;
        float b = (float)GetBValue(rgb) / 255;   
        float cmax = max(r, max(g, b));
        float cmin = min(r, min(g, b));   
        L = (cmax + cmin) / 2.0f;   
        
        if(cmax == cmin) 
        {
        S = 0;      
        H = 0; // is undefined   
        } 
        else 
        {
        if (L < 0.5f) 
            S = (cmax - cmin) / (cmax + cmin);      
        else
            S = (cmax - cmin) / (2.0f - cmax - cmin);      
          
        delta = cmax - cmin;

        if (r == cmax) 
            H = (g - b) / delta;      
        else if (g == cmax)
            H = 2.0f + (b - r) / delta;
        else          
            H = 4.0f + (r - g) / delta;
        H /= 6.0f; 
        if(H < 0.0f)
            H += 1;  
        }
    }


    const int c_hueBarWidth_        = 20;
    const int c_alphaBarHeight_     = 20;
    const int c_spacer_             =  5;
    const int c_borderSpace_        =  5;
    const int c_heightOfFinalColor_ =  6;
}


BEGIN_MESSAGE_MAP(ColorPicker, CWnd)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_PAINT()
    ON_WM_SETCURSOR()
    ON_WM_SIZE()
END_MESSAGE_MAP()


/**
 *  Constructor.
 */
ColorPicker::ColorPicker() :
    CWnd(),
    createPickerDC_(true),
    createHueBarDC_(true),
    createAlphaBarDC_(true),
    hueBarDirty_(true),
    slGraphDirty_(true),
    slPickerDirty_(true),
    alphaBarDirty_(true),
    mouseMoveMode_(MMM_NONE),
    alphaSelection_(true)
{
	BW_GUARD;

    currentColor_       = RGB(0, 0, 255);   // blue
    currentAlpha_       = 1.f;
    RGBtoHLS(currentColor_, currentHue_, currentLuminance_, currentSaturation_);    
}


/**
 *  Destructor.
 */
ColorPicker::~ColorPicker()
{
}


/**
 *  Create the window.
 * 
 *  @param style        The style of the window.  Usually this will be
 *                      WS_CHILD | WS_VISIBLE.
 *  @param pos          The position of the window.
 *  @param parent       The parent window.
 *  @param alphaSel     Allow selection of alpha values.
 */
BOOL 
ColorPicker::Create
(
    DWORD       dwStyle, 
    CRect       rcPos, 
    CWnd        *pParent, 
    bool        alphaSelection
)
{
	BW_GUARD;

    // Get the class name and create the window
    LPVOID lp = (LPVOID)NULL;
    if
    (
        !CreateEx
        (
            0,
            AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW)), 
            _T("ColorTimelineWndWnd"),
            dwStyle,
            rcPos.left,
            rcPos.top,
            rcPos.Width(),
            rcPos.Height(),
            pParent->GetSafeHwnd(),
            (HMENU)(4230),
            lp
        )
    )        
    {
        TRACE0("Failed to create ColorTimelineWnd\n");
        return FALSE;
    }

    alphaSelection_ = alphaSelection;

    ShowWindow(SW_SHOW);
    UpdateWindow();
    RedrawWindow();
    
    return TRUE;
}


/**
 *  Set the chosen color.
 * 
 *  @param ref          The new chosen color.
 */
void ColorPicker::setRGB(COLORREF ref)
{
	BW_GUARD;

    RGBtoHLS(ref, currentHue_, currentLuminance_, currentSaturation_);
    currentColor_ = ref;

    slPickerDirty_ = true;
    slGraphDirty_ = true;

    Invalidate();
}


/**
 *  Get the chosen color.
 * 
 *  @returns            The chosen color.
 */
COLORREF ColorPicker::getRGB()
{
    return currentColor_;
}


/**
 *  Set the chosen color.
 * 
 *  @param hue          The hue of the chosen color.
 *  @param lum          The luminance of the chosen color.
 *  @param sat          The saturation of the chosen color.
 */
void ColorPicker::setHLS(float hue,float luminance, float saturation)
{
	BW_GUARD;

    currentHue_ = hue;
    currentSaturation_ = saturation;
    currentLuminance_ = luminance;

    currentColor_ = HLStoRGB(currentHue_, currentLuminance_, currentSaturation_);
    slPickerDirty_ = true;
    slGraphDirty_ = true;

    Invalidate();
}


/**
 *  Get the chosen color.
 * 
 *  @param hue          The hue of the chosen color.
 *  @param lum          The luminance of the chosen color.
 *  @param sat          The saturation of the chosen color.
 */
void ColorPicker::getHLS(float & hue, float & luminance, float & saturation)
{
    hue = currentHue_;
    luminance = currentLuminance_;
    saturation = currentSaturation_;
}


/**
 *  Set the chosen color.
 * 
 *  @param color        The new chosen color.  Each component is in the
 *                      range [0, 1].
 */
void ColorPicker::setRGBA(const Vector4 & color)
{
	BW_GUARD;

    currentColor_ = RGB(color.x * 255, color.y * 255, color.z * 255);
    if (alphaSelection_)
        currentAlpha_ = color.w;

    RGBtoHLS(currentColor_, currentHue_, currentLuminance_, currentSaturation_);
    
    slPickerDirty_ = true;
    slGraphDirty_ = true;

    if ( GetSafeHwnd() )
        Invalidate();
}


/**
 *  Get the chosen color.
 * 
 *  @returns            The chosen color.  ach component is in the
 *                      range [0, 1].
 */
Vector4 ColorPicker::getRGBA() const
{
	BW_GUARD;

    return 
        Vector4
        (
            GetRValue(currentColor_)/255.f, 
            GetGValue(currentColor_)/255.f, 
            GetBValue(currentColor_)/255.f, 
            currentAlpha_
        );
}


void ColorPicker::OnLButtonDown(UINT nFlags, CPoint point) 
{
	BW_GUARD;

    SetCapture();

    if (!slGraphDirty_)
    {
        

        // Expand the hue and alpha rectangles in the direction of slide,
        // since the user can select the movement handle and still be outside
        // the rectangle.
        CRect hueRect(hueBarRect_);
        hueRect.InflateRect(0, 3);
        CRect alphaRect(alphaBarRect_);
        alphaRect.InflateRect(3, 0);

        if (hueRect.PtInRect(point))
        {
            mouseMoveMode_ = MMM_HUE;
        }
        else if (alphaRect.PtInRect(point))
        {
            mouseMoveMode_ = MMM_ALPHA;
        }
        else if (slGraphRect_.PtInRect(point))
        {
            mouseMoveMode_ = MMM_SL_PICKER;
        }

        UpdateParent(WM_CP_LBUTTONDOWN);

        OnMouseMove(nFlags, point);

        Invalidate();
    }

    CWnd::OnLButtonDown(nFlags, point);
}


void ColorPicker::OnMouseMove(UINT nFlags, CPoint point) 
{
	BW_GUARD;

    if (!slGraphDirty_ && (nFlags & MK_LBUTTON))
    {
        if (mouseMoveMode_ == MMM_HUE)
        {
            point.y = 
                Math::clamp(hueBarRect_.top, point.y, hueBarRect_.bottom);

            currentHue_   = (float)(point.y - hueBarRect_.top)/(float)hueBarRect_.Height();
            currentColor_ = HLStoRGB(currentHue_, currentLuminance_, currentSaturation_);
            slGraphDirty_ = true;
        }
        else if (alphaSelection_ && (mouseMoveMode_ == MMM_ALPHA))
        {
            point.x = 
                Math::clamp(alphaBarRect_.left, point.x, alphaBarRect_.right);
            currentAlpha_ = (float)(point.x - alphaBarRect_.left)/(float)alphaBarRect_.Width();
            slGraphDirty_ = true;
        }
        else if (mouseMoveMode_ == MMM_SL_PICKER)
        {
            point.x = 
                Math::clamp(slGraphRect_.left, point.x, slGraphRect_.right );
            point.y = 
                Math::clamp(slGraphRect_.top , point.y, slGraphRect_.bottom);
            
            if (nFlags & MK_CONTROL)
                point.x = slPickerPosition_.x;
            if (nFlags & MK_SHIFT)
                point.y = slPickerPosition_.y;

            currentLuminance_  = (float)(point.x - slGraphRect_.left)/(float)slGraphRect_.Width ();
            currentSaturation_ = (float)(point.y - slGraphRect_.top )/(float)slGraphRect_.Height();
            currentColor_ = HLStoRGB(currentHue_, currentLuminance_, currentSaturation_);

            CWnd::OnLButtonDown(nFlags, point);
            slPickerPosition_ = point;
            slGraphDirty_ = true;
        }

        if (!alphaSelection_)
            alphaBarDirty_ = true;

        // Save and restore the HSL values between the call to 
        // UpdateParent.  This is necessary because the parent can
        // call SetRGB with integerised RGB coordinates.  This can cause
        // accuracy problems when the hue is small (due to the pinching
        // of the appex of the HSL cone when H is near zero).
        float oldHue = currentHue_;
        float oldLum = currentLuminance_;
        float oldSat = currentSaturation_;
        UpdateParent(WM_CP_LBUTTONMOVE);
        currentHue_        = oldHue;
        currentLuminance_  = oldLum;
        currentSaturation_ = oldSat;
        Invalidate();
    }

    CWnd::OnMouseMove(nFlags, point);
}


void ColorPicker::OnLButtonUp(UINT nFlags, CPoint point) 
{
	BW_GUARD;

    ReleaseCapture();
    mouseMoveMode_ = MMM_NONE;    
    CWnd::OnLButtonUp(nFlags, point);
    UpdateParent(WM_CP_LBUTTONUP);
}


void ColorPicker::OnPaint() 
{
	BW_GUARD;

    // Setup a backbuffer:
    CPaintDC dcPaint(this);
    CDC dc;
    dc.CreateCompatibleDC(&dcPaint);
    CRect rectClient; 
    GetClientRect(rectClient);
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(&dcPaint, rectClient.Width(), rectClient.Height());
    CBitmap *oldBitmap = dc.SelectObject(&bitmap);

    // Fill background first
    CBrush bgBrush(GetSysColor(COLOR_BTNFACE));
    dc.FillRect(&rectClient, &bgBrush);

    // Draw a border:
    dc.Draw3dRect
    (
        0, 
        0, 
        rectClient.Width(), 
        rectClient.Height(), 
        RGB(0, 0, 0), 
        RGB(0, 0, 0)
    );

    if (hueBarDirty_)
        generateHueBar();

    if (alphaBarDirty_ || (!alphaSelection_ && slGraphDirty_))
        generateAlphaBar();

    if (slGraphDirty_)
        generatePicker();

    if (slPickerDirty_)
        setPicker(false);

    dc.BitBlt
    (
        slGraphRect_.left,
        slGraphRect_.top,
        slGraphRect_.Width(),
        slGraphRect_.Height(),
        &pickerDC_, 
        0, 
        0, 
        SRCCOPY
    );
    dc.BitBlt
    (
        hueBarRect_.left,
        hueBarRect_.top,
        hueBarRect_.Width(),
        hueBarRect_.Height(),
        &hueBarDC_, 
        0, 
        0, 
        SRCCOPY
    );
    dc.BitBlt
    (
        alphaBarRect_.left,
        alphaBarRect_.top,
        alphaBarRect_.Width(),
        alphaBarRect_.Height(),
        &alphaBarDC_, 
        0, 
        0, 
        SRCCOPY
    );

    // Draw sl picker:
    CSize sz(6, 6);
    drawCrossAt
    (
        slPickerPosition_, 
        dc, 
        sz
    );
    
    // Draw hue indicator:
    sz.cx = hueBarRect_.Width();
    sz.cy = 6;
    CPoint 
        pt
        (
            slGraphRect_.Width() + c_spacer_ + hueBarRect_.Width()/2 + c_borderSpace_, 
            (int)(currentHue_*hueBarRect_.Height()) + c_borderSpace_
        );
    drawCrossAt(pt, dc, sz);

    if (alphaSelection_)
    {
        // Draw alpha indicator:
        sz.cx = 6;
        sz.cy = alphaBarRect_.Height();
        pt = 
            CPoint
            (
                (int)(currentAlpha_ * alphaBarRect_.Width()) + c_borderSpace_, 
                (int)(slGraphRect_.Height() + c_spacer_ + alphaBarRect_.Height()/2) + c_borderSpace_
            );
        drawCrossAt(pt, dc, sz);
    }

    // Draw the backbuffer:
    dcPaint.BitBlt
    (
        rectClient.left, 
        rectClient.top, 
        rectClient.Width(), 
        rectClient.Height(), 
        &dc, 
        0, 
        0,
        SRCCOPY
    );
    dc.SelectObject(oldBitmap);
    dc.DeleteDC();
    bitmap.DeleteObject();
}


BOOL ColorPicker::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	BW_GUARD;

    HCURSOR curs = LoadCursor(NULL, IDC_CROSS);
    SetCursor(curs);
    return false;
}


void ColorPicker::OnSize(UINT nType, int cx, int cy) 
{
	BW_GUARD;

    CWnd::OnSize(nType, cx, cy);
    
    // compute size of the entire window
    calculateSizes();
}


void ColorPicker::generatePicker()
{
	BW_GUARD;

    if (createPickerDC_)
    {
        // Create bitmap:
        CBitmap pickerBmp;
        pickerBmp.CreateCompatibleBitmap(
        				GetDC(), 
						slGraphRect_.Width(), 
						slGraphRect_.Height() );

        // create picker DC 
        pickerDC_.CreateCompatibleDC( GetDC() );
        pickerDC_.SelectObject( &pickerBmp );
        
        createPickerDC_ = false;
    }   

    CBitmap * pPickerBmp = pickerDC_.GetCurrentBitmap();
    ASSERT(pPickerBmp);

    // Fill picker bitmap. Using SetPixel here is fast enough, and fixes a
	// strange issue with StretchDIBits not working early on the app's life.
    for(int j = slGraphRect_.Height() - 1; j >= 0 ; j--)
    {
        float theSaturation =  (float)j/slGraphRect_.Height();

        for(int i = 0; i < slGraphRect_.Width(); i++)
        {
            float theLuminance = (float)i/slGraphRect_.Width();

            // rearrange RGB to BGR (what StretchDIBits wants)
            float r, g, b;
            HLStoRGB(currentHue_, theLuminance, theSaturation, r, g, b);

			pickerDC_.SetPixelV( i, j, RGB( r * 255, g * 255, b * 255 ) );
        }
    }

    slGraphDirty_ = false;
}


void ColorPicker::generateHueBar()
{
	BW_GUARD;

    if (createHueBarDC_)
    {
        // Create bitmap:
        CBitmap hueBmp;
        hueBmp.CreateCompatibleBitmap( GetDC(), 
									hueBarRect_.Width(), 
									hueBarRect_.Height() );

        // Create picker DC:
        hueBarDC_.CreateCompatibleDC( GetDC() );
        hueBarDC_.SelectObject( &hueBmp );

        createHueBarDC_ = false;
    }

    // Fill hue bitmap:
    COLORREF col;
    float defaultLuminance = 0.5f;
    float defaultSaturation = 1.0f;
    for(int j = 0; j < hueBarRect_.Height(); ++j)
    {
        float theHue = (float)j/(float)hueBarRect_.Height();
        col = HLStoRGB(theHue, defaultLuminance, defaultSaturation);
        hueBarDC_.FillSolidRect(0, j, hueBarRect_.Width(), 1, col);
    }

    hueBarDirty_ = false;
}


void ColorPicker::generateAlphaBar()
{
	BW_GUARD;

    if (createAlphaBarDC_)
    {
        // Create bitmap:
        CBitmap alphaBmp;
        alphaBmp.CreateCompatibleBitmap( GetDC(), 
										alphaBarRect_.Width(), 
										alphaBarRect_.Height() );

        // Create picker DC:
        alphaBarDC_.CreateCompatibleDC( GetDC() );
        alphaBarDC_.SelectObject( &alphaBmp );

        createAlphaBarDC_ = false;
    }

    if (alphaSelection_)
    {
        // Fill the bitmap:
        for(int i = 0; i < alphaBarRect_.Width(); ++i)
        {
            float theAlpha = (float)i / (float)alphaBarRect_.Width();
            COLORREF col = HLStoRGB(0.64f, theAlpha, 0.12f);    // just pick a color to blend
            alphaBarDC_.FillSolidRect(i, 0, 1, alphaBarRect_.Height(), col);
        }
    }
    else
    {
        // Treat as a color previewer:
        alphaBarDC_.FillSolidRect
        (
            0, 
            0, 
            alphaBarRect_.Width(), 
            alphaBarRect_.Height(), 
            currentColor_
        );
    }

    alphaBarDirty_ = false;
}


void ColorPicker::setPicker(bool redraw /*=true*/)
{
	BW_GUARD;

    slPickerPosition_.x =
        (long)((float)slGraphRect_.Width ()*currentLuminance_)
        +
        c_borderSpace_;
    slPickerPosition_.y =
        (long)((float)slGraphRect_.Height()*currentSaturation_)
        +
        c_borderSpace_;
    slPickerDirty_ = false;

    if (redraw)
        Invalidate();
}


void ColorPicker::drawCrossAt(CPoint &point, CDC &dc, CSize &sz)
{
	BW_GUARD;

    CPoint localPoint = point;
    localPoint.x -= sz.cx / 2;
    localPoint.y -= sz.cy / 2;

    CRect localRect(localPoint, sz);    
    dc.DrawEdge(localRect, EDGE_BUMP, BF_TOPLEFT | BF_BOTTOMRIGHT);
}


void ColorPicker::calculateSizes() 
{
	BW_GUARD;

    GetClientRect(&totalRect_);
    int cwidth  = totalRect_.Width();
    int cheight = totalRect_.Height();

    slGraphRect_ = 
        CRect
        (
            c_borderSpace_, 
            c_borderSpace_, 
            cwidth  - c_hueBarWidth_    - c_spacer_ - c_borderSpace_,
            cheight - c_alphaBarHeight_ - c_spacer_ - c_borderSpace_
        );

    hueBarRect_ = 
        CRect
        (
            c_borderSpace_ + slGraphRect_.Width() + c_spacer_,
            c_borderSpace_,
            c_borderSpace_ + slGraphRect_.Width() + c_spacer_ + c_hueBarWidth_,
            cheight - c_alphaBarHeight_ - c_borderSpace_ - c_spacer_
        );

    alphaBarRect_ = 
        CRect
        (
            c_borderSpace_,
            cheight - c_alphaBarHeight_ - c_borderSpace_,
            cwidth  - c_hueBarWidth_    - c_spacer_ - c_borderSpace_,
            cheight - c_borderSpace_
        );

    createPickerDC_ = true;
    createHueBarDC_ = true;
    slGraphDirty_ = true;
    hueBarDirty_ = true;
}


void ColorPicker::UpdateParent(UINT message)
{
	BW_GUARD;

    CWnd *parent = GetParent();
    if (parent != NULL)
    {
        parent->SendMessage
        (
            message,
            (WPARAM)GetDlgCtrlID(),
            (LPARAM)GetSafeHwnd()
        );
    }
}

} // namespace controls
BW_END_NAMESPACE
