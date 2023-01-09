#include "pch.hpp"
#include "controls/color_timeline.hpp"
#include "controls/color_static.hpp"
#include "controls/user_messages.hpp"
#include "romp/time_of_day.hpp"
#include "cstdmf/string_utils.hpp"
#include <limits>

BW_BEGIN_NAMESPACE
namespace controls
{

namespace
{
    //
    // Spacing constants used to determine the geometry of the parts of the
    // ColorTimeline.
    //
    const int horizBorder               =  5;
    const int staticWidth               = 30;
    const int alphaWidth                = 20;
    const int colorWidth                = 45;
    const int markWidth                 = 30;
    const int vertBorder                = 15;
    const int indicatorWidth            =  5;
    const int indicatorHeight           =  3;
    const int indicatorClrWidth         = 20;
    const int indicatorClrHeight        = 15;
    const int selIndClrWidth            =  7;

    COLORREF RGBtoCOLORREF(const Vector4 & color)
    {
	    return 
            RGB
            (
                (BYTE)(color.x*255), 
                (BYTE)(color.y*255), 
                (BYTE)(color.z*255)
            );
    }

    // The difference between two normalized times at which point we regard 
    // them as being the same:
    const float TIME_EPSILON    = 1e-4f;
}


/**
 *  Constructor.
 */
ColorScheduleItem::ColorScheduleItem()
:
normalisedTime_(0.0f),
color_(),
heightPos_(0),
timeStatic_(NULL)
{
}


/**
 *  Comparison (by time).
 *
 *  @param other        The other item to compare.
 *  @returns            True if this time is less than other's time.
 */
bool ColorScheduleItem::operator<(ColorScheduleItem const &other) const
{
    return normalisedTime_ < other.normalisedTime_;
}


BEGIN_MESSAGE_MAP(ColorTimeline, CWnd)
    ON_WM_PAINT()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_ERASEBKGND()
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()


IMPLEMENT_DYNAMIC(ColorTimeline, CWnd)


/**
 *  Constructor.
 */
ColorTimeline::ColorTimeline()
:
CWnd(), 
colorSchedule_(),
colorGradientHeight_(0),
colorScheduleIterSelected_(),
mousePosition_(),
unitHeightTimeIncrement_(0.0f),
totalScheduleTime_(0.0f),
addColorAtLButton_(false),
showAlpha_(true),
timeScale_(TS_SECONDS),
wrap_(false),
lineAtTime_(-1.0f),
dragging_(false),
startPos_(0)
{
}


/**
 *  Destructor.
 */
ColorTimeline::~ColorTimeline()
{
}


/**
 *  Create the control
 *  
 *  @param style            The style of the control.  Usually this is
 *                          WS_VISIBLE | WS_CHILD.
 *  @param rcPos            The position of the control.
 *  @param parent           The parent control.
 *  @param colorSchedule    The list of colours.
 *  @param showAlpha        Show the alpha channel?
 *  @param timeScale        The units of the time.
 *  @param wrap             Does the first value equal the last value?
 *  @returns                TRUE if successfully created.
 */
BOOL 
ColorTimeline::Create
(
    DWORD               dwStyle, 
    CRect               rcPos, 
    CWnd                *pParent, 
    ColorScheduleItems  const &inColorSchedule,
    bool                showAlpha           /* = true */,
    TimeScale           timeScale           /* = TS_SECONDS */,
    bool                wrap                /* = false */
)
{
	BW_GUARD;

    LPVOID lp = (LPVOID)NULL;
    if
    (
        !CreateEx
        (
            0,
            AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW)), 
            _T("ColorTimeline"),
            dwStyle,
            rcPos.left,
            rcPos.top,
            rcPos.Width(),
            rcPos.Height(),
            pParent->GetSafeHwnd(),
            (HMENU)(4232),
            lp
        )
    )       
    {
        TRACE0("Failed to create ColorTimeline\n");
        return FALSE;
    }

    // Add some dummy items to make the timeline valid if necessary:
    ColorScheduleItems colorSchedule = inColorSchedule;
    if (colorSchedule.size() == 1)
    {
        ColorScheduleItem dummyItem;
        dummyItem.color_ = colorSchedule[0].color_;
        if (colorSchedule[0].normalisedTime_ == 0.0f)
        {
            dummyItem.normalisedTime_ = 1.0f;
            colorSchedule.push_back(dummyItem);
        }
        else
        {
            dummyItem.normalisedTime_ = 0.0f;
            colorSchedule.insert(colorSchedule.begin(), dummyItem);
        }
    }
    std::sort(colorSchedule_.begin(), colorSchedule_.end());

    showAlpha_  = showAlpha;
    timeScale_  = timeScale;
    wrap_       = wrap;

    // Since we know the size of the window, setup the mapping to window
    // coordinates:
    CRect rectClient;           // draw area
    GetClientRect(rectClient);
    colorGradientHeight_ = rectClient.Height() - 2*vertBorder;
    unitHeightTimeIncrement_ = 1.0f/colorGradientHeight_;

    // Fill the color schedule:
    CWnd *parent = GetParent();
    CFont * font = NULL;
    if (parent != NULL)
        font = parent->GetFont();
    for 
    ( 
        ColorScheduleItems::const_iterator iter = colorSchedule.begin(); 
        iter != colorSchedule.end(); 
        ++iter
    )
    {
        ColorScheduleItem newItem;
        newItem.normalisedTime_ = iter->normalisedTime_;
        newItem.color_          = iter->color_;
        newItem.heightPos_      = 
            vertBorder + (int)(iter->normalisedTime_/unitHeightTimeIncrement_);
        newItem.timeStatic_ = new ColorStatic();
        newItem.timeStatic_->Create(_T("0"), 0, CRect(0,0,0,0), this, 2);
        newItem.timeStatic_->SetBkColour(ColorStatic::TransparentBackground());
        if (font != NULL)
            newItem.timeStatic_->SetFont(font);
        BW::wstring timeStr = 
            timeToString(newItem.normalisedTime_*totalScheduleTime_);
        newItem.timeStatic_->SetWindowText(timeStr.c_str());
        colorSchedule_.push_back(newItem);
    }
    if (wrap && colorSchedule.begin() != colorSchedule.end())
    {
        ColorScheduleItems::const_iterator iter = colorSchedule.begin();
        ColorScheduleItem newItem;
        newItem.normalisedTime_ = 1.0f;
        newItem.color_          = iter->color_;
        newItem.heightPos_      = 
            vertBorder + (int)(1.0f/unitHeightTimeIncrement_);
        newItem.timeStatic_ = new ColorStatic();
        newItem.timeStatic_->Create(_T("0"), 0, CRect(0,0,0,0), this, 2);
        newItem.timeStatic_->SetBkColour(ColorStatic::TransparentBackground());
        if (font != NULL)
            newItem.timeStatic_->SetFont(font);
        BW::wstring timeStr = 
            timeToString(newItem.normalisedTime_*totalScheduleTime_);
        newItem.timeStatic_->SetWindowText(timeStr.c_str());
        colorSchedule_.push_back(newItem);
    }

    colorScheduleIterSelected_ = colorSchedule_.end();

    ShowWindow(SW_SHOW);
    return TRUE;
}


