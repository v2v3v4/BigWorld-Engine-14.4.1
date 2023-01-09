#include "pch.hpp"
#include "color_picker.hpp"

namespace
{
    // Conversion between the HSL (Hue, Saturation, and Luminosity in float)
    // and RBG (Red, Green, Blue in ranges 0-255) color model.
    // see:
    // Fundamentals of Interactive Computer Graphics by Foley and van Dam. 

    float HuetoRGB(float m1, float m2, float h )
    {
    if (h < 0)
        h += 1.0f;
    if (h > 1)
        h -= 1.0f;
    if (6.0f*h < 1)
        return (m1+(m2-m1)*h*6.0f);
    if (2.0f*h < 1)
        return m2;
    if (3.0f*h < 2.0f)
        return (m1+(m2-m1)*((2.0f/3.0f)-h)*6.0f);
    return m1;
    }

    void HLStoRGB(float H, float L, float S, float & r, float & g, float & b)
    {
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
        float r, g, b;
        HLStoRGB(H, L, S, r, g, b);
        return RGB((BYTE)(r*255), (BYTE)(g*255), (BYTE)(b*255));
    }

    void RGBtoHLS(COLORREF rgb, float & H, float & L, float & S)
    {   
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
            H = 2.0f +(b - r) / delta;
        else          
            H = 4.0f + (r - g) / delta;
        H /= 6.0f; 
        if(H < 0.0f)
            H += 1;  
        }
    }

    const int c_hueBarWidth_        = 20;
    const int c_alphaBarHeight_     = 20;
    const int c_spacer_             = 20;
    const int c_heightOfFinalColor_ =  6;
}

BEGIN_MESSAGE_MAP(ColorPicker, CWnd)
    ON_WM_LBUTTONDOWN()
    ON_WM_PAINT()
    ON_WM_SETCURSOR()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_SIZE()
END_MESSAGE_MAP()

ColorPicker::ColorPicker()
:
CWnd(),
createPickerDC_(true),
createHueBarDC_(true),
createAlphaBarDC_(true),
hueBarDirty_(true),
slGraphDirty_(true),
slPickerDirty_(true),
alphaBarDirty_(true),
pickerDC_(NULL),
hueBarDC_(NULL),
alphaBarDC_(NULL),
mouseMoveMode_(MMM_NONE),
alphaSelection_(true)
{
    pOnMove             = NULL;
    pOnLDown            = NULL;
    slGraphSize_.cx     = 100;
    slGraphSize_.cy     = 100;
    currentColor_       = RGB(0, 0, 255);   // blue
    currentAlpha_       = 1.f;
    RGBtoHLS(currentColor_, currentHue_, currentLuminance_, currentSaturation_);    
}

ColorPicker::~ColorPicker()
{
    if (pickerDC_)
        delete pickerDC_;
    if (hueBarDC_)
        delete hueBarDC_;
    if (alphaBarDC_)
        delete alphaBarDC_;
}

