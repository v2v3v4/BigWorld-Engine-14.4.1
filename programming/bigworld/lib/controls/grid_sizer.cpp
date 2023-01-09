#include "pch.hpp"
#include "grid_sizer.hpp"
#include <algorithm>

BW_BEGIN_NAMESPACE
namespace controls
{

namespace
{
    const uint32 DEFAULT_EDGE_GAP     = 2;
    const uint32 DEFAULT_GAP          = 2;
}


/**
 *	This gives sensible values to the column/row sizing information used in a
 *	GridSizer.
 */
GridSizer::SizeInfo::SizeInfo():
    size_(1),
    units_(WEIGHT)
{
}


/**
 *	This constructs a GridSizer.
 *
 *  @param columns			The number of columns.
 *  @param rows				The number of rows.
 */
GridSizer::GridSizer(uint32 columns, uint32 rows):
    Sizer(),
	wnd_(NULL),
	id_(0),
    leftEdgeGap_(DEFAULT_EDGE_GAP),
	topEdgeGap_(DEFAULT_EDGE_GAP),
	rightEdgeGap_(DEFAULT_EDGE_GAP),
	bottomEdgeGap_(DEFAULT_EDGE_GAP),
    gap_(DEFAULT_GAP),
    columns_(0),
    rows_(0),
    children_(NULL)
{
	BW_GUARD;

	create(columns, rows);
}


/**
 *	This constructs a GridSizer that also sizes a control such as a group-box.
 *
 *  @param columns		The number of columns.
 *  @param rows			The number of rows.
 *  @param container	The containing control.
 */
GridSizer::GridSizer
(
	uint32				columns, 
	uint32				rows, 
	CWnd				*container
):
    Sizer(),
	wnd_(container),
	id_(0),
    leftEdgeGap_(DEFAULT_EDGE_GAP),
	topEdgeGap_(DEFAULT_EDGE_GAP),
	rightEdgeGap_(DEFAULT_EDGE_GAP),
	bottomEdgeGap_(DEFAULT_EDGE_GAP),
    gap_(DEFAULT_GAP),
    columns_(0),
    rows_(0),
    children_(NULL)
{
	BW_GUARD;

	create(columns, rows);
}


/**
 *	This constructs a GridSizer that also sizes a control such as a group-box.
 *
 *  @param columns			The number of columns.
 *  @param rows				The number of rows.
 *  @param parent			The parent of the control.
 *  @param containerID		The id of the sub-control of the parent.				
 */
GridSizer::GridSizer
(
	uint32				columns, 
	uint32				rows, 
	CWnd				*parent, 
	uint32				containerID
):
    Sizer(),
	wnd_(parent),
	id_(containerID),
    leftEdgeGap_(DEFAULT_EDGE_GAP),
	topEdgeGap_(DEFAULT_EDGE_GAP),
	rightEdgeGap_(DEFAULT_EDGE_GAP),
	bottomEdgeGap_(DEFAULT_EDGE_GAP),
    gap_(DEFAULT_GAP),
    columns_(0),
    rows_(0),
    children_(NULL)
{
	BW_GUARD;

	create(columns, rows);
}


/**
 *	This is the GridSizer destructor.
 */
/*virtual*/ GridSizer::~GridSizer()
{
	BW_GUARD;

	destroy();
}


/** 
 *	This sets a child control.
 *
 *  @param column		The column.
 *  @param row			The row.
 *  @param sizer		The child to place in the column, row.
 */
void GridSizer::setChild
(
    uint32				column,
    uint32				row,
    SizerPtr			sizer
)
{
	BW_GUARD;

	MF_ASSERT(column < columns_ && row < rows_);
    children_[row*columns_ + column] = sizer;
}


/**
 *	This sets how a column resizes.
 *
 *  @param col			The column whose sizing is being set.
 *  @param sz			The size of the column.
 *  @param units		The units of the size.
 */
void GridSizer::setColumnSize(uint32 col, uint32 sz, SizeUnits units)
{
	BW_GUARD;

	MF_ASSERT(col < columns_);
    colInfoSizes_[col].size_  = sz;
    colInfoSizes_[col].units_ = units;
}


/**
 *	This sets how a row resizes.
 *
 *  @param col			The row whose sizing is being set.
 *  @param sz			The size of the row.
 *  @param units		The units of the size.
 */
void GridSizer::setRowSize(uint32 row, uint32 sz, SizeUnits units)
{
	BW_GUARD;

	MF_ASSERT(row < rows_);
    rowInfoSizes_[row].size_  = sz;
    rowInfoSizes_[row].units_ = units;
}


/**
 *	This gets the number of columns wide.
 *
 *  @returns			The number of columns wide.
 */
uint32 GridSizer::columns() const
{
    return columns_;
}


/**
 *	This gets the number of rows high.
 *
 *  @returns			The number of rows high.
 */
uint32 GridSizer::rows() const
{
    return rows_;
}


/** 
 *	This gets the left edge gap.
 *
 *  @returns			The gap between the left and the grid.
 */
uint32 GridSizer::leftEdgeGap() const
{
	return leftEdgeGap_;
}


/** 
 *	This gets the top edge gap.
 *
 *  @returns			The gap between the top and the grid.
 */
uint32 GridSizer::topEdgeGap() const
{
	return topEdgeGap_;
}


/** 
 *	This gets the right edge gap.
 *
 *  @returns			The gap between the right and the grid.
 */
uint32 GridSizer::rightEdgeGap() const
{
	return rightEdgeGap_;
}


/** 
 *	This gets the bottom edge gap.
 *
 *  @returns			The gap between the bottom and the grid.
 */
uint32 GridSizer::bottomEdgeGap() const
{
	return bottomEdgeGap_;
}


/** 
 *	This sets the left edge gap.
 *
 *  @param gap			The gap between the left edge and the grid.
 */
void GridSizer::leftEdgeGap(uint32 gap)
{
	leftEdgeGap_ = gap;
}


/** 
 *	This sets the top edge gap.
 *
 *  @param gap			The gap between the top edge and the grid.
 */
void GridSizer::topEdgeGap(uint32 gap)
{
	topEdgeGap_ = gap;
}


/** 
 *	This sets the right edge gap.
 *
 *  @param gap			The gap between the right edge and the grid.
 */
void GridSizer::rightEdgeGap(uint32 gap)
{
	rightEdgeGap_ = gap;
}


/** 
 *	This sets the bottom edge gap.
 *
 *  @param gap			The gap between the bottom edge and the grid.
 */
void GridSizer::bottomEdgeGap(uint32 gap)
{
	bottomEdgeGap_ = gap;
}


/** 
 *	This gets the gap between columns and between rows.
 *
 *  @returns			The intra-column and intra-row gap.
 */
uint32 GridSizer::gap() const
{
    return gap_;
}


/** 
 *	This sets the gap between columns and between rows.
 *
 *  @param gap			The new intra-column and intra-row gap.
 */
void GridSizer::gap(uint32 g)
{
    gap_ = g;
}


/**
 *	This is called when the sizer should be resized to fit inside a rectangle.
 * 
 *  @param e			The rectangle to fit into.
 */
/*virtual*/ void GridSizer::onSize(CRect const &e)
{
	BW_GUARD;

    Sizer::onSize(e);

	// If there is a containing control, resize it:
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

    if (children_ == NULL)
        return;

    CRect extents = e;
    extents.left   += leftEdgeGap_;
	extents.top    += topEdgeGap_;
	extents.right  -= rightEdgeGap_;
	extents.bottom -= bottomEdgeGap_;

    // Add up the sizes of the fixed elements and the sum of the weights for
    // the columns:
    BW::vector<uint32> colSizes(columns_, 0);
    uint32 fixedXSize  = 0;
    uint32 sumXWeights = 0;
    for (size_t col = 0; col < columns_; ++col)
    {
        if (colInfoSizes_[col].units_ == PIXELS)
        {
            if (colInfoSizes_[col].size_ == 0)
            {
                uint32 minSize = 0;
                for (size_t row = 0; row < rows_; ++row)
                {
                    SizerPtr sizer = children_[row*columns_ + col];
                    if (sizer != NULL)
                    {
                        CSize sz = sizer->minimumSize();
                        minSize = std::max(minSize, (uint32)sz.cx);
                    }
                }
                fixedXSize += minSize;
                colSizes[col] = minSize;
            }
            else
            {
                fixedXSize += colInfoSizes_[col].size_;
                colSizes[col] = colInfoSizes_[col].size_;
            }
        }
        else if (colInfoSizes_[col].units_ == WEIGHT)
        {
            sumXWeights += colInfoSizes_[col].size_;
        }
    }

    // Add up the sizes of the fixed elements and the sum of the weights for
    // the rows:
    BW::vector<uint32> rowSizes(rows_);
    uint32 fixedYSize  = 0;
    uint32 sumYWeights = 0;
    for (size_t row = 0; row < rows_; ++row)
    {
        if (rowInfoSizes_[row].units_ == PIXELS)
        {
            if (rowInfoSizes_[row].size_ == 0)
            {
                uint32 minSize = 0;
                for (size_t col = 0; col < columns_; ++col)
                {
                    SizerPtr sizer = children_[row*columns_ + col];
                    if (sizer != NULL)
                    {
                        CSize sz = sizer->minimumSize();
                        minSize = std::max(minSize, (uint32)sz.cy);
                    }
                }
                fixedYSize += minSize;
                rowSizes[row] = minSize;
            }
            else
            {
                fixedYSize  += rowInfoSizes_[row].size_;
                rowSizes[row] = rowInfoSizes_[row].size_;
            }
        }
        else if (rowInfoSizes_[row].units_ == WEIGHT)
        {
            sumYWeights += rowInfoSizes_[row].size_;
        }
    }

	// Left over area for the weighted columns/rows:
    uint32 weightXSize = 
		extents.Width () - fixedXSize - ((int)columns_ - 1)*gap_;
    uint32 weightYSize = 
		extents.Height() - fixedYSize - ((int)rows_ - 1)*gap_;

    // Position the children:    
    int cy = extents.top;
    for (size_t row = 0; row < rows_; ++row)
    {
        int rowSize = 0;
        if (rowInfoSizes_[row].units_ == PIXELS)
            rowSize = rowSizes[row];
        else if (rowInfoSizes_[row].units_ == WEIGHT)
            rowSize = rowInfoSizes_[row].size_*weightYSize/sumYWeights;

        int cx = extents.left;

        for (size_t col = 0; col < columns_; ++col)
        {
            int colSize = 0;
            if (colInfoSizes_[col].units_ == PIXELS)
                colSize = colSizes[col];
            else if (colInfoSizes_[col].units_ == WEIGHT)
                colSize = colInfoSizes_[col].size_*weightXSize/sumXWeights;

            // Reposition the child:
            SizerPtr sizer = children_[row*columns_ + col];
            if (sizer != NULL)
            {
                CRect childExtents(cx, cy, cx + colSize, cy + rowSize);          
                sizer->onSize(childExtents);
            }

            cx = cx + colSize + gap_;
        }
        cy = cy + rowSize + gap_;
    }
}


/**
 *	This gets the minimum size that the sizer can occupy.
 *
 *  @returns				The minimum size that the sizer can occupy.
 */
/*virtual*/ CSize GridSizer::minimumSize() const
{
	BW_GUARD;

    if (children_ == NULL)
        return CSize(0, 0);

    // Minimum in the x-direction:
    uint32 cx = 0;
    for (uint32 col = 0; col < columns_; ++col)
    {
        uint32 minXThisCol = 0;
        for (uint32 row = 0; row < rows_; ++row)
        {
            SizerPtr sizer = children_[row*columns_ + col];
            if (sizer != NULL)
            {
                CSize thisSizerMinSz = sizer->minimumSize();
                minXThisCol = std::max((uint32)thisSizerMinSz.cx, minXThisCol);
            }
        }
        cx += minXThisCol;
    }

    // Minimum in the y-direction:
    uint32 cy = 0;
    for (uint32 row = 0; row < rows_; ++row)
    {
        uint32 minYThisRow = 0;
        for (uint32 col = 0; col < columns_; ++col)
        {
            SizerPtr sizer = children_[row*columns_ + col];
            if (sizer != NULL)
            {
                CSize thisSizerMinSz = sizer->minimumSize();
                minYThisRow = std::max((uint32)thisSizerMinSz.cy, minYThisRow);
            }
        }
        cy += minYThisRow;
    }

    // Add the intra-sizer gaps:
    cx += (columns_ - 1)*gap_;
    cy += (rows_    - 1)*gap_;

    // Add the edge gaps:
    cx += leftEdgeGap_ + rightEdgeGap_;
    cy += topEdgeGap_  + bottomEdgeGap_;

    return CSize(cx, cy);
}


/**
 *	This can be used to debug sizing.  It draws the extents of the grid 
 *	elements.
 *
 *  @param dc				The device context to draw the outlines to.
 */
/*virtual*/ void GridSizer::draw(CDC *dc)
{
	BW_GUARD;

    drawRect(dc, extents(), RGB(128, 0, 255));

    for (uint32 row = 0; row < rows_; ++row)
    {
        for (uint32 col = 0; col < columns_; ++col)
        {
            SizerPtr sizer = children_[row*columns_ + col];
            if (sizer != NULL)
                sizer->draw(dc);
        }
    }
}


/**
 *	This creates the appropriate structures to hold child elements.
 *
 *  @param columns		The number of columns wide.
 *  @param rows			The number of rows high.
 */
void GridSizer::create(uint32 columns, uint32 rows)
{
	BW_GUARD;

	destroy();
	if (columns != 0 && rows != 0)
	{
		columns_ = columns; 
		rows_    = rows;
		children_ = new SizerPtr[columns*rows];
		colInfoSizes_.resize(columns);
		rowInfoSizes_.resize(rows);
	}
}


/**
 *	This is cleans up memory used.
 */
void GridSizer::destroy()
{
	BW_GUARD;

    bw_safe_delete_array(children_);
	colInfoSizes_.clear();
	rowInfoSizes_.clear();
}


/**
 *	This gets the control (if any) that the sizer takes over.
 *
 *  @returns			The window that the sizer sizers.
 */
CWnd *GridSizer::window()
{
	BW_GUARD;

    if (id_ != 0)
        return wnd_->GetDlgItem(id_);
    else
        return wnd_;
}

} // namespace controls
BW_END_NAMESPACE