/**
 *  Get the list of colors.
 *
 *  @returns                The list of colors.
 */
ColorScheduleItems const &ColorTimeline::colourScheduleItems() const
{
    return colorSchedule_;
}


/**
 *  Clear the list of colours.
 */
void ColorTimeline::clearSchedule()
{
	BW_GUARD;

    colorSchedule_.clear();
    colorScheduleIterSelected_ = colorSchedule_.end();
}


/**
 *  Add an item to the list of colors. 
 * 
 *  @param item             The new item to add.
 */
void ColorTimeline::addScheduleItem(ColorScheduleItem const &item)
{
	BW_GUARD;

    ColorScheduleItem newItem = item;
    newItem.heightPos_      = 
        vertBorder + (int)(newItem.normalisedTime_/unitHeightTimeIncrement_);
    newItem.timeStatic_ = new ColorStatic();
    newItem.timeStatic_->Create(_T("0"), 0, CRect(0, 0, 0, 0), this, 2);
    newItem.timeStatic_->SetBkColour(ColorStatic::TransparentBackground());
    CWnd *parent = GetParent();
    CFont * font = NULL;
    if (parent != NULL)
        font = parent->GetFont();
    if (font != NULL)
        newItem.timeStatic_->SetFont(font);
    BW::wstring timeStr = 
        timeToString(newItem.normalisedTime_*totalScheduleTime_);
    newItem.timeStatic_->SetWindowText(timeStr.c_str());
    colorSchedule_.push_back(newItem);
    std::sort(colorSchedule_.begin(), colorSchedule_.end());
    colorScheduleIterSelected_ = colorSchedule_.end();
}