BOOL 
ColorPicker::Create
(
    DWORD       dwStyle, 
    CRect       rcPos, 
    CWnd        *pParent, 
    bool        alphaSelection
)
{
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

void 
ColorPicker::registerCallbackOnMove
(
    CallbackFunc    ptr, 
    void            *userData
) 
{ 
    pOnMove  = ptr;
    onMoveCD = userData;
}

void 
ColorPicker::registerCallbackOnLButtonDown
(
    CallbackFunc    ptr, 
    void            *userData
)
{ 
    pOnLDown  = ptr; 
    onLDownCD = userData; 
}

void ColorPicker::setRGB(COLORREF ref)
{
    RGBtoHLS(ref, currentHue_, currentLuminance_, currentSaturation_);
    currentColor_ = ref;

    slPickerDirty_ = true;
    slGraphDirty_ = true;

    if (pOnLDown)
        pOnLDown(currentColor_, onLDownCD);

    Invalidate();
}

COLORREF ColorPicker::getRGB()
{
    return currentColor_;
}

void ColorPicker::setHLS(float hue,float luminance, float saturation)
{
    currentHue_ = hue;
    currentSaturation_ = saturation;
    currentLuminance_ = luminance;

    currentColor_ = HLStoRGB(currentHue_, currentLuminance_, currentSaturation_);
    slPickerDirty_ = true;
    slGraphDirty_ = true;

    if (pOnLDown)
        pOnLDown(currentColor_, onLDownCD);

    Invalidate();
}

void ColorPicker::getHLS(float & hue, float & luminance, float & saturation)
{
    hue = currentHue_;
    luminance = currentLuminance_;
    saturation = currentSaturation_;
}

void ColorPicker::setRGBA(const Vector4 & color)
{
    currentColor_ = RGB(color.x * 255, color.y * 255, color.z * 255);
    if (alphaSelection_)
        currentAlpha_ = color.w;

    RGBtoHLS(currentColor_, currentHue_, currentLuminance_, currentSaturation_);
    
    slPickerDirty_ = true;
    slGraphDirty_ = true;

    if (pOnLDown)
        pOnLDown(currentColor_, onLDownCD);

    if ( GetSafeHwnd() )
        Invalidate();
}

Vector4 ColorPicker::getRGBA() const
{
    return Vector4(GetRValue(currentColor_) / 255.f, GetGValue(currentColor_) / 255.f, GetBValue(currentColor_) / 255.f, currentAlpha_);
}

void ColorPicker::OnLButtonDown(UINT nFlags, CPoint point) 
{
    if (!slGraphDirty_)
    {
        if (point.x > slGraphSize_.cx + c_spacer_)
        {
            mouseMoveMode_ = MMM_HUE;
        }
        else if (point.y > slGraphSize_.cy + c_spacer_)
        {
            mouseMoveMode_ = MMM_ALPHA;
        }
        else if ((point.y <= slGraphSize_.cy) && (point.x <= slGraphSize_.cx))
        {
            mouseMoveMode_ = MMM_SL_PICKER;
        }

        OnMouseMove(nFlags, point);

        // call callback function
        if (pOnLDown)
            pOnLDown(currentColor_, onLDownCD);

        GetParent()->UpdateData();

        Invalidate();
    }

    CWnd::OnLButtonDown(nFlags, point);
}

void ColorPicker::OnPaint() 
{
    CPaintDC dc(this); // device context for painting

    if (hueBarDirty_)
        generateHueBar();

    if (alphaBarDirty_ || (!alphaSelection_ && slGraphDirty_))
        generateAlphaBar();

    if (slGraphDirty_)
        generatePicker();

    if (slPickerDirty_)
        setPicker();

    dc.BitBlt(0, 0, slGraphSize_.cx, slGraphSize_.cy, pickerDC_, 0, 0, SRCCOPY);
    dc.BitBlt(slGraphSize_.cx + c_spacer_, 0, hueBarSize_.cx, hueBarSize_.cy, hueBarDC_, 0, 0, SRCCOPY);
    dc.BitBlt(0, slGraphSize_.cy + c_spacer_, alphaBarSize_.cx, alphaBarSize_.cy, alphaBarDC_, 0, 0, SRCCOPY);

    // draw sl picker
    CSize sz(6, 6);
    drawCrossAt(slPickerPosition_, dc, sz);
    
    // draw hue indicator
    sz.cx = hueBarSize_.cx;
    sz.cy = 6;
    CPoint pt(slGraphSize_.cx + c_spacer_ + hueBarSize_.cx / 2, (int)(currentHue_ * hueBarSize_.cy));
    drawCrossAt(pt, dc, sz);

    if (alphaSelection_)
    {
        // draw alpha indicator
        sz.cx = 6;
        sz.cy = alphaBarSize_.cy;
        pt = CPoint((int)(currentAlpha_ * alphaBarSize_.cx), (int)(slGraphSize_.cy + c_spacer_ + alphaBarSize_.cy / 2));
        drawCrossAt(pt, dc, sz);
    }
}

BOOL ColorPicker::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
    HCURSOR curs = LoadCursor(NULL, IDC_CROSS);
    SetCursor(curs);
    return false;
}

