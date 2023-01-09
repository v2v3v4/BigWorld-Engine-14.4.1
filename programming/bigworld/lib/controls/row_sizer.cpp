#include "pch.hpp"
#include "row_sizer.hpp"
#include <algorithm>

BW_BEGIN_NAMESPACE
namespace controls
{

namespace
{
    const uint32 DEFAULT_EDGER_GAP		= 2;
    const uint32 DEFAULT_CHILD_GAP		= 2;
}


/**
 *  This constructs a RowSizer.
 *
 *  @param orientation  The orientation of the RowSizer.
 */
/*explicit*/ RowSizer::RowSizer(Orientation orientation):
    orientation_(orientation),
	wnd_(NULL),
	id_(0),
    edgeGap_(DEFAULT_EDGER_GAP),
    childGap_(DEFAULT_CHILD_GAP)
{
}


/**
 *  This constructs a RowSizer.
 *
 *  @param orientation  The orientation of the RowSizer.
 *  @param container	The containing control.
 */
RowSizer::RowSizer(Orientation orientation, CWnd *container):
    orientation_(orientation),
	wnd_(container),
	id_(0),
    edgeGap_(DEFAULT_EDGER_GAP),
    childGap_(DEFAULT_CHILD_GAP)
{
}


/**
 *  This constructs a RowSizer.
 *
 *  @param orientation  The orientation of the RowSizer.
 *  @param parent		The parent of the containing control.
 *  @param id			The id of the containing control.
 */
RowSizer::RowSizer(Orientation orientation, CWnd *parent, uint32 containerID):
    orientation_(orientation),
	wnd_(parent),
	id_(containerID),
    edgeGap_(DEFAULT_EDGER_GAP),
    childGap_(DEFAULT_CHILD_GAP)
{
}


/**
 *  This is the RowSizer destructor.
 */
/*virtual*/ RowSizer::~RowSizer()
{
}


/**
 *  This gets the orientation of the sizer.
 *
 *  @returns            The orientation.
 */
RowSizer::Orientation RowSizer::orientation() const
{
    return orientation_;
}


/**
 *  This sets the orientation of the sizer.
 *
 *  @param orientation  The orientation of the RowSizer. 
 */
void RowSizer::orientation(Orientation orientation)
{
    orientation_ = orientation;
}


/**
 *  This adds a sub-sizer to the RowSizer.
 *
 *  @param child        The child Sizer.
 *  @param size         The size of the child.
 *  @param sizeUnits    The units for the size.  
 */
void RowSizer::addChild
(
    SizerPtr            child,
    uint32				size,
    SizeUnits           sizeUnits 
)
{
	BW_GUARD;

    Child childElem;
    childElem.sizer_        = child;
    childElem.size_         = size;
    childElem.sizeUnits_    = sizeUnits;
    children_.push_back(childElem);
}


/**
 *  This adds a sub-sizer to the RowSizer.
 *
 *  @param child        The child Sizer.
 */
void RowSizer::addChild(SizerPtr child)
{
	BW_GUARD;

	uint32 size = 0;
    CSize minSz = child->minimumSize();
    if (orientation_ == VERTICAL)
        size = minSz.cy;
    else if (orientation_ == HORIZONTAL)
        size = minSz.cx;

	addChild(child, size, PIXELS);
}


/**
 *  This gets the gap between the top/bottom (if vertically orientated) or the 
 *  left/right (if horizontally orientated) and the first sub-sizer.
 *
 *  @returns            The edge gap.
 */
uint32 RowSizer::edgeGap() const
{
    return edgeGap_;
}


/**
 *  This sets the gap between the top/bottom (if vertically orientated) or the 
 *  left/right (if horizontally orientated) and the first sub-sizer.
 *  
 *  @param gap          The new edge gap.
 */
void RowSizer::edgeGap(uint32 gap)
{
    edgeGap_ = gap;
}

/**
 *  This gets the intra-sub-sizer gap.
 * 
 *  @returns            The gap between the sub-sizers.
 */
uint32 RowSizer::childGap() const
{
    return childGap_;
}


/**
 *  This sets the intra-sub-sizer gap.
 * 
 *  @param g            The new gap between the sub-sizers.
 */
void RowSizer::childGap(uint32 g)
{
    childGap_ = g;
}


/**
 *  This is called to do the actual sizing.
 *
 *  @param extents  The extents allowed for the Sizer and it's 
 *                  children.
 */
/*virtual*/ void RowSizer::onSize(CRect const &e)
{
	BW_GUARD;

    Sizer::onSize(e);

	CWnd *wnd = window();
	if (wnd != NULL)
	{
		wnd->SetWindowPos
		(
			NULL,
			e.left   , e.top    , 
			e.Width(), e.Height(),
			SWP_NOZORDER
		);
	}

    if (children_.empty())
        return;

    CRect extents = e;
    extents.InflateRect(-(int)edgeGap_, -(int)edgeGap_);

    // Add up the sizes of the fixed elements and the sum of the weights:
    uint32 fixedSize  = 0;
    uint32 sumWeights = 0;
    for (size_t i = 0; i < children_.size(); ++i)
    {
        if (children_[i].sizeUnits_ == PIXELS)
        {
            fixedSize += children_[i].size_;
        }
        else if (children_[i].sizeUnits_ == WEIGHT)
        {
            sumWeights += children_[i].size_;
        }
    }

    uint32 weightSize = 0; // pixels left for weighted children
    if (orientation_ == HORIZONTAL)
        weightSize = extents.Width() - fixedSize - ((int)children_.size() - 1)*childGap_;
    else if (orientation_ == VERTICAL)
        weightSize = extents.Height() - fixedSize - ((int)children_.size() - 1)*childGap_;

    // Position the children:
    int cx = extents.left;
    int cy = extents.top;
    for (size_t i = 0; i < children_.size(); ++i)
    {
        CRect childExtents(cx, cy, cx, cy);

        // Calculate the position of the child:
        if (children_[i].sizeUnits_ == PIXELS)
        {
            if (orientation_ == HORIZONTAL)
            {
                childExtents.right   = cx + children_[i].size_;
                childExtents.bottom  = extents.bottom;
            }
            else if (orientation_ == VERTICAL)
            {
                childExtents.right  = extents.right;
                childExtents.bottom = cy + children_[i].size_;
            }
        }
        else if (children_[i].sizeUnits_ == WEIGHT)
        {
            if (orientation_ == HORIZONTAL)
            {
                childExtents.right   = cx + children_[i].size_*weightSize/sumWeights;
                childExtents.bottom  = extents.bottom;
            }
            else if (orientation_ == VERTICAL)
            {
                childExtents.right  = extents.right;
                childExtents.bottom = cy + children_[i].size_*weightSize/sumWeights;
            }
        }

        // Reposition the child:
        children_[i].sizer_->onSize(childExtents);

        // Adjust for the inter-sizer gap:
        if (orientation_ == HORIZONTAL)
            cx = childExtents.right + childGap_;
        else if (orientation_ == VERTICAL)
            cy = childExtents.bottom + childGap_;
    }
}


/**
 *  This calculates the minimum size for a RowSizer.
 *
 *  @returns            The minimum size for the RowSizer.
 */
/*virtual*/ CSize RowSizer::minimumSize() const
{
	BW_GUARD;

    if (children_.empty())
        return CSize(0, 0);

    uint32 cx = 0;
    uint32 cy = 0;

    // Add up the space taken by the children:
    for (size_t i = 0; i < children_.size(); ++i)
    {
        CSize childSize = children_[i].sizer_->minimumSize();
        if (orientation_ == HORIZONTAL)
        {
            cx += childSize.cx;
            cy  = std::max(cy, (uint32)childSize.cy);
        }
        else if (orientation_ == VERTICAL)
        {
            cx  = std::max(cx, (uint32)childSize.cx);
            cy += childSize.cy;
        }
    }

    // Add the intra-cub-sizer gaps:
    if (orientation_ == HORIZONTAL)
        cx += ((int)children_.size() - 1)*childGap_;
    else if (orientation_ == VERTICAL)
        cy += ((int)children_.size() - 1)*childGap_;

    // Add the edge gaps:
    cx += 2*edgeGap_;
    cy += 2*edgeGap_;

    return CSize(cx, cy);
}


/**
 *  This gets called to draw the RowSizer for debugging.
 *
 *  @param dc       The device context to draw into.
 */
/*virtual*/ void RowSizer::draw(CDC *dc)
{
	BW_GUARD;

    drawRect(dc, extents(), RGB(0, 0, 0));

    for (size_t i = 0; i < children_.size(); ++i)
    {
        children_[i].sizer_->draw(dc);
    }
}

/**
 *	This gets the container window.  This value should NOT be cached.
 *
 *  @returns		The containing window, NULL if there is none.
 */
CWnd *RowSizer::window()
{
	BW_GUARD;

    if (id_ != 0)
        return wnd_->GetDlgItem(id_);
    else
        return wnd_;
}

} // namespace controls
BW_END_NAMESPACE