/**
 *  Set the total time.
 * 
 *  @param  time            The total time for the timeline.
 */
void ColorTimeline::totalScheduleTime(float time)
{
	BW_GUARD;

    totalScheduleTime_ = time;

    // Fill the new times:
    for 
    ( 
        ColorScheduleItems::const_iterator iter = colorSchedule_.begin(); 
        iter != colorSchedule_.end(); 
        ++iter
    )
    {
        BW::wstring timeStr = 
            timeToString(iter->normalisedTime_*totalScheduleTime_);
        iter->timeStatic_->SetWindowText(timeStr.c_str());
    }
}


/**
 *  Get the total time.
 * 
 *  @returns                The total time of the timeline.
 */
float ColorTimeline::totalScheduleTime() const
{
    return totalScheduleTime_;
}


/**
 *  Is an item selected?
 * 
 *  @returns                True if an item is selected.
 */
bool ColorTimeline::itemSelected() const
{
    return colorScheduleIterSelected_ != colorSchedule_.end();
}


/**
 *  Return the selected item.
 * 
 *  @returns                The selected item.  The result is undefined
 *                          if there is no selection.
 */
ColorScheduleItem *ColorTimeline::getColorScheduleItemSelected() const
{
	BW_GUARD;

    if (colorScheduleIterSelected_ == colorSchedule_.end())
        return NULL;

    return &(*colorScheduleIterSelected_);
}


/**
 *  Set the selected item.
 * 
 *  @param time              The normalized time of the selected item.
 */
void ColorTimeline::setColorScheduleItemSelected(float time)
{
	BW_GUARD;

    colorScheduleIterSelected_ = colorSchedule_.end();
    for
    (
        ColorScheduleItems::iterator iter = colorSchedule_.begin();
        iter != colorSchedule_.end();
        ++iter
    )
    {
        if (fabs(iter->normalisedTime_ - time) < TIME_EPSILON)
        {
            colorScheduleIterSelected_ = iter;            
            break;
        }
    }
    Invalidate();
    UpdateWindow();
}


/**
 *  Get the selected item's color.
 * 
 *  @returns                The color of the selected item.  This result is
 *                          undefined if there is no selection.  The color
 *                          is in the form rgba where each component is in 
 *                          [0, 1].
 */
Vector4 ColorTimeline::getColorScheduleItemSelectedColor()
{
	BW_GUARD;

    if (getColorScheduleItemSelected())
        return getColorScheduleItemSelected()->color_;

    return Vector4::zero();
}


/**
 *  Set the selected item's color.
 * 
 *  @param color            The color of the selected item.  If there is no
 *                          selection then nothing is done.
 *  @param redraw           If true then redraw the control.
 */
void 
ColorTimeline::setColorScheduleItemSelectedColor
(
    Vector4         const &color,
    bool            redraw      /*= true*/
)
{
	BW_GUARD;

    ColorScheduleItem *selItem = getColorScheduleItemSelected();
    if (selItem != NULL)
    {
        // Check for and handle the wrapping case:
        ColorScheduleItems::iterator firstItem = colorSchedule_.begin();
        ColorScheduleItems::iterator lastItem  = colorSchedule_.end();
        --lastItem;
        if 
        (
            wrap_
            &&
            (
                colorScheduleIterSelected_ == firstItem
                ||
                colorScheduleIterSelected_ == lastItem
            )
        )
        {
            firstItem->color_ = color;
            lastItem ->color_ = color;
        }
        // Not the wrapping case:
        else
        {
            selItem->color_ = color;
        }
        if (redraw)
            InvalidateRect(NULL, FALSE);
    }
}


/**
 *  Remove the selected item.
 * 
 *  @returns                True if the item was removed.  The result may
 *                          be false if there is no selection or if the
 *                          first or last items are tried to be deleted.
 */