void ColorPicker::OnMouseMove(UINT nFlags, CPoint point) 
{
    if (!slGraphDirty_ && (nFlags & MK_LBUTTON))
    {
        if (mouseMoveMode_ == MMM_HUE)
        {
            if (point.y > hueBarSize_.cy)
                point.y = hueBarSize_.cy;

            currentHue_ = (float)point.y / (float)hueBarSize_.cy;
            currentColor_ = HLStoRGB(currentHue_, currentLuminance_, currentSaturation_);
            slGraphDirty_ = true;
        }
        else if (alphaSelection_ && (mouseMoveMode_ == MMM_ALPHA))
        {
            if (point.x > alphaBarSize_.cx)
                point.x = alphaBarSize_.cx;

            currentAlpha_ = (float)point.x / (float)alphaBarSize_.cx;
        }
        else if (mouseMoveMode_ == MMM_SL_PICKER)
        {
            if (point.x > slGraphSize_.cx)
                point.x = slGraphSize_.cx;
            if (point.y > slGraphSize_.cy)
                point.y = slGraphSize_.cy;
            
            if (nFlags & MK_CONTROL)
                point.x = slPickerPosition_.x;
            if (nFlags & MK_SHIFT)
                point.y = slPickerPosition_.y;

            currentLuminance_ = (float)point.x / (float)slGraphSize_.cx;
            currentSaturation_ = (float)point.y / (float)slGraphSize_.cy;
            currentColor_ = HLStoRGB(currentHue_, currentLuminance_, currentSaturation_);

            CWnd::OnLButtonDown(nFlags, point);
            slPickerPosition_ = point;
        }

        // call callback function

        if (pOnMove)
            pOnMove(currentColor_,onMoveCD);

        if (!alphaSelection_)
            alphaBarDirty_ = true;

        // Save and restore the HSL values between the call to 
        // GetParent->UpdateData.  This is necessary because the parent can
        // call SetRGB with integerised RGB coordinates.  This can cause
        // accuracy problems when the hue is small (due to the pinching
        // of the appex of the HSL cone when H is near zero).
        float oldHue = currentHue_;
        float oldLum = currentLuminance_;
        float oldSat = currentSaturation_;
        GetParent()->UpdateData();
        currentHue_        = oldHue;
        currentLuminance_  = oldLum;
        currentSaturation_ = oldSat;

        Invalidate();
    }

    CWnd::OnMouseMove(nFlags, point);
}

void ColorPicker::OnLButtonUp(UINT nFlags, CPoint point) 
{
    mouseMoveMode_ = MMM_NONE;
    
    CWnd::OnLButtonUp(nFlags, point);
}

void ColorPicker::OnSize(UINT nType, int cx, int cy) 
{
    CWnd::OnSize(nType, cx, cy);
    
    // compute size of the entire window
    calculateSizes();
}

void ColorPicker::generatePicker()
{
    if(createPickerDC_)
    {
        // create bitmap
        CBitmap *pPickerBmp;
        pPickerBmp = new CBitmap();
        pPickerBmp->CreateCompatibleBitmap( GetDC(), slGraphSize_.cx, slGraphSize_.cy);
        if(pickerDC_)
            delete pickerDC_;
        
        pickerDC_ = new CDC();
        
        // create picker DC 
        pickerDC_->CreateCompatibleDC( GetDC() );
        pickerDC_->SelectObject(pPickerBmp);
        
        delete(pPickerBmp); // we don't need this object anymore
    
        createPickerDC_ = false;
    }   

    CBitmap * pPickerBmp = pickerDC_->GetCurrentBitmap();
    ASSERT(pPickerBmp);

    // fill picker bitmap
    const int colorArrayLength = slGraphSize_.cx * slGraphSize_.cy;
    COLORREF * colorArray = new COLORREF[colorArrayLength];
    COLORREF * colorArrayPointer = colorArray;

    for(int j = slGraphSize_.cy - 1; j >= 0 ; j--)
    {
        float theSaturation =  (float)j / slGraphSize_.cy;

        for(int i = 0; i < slGraphSize_.cx; i++)
        {
            float theLuminance = (float)i / slGraphSize_.cx;

            // rearrange RGB to BGR (what StretchDIBits wants)
            float r, g, b;
            HLStoRGB(currentHue_, theLuminance, theSaturation, r, g, b);
            COLORREF ref = RGB(b * 255, g * 255, r * 255);

            *colorArrayPointer = ref;
            colorArrayPointer++;
        }
    }

    // render to bitmap
    BITMAPINFO info;
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = slGraphSize_.cx;
    info.bmiHeader.biHeight = slGraphSize_.cy;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = sizeof(COLORREF) * 8;
    info.bmiHeader.biCompression = BI_RGB;

    StretchDIBits
    (
        pickerDC_->m_hDC, 
        0, 
        0,
        slGraphSize_.cx, 
        slGraphSize_.cy,  // screen coordinates
        0, 
        0, 
        slGraphSize_.cx, 
        slGraphSize_.cy,             // bitmap coordinates
        colorArray, 
        &info, 
        DIB_RGB_COLORS, 
        SRCCOPY
    );

    delete [] colorArray;

    slGraphDirty_ = false;
}