bool ColorTimeline::removeSelectedColor()
{
	BW_GUARD;

    addColorAtLButton_ = false;

    // Can't delete if nothing is selected:
    if (colorScheduleIterSelected_ == colorSchedule_.end())
        return false;

    // Can't delete first or last colors:
    ColorScheduleItems::iterator lastColor = colorSchedule_.end();
    --lastColor;
    if (colorScheduleIterSelected_ == lastColor)
        return false;
    ColorScheduleItems::iterator firstColor = colorSchedule_.begin();
    if (colorScheduleIterSelected_ == firstColor)
        return false;

    // We can delete the middles ones:
    delete colorScheduleIterSelected_->timeStatic_;
    colorSchedule_.erase(colorScheduleIterSelected_);
    colorScheduleIterSelected_ = colorSchedule_.end();
    InvalidateRect(NULL, FALSE);

    return true;
}


/**
 *  Should a new color be added with the next left mouse button press?
 * 
 *  @param add              If true then the next left mouse button press
 *                          adds a new color.
 */
void ColorTimeline::addColorAtLButton(bool add /* = true*/)
{
    addColorAtLButton_ = add;
}


/**
 *  Will the next left mouse button press add a new color.
 * 
 *  @returns                True if the next mouse button press will add a
 *                          new color.
 */
bool ColorTimeline::waitingToAddColor() const
{ 
    return addColorAtLButton_; 
}

/**
 *  Show a line at a given time?
 * 
 *  @param time             The time to show the line at.
 */
void ColorTimeline::showLineAtTime(float time)
{
	BW_GUARD;

    lineAtTime_ = time;
    Invalidate();
    UpdateWindow();
}

/**
 *  Don't show a special line at any time.
 */
void ColorTimeline::hideLineAtTime()
{
	BW_GUARD;

    lineAtTime_ = -1.0f;
    Invalidate();
    UpdateWindow();
}


void ColorTimeline::PostNcDestroy()
{
	BW_GUARD;

    for 
    ( 
        ColorScheduleItems::iterator iter = colorSchedule_.begin(); 
        iter != colorSchedule_.end(); 
        ++iter
    )
    {
        bw_safe_delete(iter->timeStatic_);
    }

    CWnd::PostNcDestroy();
}


void ColorTimeline::OnPaint() 
{
	BW_GUARD;

    // initialise
    CPaintDC dcPaint(this);     // device context for painting
    CDC dcMem;                      // support raster operations
    dcMem.CreateCompatibleDC(&dcPaint);

    CRect rectClient;           // draw area
    GetClientRect(rectClient);

    CBitmap bitmap;             // hook up a bitmap
    bitmap.CreateCompatibleBitmap(&dcPaint, rectClient.Width(), rectClient.Height());
    CBitmap* oldBitmap = dcMem.SelectObject(&bitmap);

    // Fill background first:
    CBrush bgBrush(GetSysColor(COLOR_BTNFACE));
    dcMem.FillRect(&rectClient, &bgBrush);

    // Draw a border:
    dcMem.Draw3dRect(rectClient, RGB(0, 0, 0), RGB(0, 0, 0));

    // Various positions:
    CRect clrRect, alphaRect;
    CalcColorRect(clrRect);
    CalcAlphaRect(alphaRect);

    // Fill color gradient
    if (colorSchedule_.empty())
    {
        // Nothing to draw
        Vector4 currentColor;
        COLORREF fillColor = RGBtoCOLORREF(currentColor);
        dcMem.FillSolidRect(clrRect, fillColor);
    }
    else
    {
        ColorScheduleItems::iterator iter    = colorSchedule_.begin();
        ColorScheduleItems::iterator iterEnd = colorSchedule_.end();        
        ColorScheduleItems::iterator next    = iter;
        ++next;

        // Draw the colour gradients:
        for (; next != iterEnd; ++iter, ++next)
        {
            Vector4 currentColor = iter->color_;
            float colorIncrement = 
                unitHeightTimeIncrement_/(next->normalisedTime_ - iter->normalisedTime_);
            for (int y = iter->heightPos_; y < next->heightPos_; ++y)
            {
                if (showAlpha_)
                {
                    Vector4 
                        alphaColor
                        (
                            currentColor.w, 
                            currentColor.w, 
                            currentColor.w, 
                            1.0f
                        );
                    COLORREF fillColor = RGBtoCOLORREF(alphaColor);
                    dcMem.FillSolidRect
                    (
                        alphaRect.left,
                        y,
                        alphaRect.Width(),
                        1,
                        fillColor
                    );
                }
                COLORREF fillColor = RGBtoCOLORREF(currentColor);
                dcMem.FillSolidRect
                (
                    clrRect.left,
                    y,
                    clrRect.Width(),
                    1,
                    fillColor
                );
                currentColor += (next->color_ - iter->color_)*colorIncrement;
            }
        }

        // Draw the line at the special time.
        if (lineAtTime_ >= 0.0f)
        {
            int ypos = vertBorder + 
                (int)((rectClient.Height() - 2*vertBorder)*lineAtTime_/totalScheduleTime_);
            CPen redPen(PS_SOLID, 0, RGB(255, 128, 128));
            CGdiObject *oldPen = dcMem.SelectObject(&redPen);
            dcMem.MoveTo(horizBorder, ypos);
            dcMem.LineTo(rectClient.Width() - horizBorder, ypos);
            dcMem.SelectObject(oldPen);
        }

        // Draw the markers:
        for 
        (
            iter = colorSchedule_.begin(); 
            iter != colorSchedule_.end(); 
            ++iter
        )
        {
            drawColorMark(dcMem, *iter, rectClient);
        }
    }     

    // Render back buffer:
    CWnd::DefWindowProc( WM_PAINT, (WPARAM)dcMem.m_hDC, 0 );
    dcPaint.BitBlt
    (
        rectClient.left, 
        rectClient.top, 
        rectClient.Width(), 
        rectClient.Height(), 
        &dcMem, 
        0, 
        0,
        SRCCOPY
    );

    // Cleanup
    dcMem.SelectObject(oldBitmap);
    dcMem.DeleteDC();
    bitmap.DeleteObject();
}


void ColorTimeline::OnLButtonDown(UINT nFlags, CPoint point)
{
	BW_GUARD;

    // Add a new color?
    CRect alphaRect, clrRect;
    CalcColorRect(clrRect);
    CalcAlphaRect(alphaRect);
    if 
    (
        addColorAtLButton_
        &&
        (
            (showAlpha_ && alphaRect.PtInRect(point))
            ||
            clrRect.PtInRect(point)
        )
    )
    {        
        ColorScheduleItems::iterator iter     = colorSchedule_.begin();
        ColorScheduleItems::iterator iterEnd  = colorSchedule_.end();
        ColorScheduleItems::iterator next     = iter;
        ++next;
        for (; next != iterEnd; ++iter, ++next)
        {
            if 
            (
                iter->heightPos_ <= point.y
                &&
                point.y < next->heightPos_
            )
            {
                UpdateParent(WM_CT_ADDING_COLOR);

                // Add new colour here:
                ColorScheduleItem newItem;
                newItem.heightPos_ = point.y;
                newItem.normalisedTime_ = 
                    (newItem.heightPos_ - vertBorder)*unitHeightTimeIncrement_;
                
                // Interpolate between the existing colours:
                Vector4 colorDifference = next->color_ - iter->color_;
                float proportion = 
                    (float)(newItem.heightPos_ - iter->heightPos_)/(next->heightPos_ - iter->heightPos_);
                newItem.color_ = iter->color_ + proportion*colorDifference;

                // Create the label:
                newItem.timeStatic_ = new ColorStatic();
                newItem.timeStatic_->Create(_T("0"), 0, CRect(0,0,0,0), this, 2);
                newItem.timeStatic_->SetBkColour(ColorStatic::TransparentBackground());
                CWnd *parent = GetParent();
                CFont *font = (parent != NULL) ? parent->GetFont() : NULL;
                if (font != NULL)
                    newItem.timeStatic_->SetFont(font);            
                BW::wstring timeStr =
                    timeToString(newItem.normalisedTime_*totalScheduleTime_);
                newItem.timeStatic_->SetWindowText(timeStr.c_str());

                colorScheduleIterSelected_ = colorSchedule_.insert(next, newItem);

                // Update our internal state and inform the parent about the
                // new addition:
                addColorAtLButton_ = false;
                mousePosition_ = point;         // remember mouse point
                InvalidateRect(NULL, FALSE);    // force redraw
                UpdateParent(WM_CT_ADDED_COLOR);
                return;
            }
        }
        return;
    }

    // See if the press was on a color mark:
    ColorScheduleItems::iterator iter     = colorSchedule_.begin();
    ColorScheduleItems::iterator iterEnd  = colorSchedule_.end();
    ColorScheduleItems::iterator best     = iter;
    int                          bestDist = std::numeric_limits<int>::max();
    CRect                        client;
    bool                         foundOne = false;
    GetClientRect(client);
    for (; iter != iterEnd; ++iter)
    {
        CRect markRect;
        CalcMarkRect(*iter, client, markRect);
        if (markRect.PtInRect(point))
        {
            foundOne = true;
            int dist = abs(point.y - (markRect.top + markRect.bottom)/2);
            if (dist < bestDist)
            {
                best = iter;
                bestDist = dist;
            }
        }
    }

    if (foundOne)
    {
        colorScheduleIterSelected_ = best;
        mousePosition_ = point;         // remember mouse point
        InvalidateRect(NULL, FALSE);    // force redraw
        UpdateParent(WM_CT_NEW_SELECTION);
        dragging_ = true;
        startPos_ = colorScheduleIterSelected_->heightPos_;
        SetCapture();
        UpdateParent(WM_CT_UPDATE_BEGIN);
    }
    else
    {
        point.y = 
            Math::clamp
            (
                client.top    + vertBorder,
                point.y,
                client.bottom - vertBorder
            );
        float ratio = (float)(point.y - vertBorder)/(client.Height() - 2*vertBorder);
        float time  = totalScheduleTime_*ratio;
        CWnd *parent = GetParent();
        if (parent != NULL)
        {
            parent->SendMessage
            (
                WM_CT_SEL_TIME, 
                (WPARAM)GetDlgCtrlID(),
                (LPARAM)(time*10000)
            );
        }
    }
}