void ColorPicker::generateHueBar()
{
    if(createHueBarDC_)
    {
        // create bitmap
        CBitmap *pHueBmp;
        pHueBmp = new CBitmap();
        pHueBmp->CreateCompatibleBitmap(GetDC(), hueBarSize_.cx, hueBarSize_.cy);

        // create picker DC 
        if(hueBarDC_)
            delete hueBarDC_;
        hueBarDC_ = new CDC();

        hueBarDC_->CreateCompatibleDC( GetDC() );
        hueBarDC_->SelectObject(pHueBmp);

        delete pHueBmp; // we don't need this object anymore
        
        createHueBarDC_ = false;
    }

    // fill hue bitmap
    COLORREF col;
    float defaultLuminance = 0.5f;
    float defaultSaturation = 1.0f;
    for(int j = 0; j < hueBarSize_.cy; j++)
    {
        float theHue = (float)j/(float)hueBarSize_.cy;
        col = HLStoRGB(theHue, defaultLuminance, defaultSaturation);
        hueBarDC_->FillSolidRect(0, j, hueBarSize_.cx, 1, col);
    }

    hueBarDirty_ = false;
}

void ColorPicker::generateAlphaBar()
{
    if(createAlphaBarDC_)
    {
        // create bitmap
        CBitmap *pAlphaBmp = new CBitmap();
        pAlphaBmp->CreateCompatibleBitmap(GetDC(), alphaBarSize_.cx, alphaBarSize_.cy);

        // create picker DC 
        if(alphaBarDC_)
            delete alphaBarDC_;
        alphaBarDC_ = new CDC();

        alphaBarDC_->CreateCompatibleDC( GetDC() );
        alphaBarDC_->SelectObject(pAlphaBmp);

        delete pAlphaBmp; // we don't need this object anymore
        
        createAlphaBarDC_ = false;
    }

    if (alphaSelection_)
    {
        // fill the bitmap
        for(int i = 0; i < alphaBarSize_.cx; i++)
        {
            float theAlpha = (float)i / (float)alphaBarSize_.cx;
            COLORREF col = HLStoRGB(0.64f, theAlpha, 0.12f);    // just pick a color to blend
            alphaBarDC_->FillSolidRect(i, 0, 1, alphaBarSize_.cy, col);
        }
    }
    else
    {
        // treat as a color previewer
        alphaBarDC_->FillSolidRect(0, 0, alphaBarSize_.cx, alphaBarSize_.cy, currentColor_);
    }

    alphaBarDirty_ = false;
}

void ColorPicker::setPicker()
{
    slPickerPosition_.x =(long)((float)slGraphSize_.cx * currentLuminance_);    
    slPickerPosition_.y =(long)((float)slGraphSize_.cy * currentSaturation_);   
    slPickerDirty_ = false;

    Invalidate();
}

void ColorPicker::drawCrossAt(CPoint &point, CPaintDC &dc, CSize &sz)
{
    CPoint localPoint = point;
    localPoint.x -= sz.cx / 2;
    localPoint.y -= sz.cy / 2;

    CRect localRect(localPoint, sz);    
    dc.DrawEdge(localRect, EDGE_BUMP, BF_TOPLEFT | BF_BOTTOMRIGHT);
}

void ColorPicker::calculateSizes() 
{
    RECT rect;
    GetClientRect(&rect);
    int maxi = rect.right - rect.left;
    int maxj = rect.bottom - rect.top;
    
    slGraphSize_.cx = maxi - c_hueBarWidth_ - c_spacer_;
    slGraphSize_.cy = maxj - c_alphaBarHeight_ - c_spacer_;

    hueBarSize_.cx = c_hueBarWidth_;
    hueBarSize_.cy = maxj - c_alphaBarHeight_ - c_spacer_;

    alphaBarSize_.cx = slGraphSize_.cx;
    alphaBarSize_.cy = c_alphaBarHeight_;

    totalSize_.cx = maxi;
    totalSize_.cy = maxj;

    createPickerDC_ = true;
    createHueBarDC_ = true;
    slGraphDirty_ = true;
    hueBarDirty_ = true;
}