void ColorTimeline::OnMouseMove(UINT nFlags, CPoint point)
{
	BW_GUARD;

    CWnd::OnMouseMove(nFlags, point);

    if ((nFlags & MK_LBUTTON) == 0)
        return;

    if (dragging_)
    {
        // if nothing is selected then do nothing:
        if (colorScheduleIterSelected_ == colorSchedule_.end())
            return;

        // If is the first or the end item, do not move it!
        ColorScheduleItems::iterator iterStart = colorSchedule_.begin();
        ColorScheduleItems::iterator iterEnd   = colorSchedule_.end();
        --iterEnd;
        if (colorScheduleIterSelected_ == iterStart)
            return;
        if (colorScheduleIterSelected_ == iterEnd)
            return;

        CRect client;
        GetClientRect(client);
        point.y = 
            Math::clamp
            (
                vertBorder + 1, 
                (int)point.y, 
                client.Height() - vertBorder - 1
            );

        int verticalMouseDisplacement = point.y - mousePosition_.y;
        mousePosition_ = point;

        // Move the indicator:
        colorScheduleIterSelected_->heightPos_ += verticalMouseDisplacement;
        colorScheduleIterSelected_->normalisedTime_ = 
            (colorScheduleIterSelected_->heightPos_ - vertBorder)
            *
            unitHeightTimeIncrement_;
        BW::wstring timeStr = 
            timeToString(colorScheduleIterSelected_->normalisedTime_*totalScheduleTime_);
        colorScheduleIterSelected_->timeStatic_->SetWindowText(timeStr.c_str());

        // Test for overlap with previous elements and swap as required:
        ColorScheduleItems::iterator previous = colorScheduleIterSelected_;
        --previous;
        while (colorScheduleIterSelected_->heightPos_ <= previous->heightPos_)
        {
            if (previous == colorSchedule_.begin())
            {
                // Stop movement:
                colorScheduleIterSelected_->heightPos_ = 
                    previous->heightPos_ + 1;
                break;
            }
            else
            {
                // Put on the other side
                ColorScheduleItem temp = *previous;
                *previous = *colorScheduleIterSelected_;
                *colorScheduleIterSelected_ = temp;
                colorScheduleIterSelected_ = previous;
                colorScheduleIterSelected_->heightPos_ -= 1;
            }
            previous = colorScheduleIterSelected_;
            --previous;
        }

        // Test for overlap with next elements and swap as required:
        ColorScheduleItems::iterator next = colorScheduleIterSelected_;
        ++next;
        ColorScheduleItems::iterator lastValid = colorSchedule_.end();
        --lastValid;
        while (colorScheduleIterSelected_->heightPos_ >= next->heightPos_)
        {
            if (next == lastValid)
            {
                // Stop movement:
                colorScheduleIterSelected_->heightPos_ = next->heightPos_ - 1;
                break;
            }
            else
            {
                // Put on the other side:
                ColorScheduleItem temp = *next;
                *next = *colorScheduleIterSelected_;
                *colorScheduleIterSelected_ = temp;
                colorScheduleIterSelected_ = next;
                colorScheduleIterSelected_->heightPos_ += 1;
            }
            next = colorScheduleIterSelected_;
            ++next;
        }

        // Force redraw
        InvalidateRect(NULL, FALSE);

        // Update the real object
        UpdateParent(WM_CT_UPDATE_MIDDLE);
    }
    else
    {
        CRect client;
        GetClientRect(client);
        point.y = 
            Math::clamp
            (
                client.top    + vertBorder,
                point.y,
                client.bottom - vertBorder
            );
        float ratio = (float)(point.y - vertBorder)/(client.Height() - 2*vertBorder);
        float time  = totalScheduleTime_*ratio;
        CWnd *parent = GetParent();
        if (parent != NULL)
        {
            parent->SendMessage
            (
                WM_CT_SEL_TIME, 
                (WPARAM)GetDlgCtrlID(),
                (LPARAM)(time*10000)
            );
        }
    }
}


void ColorTimeline::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	BW_GUARD;

    if (dragging_)
    {
        ReleaseCapture();
        dragging_ = false;
        if (colorScheduleIterSelected_->heightPos_ != startPos_)
            UpdateParent(WM_CT_UPDATE_DONE);
        else
            UpdateParent(WM_CT_UPDATE_CANCEL);
    }    
}


BOOL ColorTimeline::OnEraseBkgnd(CDC* pDC) 
{
    // This control is doubled buffered, so there is no background erasing
    // to do.
    return TRUE;
}


BOOL ColorTimeline::OnSetCursor(CWnd * /*wnd*/, UINT /*hittest*/, UINT /*message*/)
{
	BW_GUARD;

    CPoint cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(&cursorPos);

    CRect alphaRect;
    CalcAlphaRect(alphaRect);
    CRect clrRect;
    CalcColorRect(clrRect);

    if 
    (
        addColorAtLButton_
        &&
        (
            (showAlpha_ && alphaRect.PtInRect(cursorPos))
            ||
            clrRect.PtInRect(cursorPos)
        )
    )
    {
        SetCursor(::LoadCursor(NULL, IDC_CROSS));
    }
    else
    {
        SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
    return TRUE;
}


void 
ColorTimeline::drawColorMark
(
    CDC                 &dc, 
    ColorScheduleItem   const &item,
    CRect               const &clientRect
)
{
	BW_GUARD;

    // Is this the selected mark?
    bool isSelected =
        colorScheduleIterSelected_ != colorSchedule_.end()
        && 
        item.heightPos_ == colorScheduleIterSelected_->heightPos_;

    // Some positions:
    int left = clientRect.Width() - markWidth;
    int clrLeft = left + (markWidth - indicatorClrWidth)/2;
    int clrTop  = item.heightPos_ - indicatorClrHeight/2;

    // Draw a line from left to right and a triangle at the end:
    CGdiObject *oldPen = dc.SelectStockObject(BLACK_PEN);
    dc.MoveTo(horizBorder + staticWidth, item.heightPos_);
    dc.LineTo(clrLeft - indicatorWidth , item.heightPos_);
    if (isSelected)
    {
        dc.MoveTo(horizBorder + staticWidth, item.heightPos_ - 1);
        dc.LineTo(clrLeft - indicatorWidth , item.heightPos_ - 1);
        dc.MoveTo(horizBorder + staticWidth, item.heightPos_ + 1);
        dc.LineTo(clrLeft - indicatorWidth , item.heightPos_ + 1);
    }
    dc.MoveTo(clrLeft - indicatorWidth, item.heightPos_);
    dc.LineTo(clrLeft, item.heightPos_ - indicatorHeight);
    dc.MoveTo(clrLeft - indicatorWidth, item.heightPos_);
    dc.LineTo(clrLeft, item.heightPos_ + indicatorHeight);

    dc.SelectObject(oldPen);

    // Draw the solid colour:
    dc.FillSolidRect
    (
        clrLeft,
        clrTop,
        indicatorClrWidth,
        indicatorClrHeight,
        RGBtoCOLORREF(item.color_)
    );

    // If selected draw a selection bit:
    if (isSelected)
    {
        dc.FillSolidRect
        (
            clrLeft + indicatorClrWidth - selIndClrWidth - 2,
            clrTop,
            selIndClrWidth + 2,
            indicatorClrHeight,
            ::GetSysColor(COLOR_BTNFACE)
        );
        dc.FillSolidRect
        (
            clrLeft + indicatorClrWidth - selIndClrWidth,
            clrTop,
            selIndClrWidth,
            indicatorClrHeight,
            RGB(102, 102, 102)
        );
    }
    item.timeStatic_->SetWindowPos
    (
        &CWnd::wndTop, 
        horizBorder, 
        item.heightPos_ - indicatorClrHeight/2,
        staticWidth,
        indicatorClrHeight,
        SWP_SHOWWINDOW | SWP_FRAMECHANGED
    );
}


void ColorTimeline::UpdateParent(UINT message)
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


BW::wstring ColorTimeline::timeToString(float time) const
{
	BW_GUARD;

    if (timeScale_ == TS_SECONDS)
    {
        wchar_t buffer[64];
        bw_snwprintf(buffer, ARRAY_SIZE(buffer), L"%.2f", time);
        return buffer;
    }
    else if (timeScale_ == TS_HOURS_MINS)
    {
		BW::wstring wtime;
		bw_utf8tow( TimeOfDay::gameTimeToString(time), wtime );
        return wtime;
    }
    else
    {
        return BW::wstring();
    }
}


void ColorTimeline::CalcColorRect(CRect &clrRect) const
{
	BW_GUARD;

    CRect client;
    GetClientRect(client);

    int gradX1    = horizBorder + staticWidth;
    int gradWidth = client.Width() - 2*horizBorder - staticWidth - markWidth;
    int alphaX1   = 0;
    int clrX1     = 0;
    if (showAlpha_)
    {
        int division = (gradWidth - alphaWidth - colorWidth)/3;
        alphaX1 = gradX1  + division;
        clrX1   = alphaX1 + alphaWidth + division;
    }
    else
    {
        clrX1   = gradX1 + (gradWidth - colorWidth)/2;
    }
    clrRect.left    = clrX1;
    clrRect.top     = vertBorder;
    clrRect.right   = clrRect.left + colorWidth;
    clrRect.bottom  = client.Height() - vertBorder;
}


void ColorTimeline::CalcAlphaRect(CRect &alphaRect) const
{
	BW_GUARD;

    if (!showAlpha_)
    {
        alphaRect = CRect(-1, -1, -1, -1);
    }
    else
    {
        CRect client;
        GetClientRect(client);

        int gradX1    = horizBorder + staticWidth;
        int gradWidth = client.Width() - 2*horizBorder - staticWidth - markWidth;
        int alphaX1   = gradX1  + (gradWidth - alphaWidth - colorWidth)/3;

        alphaRect.left    = alphaX1;
        alphaRect.top     = vertBorder;
        alphaRect.right   = alphaRect.left + alphaWidth;
        alphaRect.bottom  = client.Height() - vertBorder;
    }
}


void 
ColorTimeline::CalcMarkRect
(
    ColorScheduleItem   const &item,
    CRect               const &clientRect,
    CRect               &markRect 
) const
{
	BW_GUARD;

    int left    = clientRect.Width() - markWidth;
    int clrLeft = left + (markWidth - indicatorClrWidth)/2;
    int clrTop  = item.heightPos_ - indicatorClrHeight/2;
    markRect = 
        CRect
        (
            clrLeft, 
            clrTop, 
            clrLeft + indicatorClrWidth, 
            clrTop  + indicatorClrHeight
        );
}

} // namespace controls
BW_END_NAMESPACE

