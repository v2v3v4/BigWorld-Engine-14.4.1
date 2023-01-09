// SceneBrowserList.cpp : implementation file
//

#include "pch.hpp"
#include "scene_browser_list.hpp"
#include "scene_browser_utils.hpp"
#include "setup_items_task.hpp"
#include "group_item.hpp"
#include "single_property_editor.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/dogwatch.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "resource.h"

BW_BEGIN_NAMESPACE

namespace
{
	// Group by constants
	const int GROUP_COLUMN_IDX = 0;
	const int COLUMN_ORDER_GROUPBY = -1;

	// Graphical constants
	const int INTERNAL_COLUMN_GAP_X = 4;
	const int ROW_ALTERNATE_THRESHOLD = 128;
	const int ROW_ALTERNATE_COLOUR = 20;
	const int EXPAND_BTN_SIZE = 5;
	const int GROUP_START_PADDING = 8;
	const int GROUP_BTN_INDENT = 1;
	const int GROUP_BTN_BORDER_OFFSET = 2;
	const int GROUP_BTN_BORDER_GROW = 3;

	// Limit size of the itemTextOverflow_ map to make sure memory usage
	// doesn't skyrocket. Should be an estimate of the maximum number of
	// cells visible at any one time, plus some slack. Estimating 300
	// items x 100 columns = 30000.
	const size_t MAX_ITEM_TEXT_OVERFLOW_SIZE = 30000;

} // namespace anonymous



///////////////////////////////////////////////////////////////////////////////
// Section: SceneBrowserList
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor
 */
SceneBrowserList::SceneBrowserList() :
	firstTick_( true ),
	numItems_( 0 ),
	numTris_( 0 ),
	numPrimitives_( 0 ),
	ignoreSelMessages_( false ),
	sortColumnAdded_( false ),
	searchChanged_( false ),
	needsTick_( false ),
	lastMouseOverIdx_( -1 ),
	searchFilters_( columns_ )
{
	BW_GUARD;
}


/**
 *	Destructor
 */
SceneBrowserList::~SceneBrowserList()
{
	BW_GUARD;

	pSetupItemsTask_ = NULL;
}


/**
 *	This method reads in the list configuration, including column display
 *	information, and does other initialisation tasks for the list.
 *
 *	@param pDS	DataSection pointing to the section with the config data.
 *	@return		True if initialisation was successful, false otherwise.
 */
bool SceneBrowserList::init( DataSectionPtr pDS )
{
	BW_GUARD;

	if (!pDS)
	{
		ERROR_MSG(
			"SceneBrowser: cannot configure list, using defaults.\n" );
		return true; // Don't stop the tool because of this.
	}

	// Load default info for columns
	DataSectionPtr pColumnsDS = pDS->openSection( "DefaultColumnLayout" );
	if (!pColumnsDS)
	{
		ERROR_MSG(
			"SceneBrowser: cannot configure list columns, using defaults.\n" );
		return true; // Don't stop the tool because of this.
	}

	int hdrImgMargin = pColumnsDS->readInt( "headerImageMargin", 2 );

	columnStates_.initDefaultStates( pColumnsDS, headerImgList_ );
	this->GetHeaderCtrl()->SetImageList( &headerImgList_ );

	// "2" seems to be the Bitmap margin value that works best with bitmap
	// columns and the sorting arrow bitmap.
	GetHeaderCtrl()->SetBitmapMargin( hdrImgMargin );

	// Precalculate all checkbox bitmap combinations
	COLORREF dummy;
	COLORREF fgEven;
	COLORREF fgOdd;
	COLORREF fgSel;
	COLORREF bgEven;
	COLORREF bgOdd;
	COLORREF bgSel;

	drawItemColours( false, 0, fgEven, dummy, bgEven, dummy );
	drawItemColours( false, 1, fgOdd, dummy, bgOdd, dummy );
	drawItemColours( true, 0, fgSel, dummy, bgSel, dummy );

	checkboxHelper_.init( IDB_CHECK_ON, IDB_CHECK_OFF, fgSel, bgSel,
											fgEven, bgEven, fgOdd, bgOdd );

	selHelper_.init( this );

	// Create the tooltip control
	if (headerToolTips_.CreateEx( this, 0, WS_EX_TOPMOST ))
	{
		headerToolTips_.SetMaxTipWidth( SHRT_MAX );
		headerToolTips_.SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0,
									SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );
		headerToolTips_.Activate( TRUE );
	}

	return true;
}


/**
 *	This method updates the list from the ItemInfoDB, adding/removing items to
 *	the list, updating columns, etc.
 *
 *	@param forceTick	If true, it will force a new tick to be requested.
 *	@return		True if the list of items changed, false otherwise.
 */
bool SceneBrowserList::tick( bool forceTick )
{
	BW_GUARD;

	bool changed = false;

	if (ItemInfoDB::pInstance())
	{
		if (selHelper_.needsUpdate())
		{
			static DogWatch dw( "ListSelectionTick" );
			ScopedDogWatch sdw( dw );

			selHelper_.update( itemIndex_ );
			selHelper_.needsUpdate( false );
		}

		{
			static DogWatch dw( "ListItemSetupTick" );
			ScopedDogWatch sdw( dw );

			if (pSetupItemsTask_)
			{
				if (pSetupItemsTask_->finished())
				{
					finishSetupItemsThread();
					selHelper_.filter( false /* don't erase items */ );
					searchChanged_ = false;
					changed = true;
					needsTick_ = false;
				}
			}
			
			if (!pSetupItemsTask_ &&
				(ItemInfoDB::instance().hasChanged() ||
				firstTick_ || searchChanged_))
			{
				pSetupItemsTask_ = new SetupItemsBackgroundTask( search_, groupStates_,
							searchFilters_.allowedTypes(), currentComparer() );
				BgTaskManager::instance().addBackgroundTask( pSetupItemsTask_ );
			}

			if (forceTick)
			{
				needsTick_ = true;
			}
		}

		if (ItemInfoDB::instance().typesChanged() || firstTick_)
		{
			static DogWatch dw( "ListColumnsTick" );
			ScopedDogWatch sdw( dw );

			bool needsSort = false;
			// sync old columns into the state map
			columnStates_.syncColumnStates( columns_ );
			columns_.clear();
			ItemInfoDB::TypeUsage knownTypes;
			ItemInfoDB::instance().knownTypes( knownTypes );
			for (ItemInfoDB::TypeUsage::const_iterator it = knownTypes.begin();
				it != knownTypes.end(); ++it)
			{
				columns_.push_back(
								columnStates_.defaultColumn( (*it).first ) );

				if (!sortColumnAdded_ &&
					(*it).first == columnStates_.sortingType())
				{
					sortColumnAdded_ = true;
					needsSort = true;
				}
			}
			
			// set new columns' states and store new columns' states
			columnStates_.applyColumnStates( columns_ );	
			columnStates_.syncColumnStates( columns_ );

			updateColumns( needsSort );
		}

		ItemInfoDB::instance().clearChanged();
		firstTick_ = false;
	}

	if (itemTextOverflow_.size() > MAX_ITEM_TEXT_OVERFLOW_SIZE)
	{
		// Too many entries in the map. Regenerate it to save memory.
		redraw();
	}

	return changed;
}


/**
 *	This method is called to tell the list to configure itself.
 *
 *	@param pDS	DataSection containing list layout info.
 */
void SceneBrowserList::load( DataSectionPtr pDS )
{
	BW_GUARD;

	if (!pDS)
	{
		return;
	}

	DataSectionPtr pColsDS = pDS->openSection( "Columns" );
	if (pColsDS)
	{
		columnStates_.sorting( 0 );
		columnStates_.sortingType( ItemInfoDB::Type() );

		columnStates_.loadColumnStates( pColsDS );
		columnStates_.applyColumnStates( columns_ );
	}
}


/**
 *	This method is called to tell the list to save it's configuration.
 *
 *	@param pDS	DataSection to store list layout info.
 */
void SceneBrowserList::save( DataSectionPtr pDS )
{
	BW_GUARD;

	if (!pDS)
	{
		return;
	}

	DataSectionPtr pColsDS = pDS->newSection( "Columns" );

	columnStates_.syncColumnStates( columns_ );
	columnStates_.saveColumnStates( pColsDS );
}


/**
 *	This method is used to set the current group for grouping items.
 *
 *	@param pGroup	Group info to use for grouping.
 */
void SceneBrowserList::groupBy( ListGroup * pGroup )
{
	BW_GUARD;

	if (pGroup)
	{
		ListColumns::iterator it = columns_.begin();
		for (; it != columns_.end(); ++it)
		{
			if ((*it).type() == pGroup->second && !(*it).visible())
			{
				ERROR_MSG( "The Group By column is not visible.\n" );
				GetParent()->SendMessage( WM_LIST_GROUP_BY_CHANGED );
				return;
			}
		}
	}

	groupStates_.groupBy( pGroup );
	if (pGroup)
	{
		columnStates_.isGroupBySorting( true );
		columnStates_.sorting( -1 );
		columnStates_.sortingType( pGroup->second );
	}
	else
	{
		columnStates_.isGroupBySorting( false );
		columnStates_.sorting( 0 );
		columnStates_.sortingType( ItemInfoDB::Type() );
	}
	this->updateColumns( false /* will sort in updateSorting */ );
	this->updateSorting( 0, columnStates_.sorting(), true );
	if (columnStates_.sorting() == 0)
	{
		// Setup items manually if sorting == 0. If sorting != 0, the above 
		// updateSorting will handle it.
		CWaitCursor wait;
		this->setupItems();
	}
}


/**
 *	This method sets search text for filtering the list results.
 *
 *	@param newSearch	New search text to use for filtering results.
 */
void SceneBrowserList::search( const BW::string & newSearch )
{
	BW_GUARD;

	pSetupItemsTask_ = NULL;

	searchChanged_ = true;
	needsTick_ = true;
	search_ = newSearch;
	std::transform( search_.begin(), search_.end(), search_.begin(), tolower );
}


/**
 *	This method returns true if the list needs to be ticked as soon as
 *	possible, for example, after the search has changed.
 *
 *	@return		True if we need a tick ASAP.
 */
bool SceneBrowserList::needsTick() const
{
	BW_GUARD;

	return needsTick_;
}


/**
 *	This method scroll the list to make the item visible.
 *
 *	@param pItem	Item to show.
 */
void SceneBrowserList::scrollTo( ChunkItemPtr pItem )
{
	BW_GUARD;

	// Make sure the DB and the list are updated to the latest items first.
	if (ItemInfoDB::instance().hasChanged())
	{
		CWaitCursor wait;
		ItemInfoDB::instance().tick();
		setupItems();
	}

	// Check if its group is collapsed. If so, expand it.
	if (groupStates_.groupBy())
	{
		ItemInfoDB::ItemPtr pDbItem;

		ItemInfoDB::instance().lock();
		const ItemInfoDB::ChunkItemsMap & chunkItems =
						ItemInfoDB::instance().chunkItemsMapLocked();
		ItemInfoDB::ChunkItemsMap::const_iterator it =
												chunkItems.find( pItem.get() );

		if (it != chunkItems.end())
		{
			// Get group name from the item.
			pDbItem = (*it).second;
		}
		ItemInfoDB::instance().unlock();

		if (pDbItem)
		{
			BW::string group = pDbItem->propertyAsString(
											groupStates_.groupByType() );
			if (!groupStates_.groupExpanded( group ))
			{
				// Not expanded, so expand.
				CWaitCursor wait;
				groupStates_.expandCollapse( group, true );
				setupItems();
			}
		}
	}

	// We know the item must be in the itemIndex_ list, so find it there.
	bool itemFound = false;
	int itemIdx = 0;

	for (ItemIndex::iterator it = itemIndex_.begin(); it != itemIndex_.end();
		++it)
	{
		if ((*it)->chunkItem() == pItem)
		{
			itemFound = true;
			break;
		}
		itemIdx++;
	}

	if (itemFound)
	{
		EnsureVisible( itemIdx, FALSE );
		SetSelectionMark( itemIdx );
	}
	else
	{
		ERROR_MSG( "SceneBrowserList::scrollTo: "
					"Couldn't find DB item in the list.\n" );
	}
}


/**
 *	This method returns the currently visible items
 *
 *	@param chunkItems	Container where the items are returned.
 */
void SceneBrowserList::currentItems(
							BW::vector< ChunkItemPtr > & chunkItems ) const
{
	BW_GUARD;

	chunkItems.clear();
	chunkItems.reserve( itemIndex_.size() );
	for (ItemIndex::const_iterator it = itemIndex_.begin();
		it != itemIndex_.end(); ++it)
	{
		if ((*it)->chunkItem())
		{
			// add all non-group items (the ones that have an actual ChunkItem.
			chunkItems.push_back( (*it)->chunkItem() );
		}
	}
}


/**
 *	This method allows setting the selection in the Scene Browser list
 *
 *	@param selection	List of chunk items to select
 */
void SceneBrowserList::selection( const BW::vector<ChunkItemPtr> & selection )
{
	BW_GUARD;

	bool oldIgnore = ignoreSelMessages_;
	ignoreSelMessages_ = true;
	selHelper_.selection( selection, itemIndex_ );
	ignoreSelMessages_ = oldIgnore;
}


/**
 *	This method sets some extra windows styles for the list.
 */
void SceneBrowserList::PreSubclassWindow()
{
	BW_GUARD;

	CListCtrl::PreSubclassWindow();

	SetExtendedStyle( GetExtendedStyle() |
							LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP );

	GetToolTips()->Activate( FALSE );
	EnableToolTips( TRUE );
}


/**
 *	This method overrides MFC's DrawItem method to draw the list ourselves.
 *
 *	@param pDis		MFC draw info structure
 */
void SceneBrowserList::DrawItem( LPDRAWITEMSTRUCT pDis )
{
	BW_GUARD;

	// Get properties as text
	int idx = pDis->itemID;
	BW::vector<BW::wstring> text;
	size_t numCols = GetHeaderCtrl()->GetItemCount();
	for (size_t i = 0; i < numCols; ++i)
	{
		text.push_back( (LPCTSTR)GetItemText( idx, int(i) ) );
	}

	ItemInfoDB::ItemPtr pItem = this->getItem( idx );

	// Find out if the item is selected
	bool selected = (pDis->itemAction | ODA_SELECT) && 
					(pDis->itemState & ODS_SELECTED);

	bool hidden = false;
	bool frozen = false;
	if (pItem)
	{
		hidden = pItem->isHidden();
		frozen = pItem->isFrozen();
	}

	if (groupStates_.groupBy() &&
		groupStates_.isGroupStart( itemIndex_, idx ))
	{
		// don't let the group row to be selected.
		selected = false;
	}

	// Colour and area setup
	RECT rectItem = pDis->rcItem;

	COLORREF crText;
	COLORREF crGrayText;
	COLORREF crBackground;
	COLORREF crBackHilite;

	drawItemColours( selected, idx,
					crText, crGrayText, crBackground, crBackHilite );

	// Setup DC and begin draw, one column at a time
	CDC dc;
	dc.Attach( pDis->hDC );
	dc.FillSolidRect( &rectItem, crBackground );

	COLORREF crOldBkColor = dc.GetBkColor();
	COLORREF crOldTextColor = dc.GetTextColor();

	int colWidth = 0;
	rectItem.right = rectItem.left;
	for (size_t i = 0; i < numCols; ++i)
	{
		dc.SetBkColor( crBackground );
		dc.SetTextColor( (hidden || frozen) ? crGrayText : crText );

		rectItem.left += colWidth;
		colWidth = this->GetColumnWidth( int( i ) );
		rectItem.right += colWidth;

		CRect rectItemPadded = rectItem;
		rectItemPadded.DeflateRect( INTERNAL_COLUMN_GAP_X, 0 );

		BW::wstring drawText = text[i];
		if (groupStates_.groupBy() && i == GROUP_COLUMN_IDX)
		{
			// Draw elements and adjust rects if it's the group column.
			if (groupStates_.isGroupStart( itemIndex_, idx ))
			{
				bool expanded =
						groupStates_.groupExpanded( bw_wtoutf8( drawText ) );
				drawExpandCollapseBtn( expanded, dc, crText, crBackHilite,
															rectItemPadded );

				// Draw the (# items) text next to the group if it's collapsed.
				if (!expanded && pItem)
				{
					BW::string numItemsText;
					drawInfoNumItems( pItem, crText, crBackground, dc,
															numItemsText );

					drawNumItemsPostfix( dc, drawText, static_cast<int>(i), text, 
											numItemsText, rectItemPadded );
					dc.SetTextColor( crText );
				}
			}
			else
			{
				drawText = L"";

				// Draw the (# items) text below the group if it's expanded, at
				// the row of the item just below the group item.
				if (groupStates_.isGroupStart( itemIndex_, idx - 1 ))
				{
					ItemInfoDB::ItemPtr pItem = this->getItem( idx - 1 );
					if (pItem)
					{
						BW::string numItemsText;
						drawInfoNumItems( pItem, crText, crBackground, dc,
																numItemsText );
						drawText = bw_utf8tow( numItemsText );
					}
				}
			}

			// Final item rect is padded to the right.
			rectItemPadded.left = std::min(
								rectItemPadded.left + GROUP_START_PADDING,
								rectItemPadded.right );
		}

		// Do the actual drawing.
		drawItemColumn( dc, static_cast<int>(i), rectItemPadded, drawText, selected, idx );
	}

	// Restore DC before returning
	dc.SetTextColor( crOldTextColor );
	dc.SetBkColor( crOldBkColor );

	dc.Detach();
}


// Message map
BEGIN_MESSAGE_MAP( SceneBrowserList, CListCtrl )
	ON_NOTIFY_REFLECT( LVN_GETDISPINFO, OnGetDispInfo )
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT( LVN_COLUMNCLICK, OnColumnClick )
	ON_NOTIFY_REFLECT( LVN_ODSTATECHANGED, OnOdStateChanged )
	ON_NOTIFY_REFLECT( LVN_ITEMCHANGED, OnItemChanged )
	ON_NOTIFY( HDN_ENDDRAG, 0, OnHdrColumnReorderEnd )
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()


/**
 *	This method returns information about a list item's text for a column. This
 *	method needs to be implemented because our list is an MFC Virtual List.
 *
 *	@param pNMHDR		MFC notify structure
 *	@param LRESULT		MFC return value
 */
void SceneBrowserList::OnGetDispInfo( NMHDR* pNMHDR, LRESULT* pResult )
{
	BW_GUARD;

    LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
    LV_ITEM* pListItem = &(pDispInfo)->item;
	*pResult = 0;

	if (pListItem->mask & LVIF_TEXT &&
		pListItem->iSubItem < GetHeaderCtrl()->GetItemCount())
	{
		ItemInfoDB::ItemPtr pItem = this->getItem( pListItem->iItem );
		if (pItem)
		{
			// Get the item info.
			int colIdx = ctrlToCol( pListItem->iSubItem );
			BW::wstring text = bw_utf8tow(
					pItem->propertyAsString( columns_[colIdx].type() ) );
			wcsncpy( pListItem->pszText, text.c_str(),
											pListItem->cchTextMax );
			pListItem->pszText[ pListItem->cchTextMax - 1 ] = 0;
		}
	}
}


/**
 *	This method handles mouse-move events, such as highlighting a checkbox if
 *	the mouse is over it.
 *
 *	@param nFlags	MFC's event flags
 *	@param rect		Mouse position in client coordinates.
 */
void SceneBrowserList::OnMouseMove( UINT nFlags, CPoint point )
{
	BW_GUARD;

	CListCtrl::OnMouseMove( nFlags, point );

	// If over the header, return to keep processing the event
	CRect hdrRect;
	this->GetHeaderCtrl()->GetWindowRect( &hdrRect );
	ScreenToClient( &hdrRect );
	if (hdrRect.PtInRect( point ))
	{
		return;
	}

	//TODO: Fix this properly, something is stealing the focus.
	CWnd * curFocus = GetFocus();
	
	if (curFocus && curFocus != this && !curFocus->IsChild( this ))
	{
		// Steal the focus from the top parent window so we can scroll the list
		// with the mouse wheel.
		this->SetFocus();
	}

	// Track mouseover for the checkboxes.
	LVHITTESTINFO ht;
	ht.pt = point;
	int idx = this->SubItemHitTest( &ht );
	int ctrlCol = ht.iSubItem;

	int lastItemUpdated = lastMouseOverIdx_;
	if (lastMouseOverIdx_ != -1)
	{
		Update( lastMouseOverIdx_ );
		lastMouseOverIdx_ = -1;
	}

	if (idx >= 0 && ctrlCol >= 0)
	{
		// If it's a bool, update it to show the rectangle
		CRect cbRect;
		checkboxRect( idx, ctrlCol, cbRect );
		int col = ctrlToCol( ctrlCol );
		if (columns_[ col ].type().valueType().desc() == ValueTypeDesc::BOOL &&
			cbRect.PtInRect( point ))
		{
			if (lastItemUpdated != idx)
			{
				Update( idx );
			}
			lastMouseOverIdx_ = idx;
		}
	}
}


/**
 *	This method handles a click with the left mouse button on the column header
 *	and if possible, it changes the sorting for the clicked column.
 *
 *	@param pNMHDR		MFC notify structure
 *	@param LRESULT		MFC return value
 */
void SceneBrowserList::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult )
{
	BW_GUARD;

    LPNMLISTVIEW pInfo = (LPNMLISTVIEW)pNMHDR;
	*pResult = 0;

	if (!groupStates_.groupBy() || pInfo->iSubItem == GROUP_COLUMN_IDX)
	{
		int colIdx = ctrlToCol( pInfo->iSubItem );
		if (columns_[ colIdx ].type() != columnStates_.sortingType())
		{
			// reset sorting, changing sorting column.
			columnStates_.sorting( 0 );
			columnStates_.sortingType( columns_[ colIdx ].type() );
		}

		if (columnStates_.sorting() == 0)
		{
			columnStates_.sorting( -1 );
		}
		else if (columnStates_.sorting() == -1)
		{
			columnStates_.sorting( 1 );
		}
		else if (columnStates_.sorting() == 1)
		{
			columnStates_.sorting( groupStates_.groupBy() ? -1 : 0 );
		}

		updateSorting( colIdx, columnStates_.sorting(), true );
	}
}


/**
 *	This method handles item selection in batch, via shitf-click
 *
 *	@param pNMHDR		MFC notify structure
 *	@param LRESULT		MFC return value
 */
void SceneBrowserList::OnOdStateChanged( LPNMHDR pNMHDR, LRESULT * pResult )
{
	BW_GUARD;

	*pResult = 0;

	LPNMLVODSTATECHANGE state = (LPNMLVODSTATECHANGE)pNMHDR;

	bool oldSelected = (state->uOldState & LVIS_SELECTED) != 0;
	bool newSelected = (state->uNewState & LVIS_SELECTED) != 0;
	if (oldSelected != newSelected && !ignoreSelMessages_)
	{
		selHelper_.needsUpdate( true );
	}
}


/**
 *	This method handles item selection via simple clicking.
 *
 *	@param pNMHDR		MFC notify structure
 *	@param LRESULT		MFC return value
 */
void SceneBrowserList::OnItemChanged( LPNMHDR pNMHDR, LRESULT * pResult )
{
	BW_GUARD;

	*pResult = 0;

	LPNMLISTVIEW state = (LPNMLISTVIEW)pNMHDR;
	bool oldSelected = (state->uOldState & LVIS_SELECTED) != 0;
	bool newSelected = (state->uNewState & LVIS_SELECTED) != 0;
	if (oldSelected != newSelected && !ignoreSelMessages_)
	{
		if (state->iItem >= 0 &&
			groupStates_.isGroupStart( itemIndex_, state->iItem ))
		{
			// We don't allow selection of the group items, so if a group was
			// clicked, make sure this doesn't alter the current selection.
			selHelper_.needsUpdate( false );
			restoreSelection();
		}
		else
		{
			// Mormal case, mark for updating the selection from the list.
			selHelper_.needsUpdate( true );

			if (state->iItem >= 0 &&
				Options::getOptionInt( "scenebrowser/autozoom" ) == 1)
			{
				// AutoZoom
				ListItemZoom::zoomTo( getItem( state->iItem ) );
			}
		}
	}
}


/**
 *	This method handles reordering of columns via drag & drop.
 *
 *	@param pNMHDR		MFC notify structure
 *	@param LRESULT		MFC return value
 */
void SceneBrowserList::OnHdrColumnReorderEnd( NMHDR *pNMHDR, LRESULT * pResult )
{
	BW_GUARD;

	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 1;

	int idx = phdr->iItem;
	int newIdx = phdr->pitem->iOrder;

	if (groupStates_.groupBy() && idx == GROUP_COLUMN_IDX)
	{
		INFO_MSG( "Can't reorder the 'Group By' column.\n" );
		return;
	}

	if (groupStates_.groupBy() && newIdx == GROUP_COLUMN_IDX)
	{
		// can't reposition a column before the Group By column
		newIdx = 1;
	}

	if (idx < newIdx)
	{
		int start = ctrlToCol( idx );
		int end = ctrlToCol( newIdx );
		int newOrder = columns_[ end ].order();
		for (int i = end; i > start; --i)
		{
			columns_[ i ].order( columns_[ i - 1 ].order() );
		}
		columns_[ start ].order( newOrder );
		updateColumns( false );
	}
	else if (idx > newIdx)
	{
		int start = ctrlToCol( idx );
		int end = ctrlToCol( newIdx );
		int newOrder = columns_[ end ].order();
		for (int i = end; i < start; ++i)
		{
			columns_[ i ].order( columns_[ i + 1 ].order() );
		}
		columns_[ start ].order( newOrder );
		updateColumns( false );
	}

	redraw();
}


/**
 *	This method handles right-clicking on the column header bar.
 *
 *	@param wParam		MFC wParam parameter
 *	@param lParam		MFC lParam parameter (notify struct in this case).
 *	@param LRESULT		MFC return value
 *	@return		The result of the base class' OnNotify call.
 */
BOOL SceneBrowserList::OnNotify( WPARAM wParam, LPARAM lParam,
														LRESULT * pResult )
{
	BW_GUARD;

	LPNMHDR pNH = (LPNMHDR)lParam;
	*pResult = 0;

	if (pNH->code == NM_RCLICK &&
		pNH->hwndFrom == this->GetHeaderCtrl()->GetSafeHwnd())
	{
		// Right button was clicked on header, column visibility menu.
		// Track as many times as columns the user clicks
		CPoint pt;
		GetCursorPos( &pt );

		int result = -1;
		while (result != 0)
		{
			int groupByColumnIdx =
							groupStates_.groupBy() ? GROUP_COLUMN_IDX : -1;

			result = ListColumnPopup::doModal( this, pt, columnStates_,
												columns_, groupByColumnIdx );
		
			if (result == ListColumnPopup::UPDATE_COLUMNS)
			{
				// Just update the column layout.
				updateColumns( false );
			}
			else if (result == ListColumnPopup::UPDATE_ALL ||
					result == 0 /*also update all on exit*/)
			{
				// Calling groupBy takes care of everything
				ItemInfoDB::Type & groupByType = columnStates_.sortingType();
				if (groupByType.valueType().isValid())
				{
					if (columnStates_.isGroupBySorting())
					{
						ListGroup group( groupByType.name(), groupByType );
						groupBy( &group);
					}
					else
					{
						groupStates_.groupBy( NULL );
						updateColumns( true );
					}
				}
				else
				{
					groupBy( NULL );
				}
				GetParent()->SendMessage( WM_LIST_GROUP_BY_CHANGED );
			}

			redraw();
		}
	}
	else if(pNH->code == HDN_ITEMCHANGED &&
		pNH->hwndFrom == this->GetHeaderCtrl()->GetSafeHwnd())
	{
		updateHeaderColumnWidth( pNH );
	}
	
	return CListCtrl::OnNotify( wParam, lParam, pResult );
}

/**
 *	This method updated the width of the columns in columns_ vector, and
 *	recreate the tooltips for visible header columns
 *
 *	@param pNMHDR		MFC notify structure	
 */
void SceneBrowserList::updateHeaderColumnWidth( NMHDR *pNMHDR )
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	if (phdr == NULL)
		return;
	int idx = phdr->iItem;
	if (idx >= 0 && idx < this->GetHeaderCtrl()->GetItemCount())
	{
		int colIdx = ctrlToCol( idx );
		int oldWidth = columns_[ colIdx ].width();
		int width = this->GetColumnWidth( idx );
		if (oldWidth != width)
		{
			columns_[ colIdx ].width( width );
			updateColumnTooltips();
		}
	}
}

/**
 *	This method intercepts messages that can't be handled in other ways and 
 *	handles them.
 *
 *	@param pMsg		Windows message to process.
 */
BOOL SceneBrowserList::PreTranslateMessage( MSG * pMsg )
{
	BW_GUARD;

	headerToolTips_.RelayEvent( pMsg );

    if (pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_ESCAPE)
		{
			// Deselect all
			this->SetItemState( -1, 0, LVIS_SELECTED );
			selHelper_.update( itemIndex_ );
		}
		else if (pMsg->wParam == VK_DELETE)
		{
			// Delete selected items
			selHelper_.deleteSelectedItems();
		}
    }
	else if (pMsg->message == WM_LBUTTONDOWN)
	{
		if (onLeftMouseDown( pMsg->pt ))
		{
			return TRUE;
		}
	}

	return CListCtrl::PreTranslateMessage( pMsg );
}


/**
 *	This method displays returns tooltip text for an item's property if the
 *	mouse cursor hovers over it.
 *
 *	@param point	MFC's mouse cursor position.
 *	@param pTI		MFC's tooltip return struct.
 *	@return			Id of the control.
 */
INT_PTR SceneBrowserList::OnToolHitTest( CPoint point, TOOLINFO * pTI ) const
{
	BW_GUARD;

	SceneBrowserList * nonConstThis = const_cast< SceneBrowserList * >( this );

	LVHITTESTINFO ht;
	ht.pt = point;
	int idx = nonConstThis->SubItemHitTest( &ht );
	int ctrlCol = ht.iSubItem;

	INT_PTR ret = -1;
	if (idx > -1)
	{
		// We are hovering an item
		ItemTextOverflow::const_iterator itOverflow =
						itemTextOverflow_.find( ((uint64)idx << 32) + ctrlCol );

		if (itOverflow != itemTextOverflow_.end() && (*itOverflow).second)
		{
			// The text didn't fit in its rect, so show a tooltip
			pTI->hwnd = m_hWnd;

			int colIdx = nonConstThis->ctrlToCol( ctrlCol );

			BW::wstring colText;
			bw_utf8tow(
				getItem( idx )->propertyAsString( columns_[ colIdx ].type() ),
				colText );

			// This string will be deallocated by MFC, so must use raw malloc.
			size_t size = (colText.length() + 1) * sizeof( wchar_t );
			pTI->lpszText = (wchar_t *)raw_malloc( size );
			memcpy( pTI->lpszText, colText.c_str(), size );

			GetClientRect( &(pTI->rect) );

			// Generate a unique ID for each cell and each item. For this we
			// assume the user won't have more than 512 columns visible at any
			// one time, which allows for 8 million+ indexes.
			pTI->uId = (UINT) (idx * 512 + ctrlCol);

			ret = pTI->uId;
		}
	}

	return ret;
}


/**
 *	This method handles right-clicking on the list box.
 *
 *	@param nFlags		MFC's event flags
 *	@param point		Point in client area coordinates.
 */
void SceneBrowserList::OnRButtonDown( UINT nFlags, CPoint point )
{
	BW_GUARD;

	// Right button was clicked on the list.
	bool isGroup = false;
	int itemIdx = this->HitTest( point );
	ItemInfoDB::ItemPtr pItem;
	if (itemIdx >= 0)
	{
		isGroup = groupStates_.isGroupStart( itemIndex_, itemIdx );
		pItem = getItem( itemIdx );
	}

	int result = ListItemPopup::doModal(
								this, isGroup, pItem, selHelper_, itemIndex_ );

	if (result == ListItemPopup::COLLAPSE_ALL ||
		result == ListItemPopup::EXPAND_ALL)
	{
		CWaitCursor wait;
		groupStates_.expandCollapseAll( itemIndex_,
									(result == ListItemPopup::EXPAND_ALL) );
		setupItems();
	}
}


/**
 *	This method handles double-clicking on the list box.
 *
 *	@param nFlags		MFC's event flags
 *	@param point		Point in client area coordinates.
 */
void SceneBrowserList::OnLButtonDblClk( UINT nFlags, CPoint point )
{
	BW_GUARD;

	int itemIdx = this->HitTest( point );
	bool isGroup = false;
	if (itemIdx >= 0)
	{
		ItemInfoDB::ItemPtr pItem = getItem( itemIdx );
		if (groupStates_.isGroupStart( itemIndex_, itemIdx ))
		{
			// For groups, toggle expand/collapse
			if (pItem)
			{
				CWaitCursor wait;
				BW::string group = pItem->propertyAsString(
												GroupItem::groupNameType() );
				groupStates_.handleGroupClick( itemIndex_, group );
				setupItems();
			}
		}
		else
		{
			// For normal items, zoom to it
			ListItemZoom::zoomTo( pItem );
		}
	}
}


/**
 *	This method handles left-click events on the list.
 *
 *	@param mouseDownPoint	Mouse coords at the time of the event.
 *	@return		True if the message was handled, false otherwise.
 */
bool SceneBrowserList::onLeftMouseDown( const CPoint & mouseDownPoint )
{
	BW_GUARD;

	CPoint pt( mouseDownPoint ); 
	ScreenToClient( &pt );

	// If clicked the header, return to keep processing the event
	CRect hdrRect;
	this->GetHeaderCtrl()->GetWindowRect( &hdrRect );
	ScreenToClient( &hdrRect );
	if (hdrRect.PtInRect( pt ))
	{
		return false;
	}

	LVHITTESTINFO ht;
	ht.pt = pt;
	int idx = this->SubItemHitTest( &ht );
	int ctrlCol = ht.iSubItem;

	if (groupStates_.isGroupStart( itemIndex_, idx ))
	{
		if (groupStates_.groupBy())
		{
			// Clicked a group item row, find out where.
			// Reduce rect to the actual "+" sign that expands/collapses.
			CRect rectItem;
			this->GetItemRect( idx, rectItem, LVIR_BOUNDS );
			rectItem.DeflateRect( INTERNAL_COLUMN_GAP_X, 0 );
			rectItem.right = rectItem.left + EXPAND_BTN_SIZE + 2;
			rectItem.left = rectItem.left - 3;

			if (rectItem.PtInRect( pt ))
			{
				// Clicked the expand/collapse button area
				ItemInfoDB::ItemPtr pItem = this->getItem( idx );
				if (pItem)
				{
					CWaitCursor wait;
					BW::string group = pItem->propertyAsString(
											groupStates_.groupByType() );
					groupStates_.handleGroupClick( itemIndex_, group );
					setupItems();
				}
			}
			return true;
		}
	}
	else
	{
		// Normal asset list item, find out which column was clicked.
		if (ctrlCol >= 0)
		{
			// Found the column clicked. If it's a bool column, toogle it.
			ItemInfoDB::ItemPtr pItem = this->getItem( idx );
			if (pItem)
			{
				return handleItemClick( pItem, idx, ctrlCol, pt );
			}
		}
	}

	return false;
}


/**
 *	This method recreates the columns in the MFC CHeaderCtrl for the list from
 *	the internal columns_ vector.
 *
 *	@param reSort	True if it should sort the items afterwards.
 */
void SceneBrowserList::updateColumns( bool reSort )
{
	BW_GUARD;

	// Delete all of the columns.
	while (this->GetHeaderCtrl()->GetItemCount() > 0)
	{
		this->DeleteColumn( 0 );
	}

	// Fiddle with ordering to put the "Group By" column first always.
	for (ListColumns::iterator it = columns_.begin(); it != columns_.end();
		++it)
	{
		if ((*it).order() == COLUMN_ORDER_GROUPBY)
		{
			// Restore order in column(s) that were used for GroupBy.
			(*it).order( columnStates_.defaultColumn( (*it).type() ).order() );
		}
		if (groupStates_.groupByType() == (*it).type())
		{
			// Set the order of the Group By column to its special value.
			(*it).order( COLUMN_ORDER_GROUPBY );
		}
	}

	// Sort columns and put them into the header control
	std::sort( columns_.begin(), columns_.end() );
	int ctrlIdx = 0;
	for (size_t i = 0; i < columns_.size(); ++i)
	{
		if (columns_[i].visible())
		{
			LVCOLUMN colInfo;
			colInfo.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
			if (columns_[i].imageIdx() > -1)
			{
				colInfo.mask |= LVCF_IMAGE;
				colInfo.iImage = columns_[i].imageIdx();
			}
			colInfo.fmt = LVCFMT_LEFT;
			colInfo.cx = columns_[i].width();
			colInfo.iSubItem = ctrlIdx;
			BW::wstring colStr = bw_utf8tow( columns_[i].name() );
			colInfo.pszText = (wchar_t *)colStr.c_str();

			this->InsertColumn( ctrlIdx++, &colInfo );

			if (columnStates_.sorting() != 0 &&
				columns_[ i ].type() == columnStates_.sortingType())
			{
				// We were sorting by this column, so setup sorting again.
				updateSorting( static_cast<int>(i), columnStates_.sorting(), reSort );
			}
		}
	}

	searchFilters_.updateAllowedTypes();
	updateColumnTooltips();
}


/**
 *	This method recreates the tooltips for the column headers.
 */
void SceneBrowserList::updateColumnTooltips()
{
	BW_GUARD;

	int ctrlIdx = 1;
	CRect rect;

	GetHeaderCtrl()->GetClientRect( rect );

	rect.left = 0;
	rect.right = 0;

	for (size_t i = 0; i < columns_.size(); ++i)
	{
		headerToolTips_.DelTool(GetHeaderCtrl(), ctrlIdx);
		if (columns_[i].visible())
		{
			rect.right += columns_[i].width();

			headerToolTips_.AddTool( GetHeaderCtrl(),
				bw_utf8tow( columns_[i].name() ).c_str(), rect, ctrlIdx );

			rect.left += columns_[i].width();
		}
		ctrlIdx++;
	}
}


/**
 *	This method updates a column's appearance to reflect the sorting method,
 *	and if desired also sorts the list items based on the a column and sorting
 *	mechanism specified.
 *
 *	@param colIdx	Index of the column in the internal columns_ vector.
 *	@param sorting	-1 for ascending, +1 for descending, 0 for no sorting.
 *	@param reSort	True to sort the list's items, false to not sort.
 */
void SceneBrowserList::updateSorting( int colIdx, int sorting, bool reSort )
{
	BW_GUARD;

	if (colIdx >= 0 && colIdx < (int)columns_.size())
	{
		int newSort = 0;

		if (sorting < 0)
		{
			newSort = HDF_SORTDOWN;
		}
		else if (sorting > 0)
		{	
			newSort = HDF_SORTUP;
		}

		for (int i = 0; i < this->GetHeaderCtrl()->GetItemCount(); ++i)
		{
			HDITEM curCol;
			curCol.mask = HDI_FORMAT;
			MF_ASSERT( this->GetHeaderCtrl()->GetItem( i, &curCol ) );
			curCol.mask = HDI_FORMAT;
			curCol.fmt = curCol.fmt & ~HDF_SORTDOWN & ~HDF_SORTUP;
			MF_ASSERT( this->GetHeaderCtrl()->SetItem( i, &curCol ) );
		}

		if (newSort != 0)
		{
			CWaitCursor wait;

			int ctrlIdx = colToCtrl( colIdx );
			if (ctrlIdx != -1)
			{
				HDITEM curCol;
				curCol.mask = HDI_FORMAT;
				MF_ASSERT( this->GetHeaderCtrl()->GetItem( ctrlIdx, &curCol ) );
				curCol.mask = HDI_FORMAT;
				curCol.fmt = curCol.fmt | newSort;
				MF_ASSERT( this->GetHeaderCtrl()->SetItem( ctrlIdx, &curCol ) );
			}

			if (reSort)
			{
				setupItems();
			}
			
			redraw();
		}
	}
}


/**
 *	This method converts column index from the list's CHeaderCtrl column index
 *	to the internal columns_ vector index. Indices may differ between these two
 *	when, for example, some columns in columns_ are invisible.
 *
 *	@param ctrlIdx				Column index in the CHeaderCtrl.
 *	@param assertOnOutOfBounds	Assert if out of bounds.
 *	@return		Column index in the columns_ vector.
 */
int SceneBrowserList::ctrlToCol( int ctrlIdx, bool assertOnOutOfBounds )
{
	BW_GUARD;

	MF_ASSERT( ctrlIdx >= 0 && ctrlIdx < GetHeaderCtrl()->GetItemCount() );
	int colIdx = ctrlIdx;
	for (int i = 0; i <= colIdx; ++i)
	{
		if (!columns_[ i ].visible())
		{
			colIdx++;
		}
	}

	if (assertOnOutOfBounds)
	{
		MF_ASSERT( colIdx >= 0 && colIdx < (int)columns_.size() );
	}
	else if (colIdx < 0 || colIdx >= (int)columns_.size())
	{
		colIdx = -1;
	}
	return colIdx;
}


/**
 *	This method converts column index from the internal columns_ vector column
 *	index to the list's CHeaderCtrl index. Indices may differ between these two
 *	when, for example, some columns in columns_ are invisible.
 *
 *	@param colIdx				Column index in the columns_ vector.
 *	@param assertOnOutOfBounds	Assert if out of bounds.
 *	@return		Column index in the CHeaderCtrl.
 */
int SceneBrowserList::colToCtrl( int colIdx, bool assertOnOutOfBounds )
{
	BW_GUARD;

	MF_ASSERT( colIdx >= 0 && colIdx < (int)columns_.size() );

	if (!columns_[ colIdx ].visible())
	{
		return -1;
	}

	int ctrlIdx = colIdx;
	for (int i = 0; i < colIdx; ++i)
	{
		if (!columns_[ i ].visible())
		{
			ctrlIdx--;
		}
	}

	if (assertOnOutOfBounds)
	{
		MF_ASSERT( ctrlIdx >= 0 && ctrlIdx < GetHeaderCtrl()->GetItemCount() );
	}
	else if (ctrlIdx < 0 || ctrlIdx >= GetHeaderCtrl()->GetItemCount())
	{
		ctrlIdx = -1;
	}
	return ctrlIdx;
}


/**
 *	This method populates the internal itemIndex_ vector, which is used to
 *	display the items in the ItemInfoDB in the list, to apply grouping, search
 *	etc, to the raw list of items in the ItemInfoDB class.
 */
void SceneBrowserList::setupItems()
{
	BW_GUARD;

	// Stop any previous thread because we'll calculate it here.
	pSetupItemsTask_ = NULL;

	// Setup items
	SetupItemsTask setupTask( search_, groupStates_,
							searchFilters_.allowedTypes(), currentComparer() );

	// Search has been updated here as well, so clear the flag.
	searchChanged_ = false;

	setupTask.execute();
	setupTask.results( itemIndex_, numItems_, numTris_, numPrimitives_ );

	finishSetupItems();
}


/**
 *	This method returns the list item positioned at the desired index.
 *
 *	@param index	Desired item's index in the list.
 *	@return			Item at the index row, or NULL of index is out of bounds.
 */
ItemInfoDB::ItemPtr SceneBrowserList::getItem( int index ) const
{
	BW_GUARD;

	if (index < 0 || index >= (int)itemIndex_.size())
	{
		return NULL;
	}

	return itemIndex_[ index ];
}


/**
 *	This method calculates info for displaying the # of items of a group
 *
 *	@param pItem	Group item
 *	@param textCol	List's text colour
 *	@param bgCol	List's background colour
 *	@param dc		MFC's device context
 *	@param retText	Return param that contains the "# of items" string
 */
void SceneBrowserList::drawInfoNumItems( const ItemInfoDB::ItemPtr & pItem,
			COLORREF textCol, COLORREF bgCol, CDC & dc, BW::string & retText )
{
	BW_GUARD;

	COLORREF crNumItems = RGB(
							(GetRValue( textCol ) + GetRValue( bgCol )) / 2,
							(GetGValue( textCol ) + GetGValue( bgCol )) / 2,
							(GetBValue( textCol ) + GetBValue( bgCol )) / 2 );
	dc.SetTextColor( crNumItems );

	retText = pItem->propertyAsString( GroupItem::groupNumItemsType() );
}


/**
 *	This method calculates and draws the # items string after the group string.
 *
 *	@param dc		MFC's device context.
 *	@param mainText	The normal text for the item's group column.
 *	@param ctrlCol	Header control column index.
 *	@param colText	String vector with the text of all item's visible columns.
 *	@param numItemsText	Text containing the "# items" string.
 *	@param retRect	Rectangle where to fit the item's column
 */
void SceneBrowserList::drawNumItemsPostfix( CDC & dc,
					const BW::wstring & mainText, int ctrlCol,
					const BW::vector<BW::wstring> & colText,
					const BW::string & numItemsText, const RECT & itemRect )
{
	BW_GUARD;

	// Calc main text size
	RECT mainTextRect = itemRect;
	dc.DrawText( mainText.c_str(), int( mainText.length() ), &mainTextRect,
		DT_LEFT | DT_SINGLELINE | DT_WORD_ELLIPSIS | DT_CALCRECT );

	RECT numItemsRect = mainTextRect;
	numItemsRect.left = mainTextRect.right;
	numItemsRect.right = itemRect.right;

	// Steal area from other columns if we can for the "# items" text.
	size_t numCols = GetHeaderCtrl()->GetItemCount();
	size_t nextCol = ctrlCol + 1;
	while (nextCol < numCols && colText[ nextCol ].empty())
	{
		numItemsRect.right += this->GetColumnWidth( int( nextCol ) );
		nextCol++;
	}

	// Find the starting point for the "# items" text
	const ValueType & colType = columns_[ ctrlToCol( ctrlCol ) ].type().valueType();

	if (mainTextRect.right > itemRect.right || colType.isNumeric())
	{
		numItemsRect.left = itemRect.right;
	}
	else
	{
		numItemsRect.left = mainTextRect.right + GROUP_START_PADDING;
	}

	// Stick the "# items" string after the text's rect
	if (numItemsRect.left < numItemsRect.right)
	{
		BW::wstring wnumItemsText = bw_utf8tow( "  " + numItemsText );
		dc.DrawText( wnumItemsText.c_str(), int( wnumItemsText.length() ),
				&numItemsRect, DT_LEFT | DT_SINGLELINE | DT_WORD_ELLIPSIS );
	}
}


/**
 *	This method draws a [+] or [-] button for a group
 *
 *	@param expanded Whether the group is expanded
 *	@param dc		MFC's device context
 *	@param textCol	List's text colour
 *	@param bgCol	List's background colour
 *	@param rect		Rectangle in which to fit the button.
 */
void SceneBrowserList::drawExpandCollapseBtn( bool expanded, CDC & dc,
						COLORREF textCol, COLORREF bgCol, const CRect & rect )
{
	BW_GUARD;

	// Setup colours and coords for the [+] collapse button.
	CPen expandBtn( PS_SOLID, 1, textCol );
	CPen* oldPen = dc.SelectObject( &expandBtn );

	int hleft = rect.left - GROUP_BTN_INDENT;
	int hcenter = hleft + EXPAND_BTN_SIZE / 2;

	int vcenter = rect.top + rect.Size().cy / 2;
	int vtop = vcenter - EXPAND_BTN_SIZE / 2;

	CRect rcExpandBtn( hleft, vtop,
						hleft + EXPAND_BTN_SIZE, vtop + EXPAND_BTN_SIZE );

	// Draw a [+] if it's collapsed, or a [-] if it's expanded
	dc.MoveTo( rcExpandBtn.left, vcenter );
	dc.LineTo( rcExpandBtn.right, vcenter );

	if (!expanded)
	{
		dc.MoveTo( hcenter, rcExpandBtn.top );
		dc.LineTo( hcenter, rcExpandBtn.bottom );
	}
	
	// Draw the border in [+] or [-]
	CPen expandBtnBorder( PS_SOLID, 1, bgCol );
	dc.SelectObject( &expandBtnBorder );

	rcExpandBtn.OffsetRect(
		-GROUP_BTN_BORDER_OFFSET, -GROUP_BTN_BORDER_OFFSET );
	rcExpandBtn.right += GROUP_BTN_BORDER_GROW;
	rcExpandBtn.bottom += GROUP_BTN_BORDER_GROW;
	dc.MoveTo( rcExpandBtn.left, rcExpandBtn.top );
	dc.LineTo( rcExpandBtn.right, rcExpandBtn.top );
	dc.LineTo( rcExpandBtn.right, rcExpandBtn.bottom );
	dc.LineTo( rcExpandBtn.left, rcExpandBtn.bottom );
	dc.LineTo( rcExpandBtn.left, rcExpandBtn.top );

	// Restore DC
	dc.SelectObject( oldPen );
}


/**
 *	This method calculates the colours for an item
 *
 *	@param selected		Whether the item is selected
 *	@param idx			Item index.
 *	@param textCol		Returns the text colour
 *	@param grayTextCol	Returns the text colour
 *	@param bgCol		Returns the background colour
 *	@param bgHiCol		Returns the background highlight colour
 */
void SceneBrowserList::drawItemColours( bool selected, int idx,
					COLORREF & textCol, COLORREF & grayTextCol,
					COLORREF & bgCol, COLORREF & bgHiCol )
{
	BW_GUARD;

	int nCrBackground, nCrText;
	if (selected)
	{
		nCrBackground = COLOR_HIGHLIGHT;
		nCrText = COLOR_HIGHLIGHTTEXT;
	}
	else
	{
		nCrBackground = COLOR_WINDOW;
		nCrText = COLOR_WINDOWTEXT;
	}

	textCol = ::GetSysColor( nCrText );
	bgCol = ::GetSysColor( nCrBackground );

	if (!selected)
	{
		int bgR = GetRValue( bgCol );
		int bgG = GetGValue( bgCol );
		int bgB = GetBValue( bgCol );
		int colOff = ROW_ALTERNATE_COLOUR;
		int colThresh = ROW_ALTERNATE_THRESHOLD;
		bgHiCol =
			(bgR > colThresh && bgG > colThresh && bgB > colThresh) ?
				RGB( bgR - colOff, bgG - colOff, bgB - colOff ) :
				RGB( bgR + colOff, bgG + colOff, bgB + colOff );
	}
	else
	{
		bgHiCol = textCol;
	}

	if (!selected && (idx % 2) != 0)
	{
		std::swap( bgCol, bgHiCol );
	}

	grayTextCol = RGB(
					(GetRValue( textCol ) + GetRValue( bgCol )) / 2,
					(GetGValue( textCol ) + GetGValue( bgCol )) / 2,
					(GetBValue( textCol ) + GetBValue( bgCol )) / 2 );
}


/**
 *	This method draws an item's column (property) text, depending on the type.
 *
 *	@param dc		MFC's device context
 *	@param rect		Rectangle in which to draw the text.
 *	@param ctrlCol	List header control's column index.
 *	@param text		Text corresponding to the items's column type.
 *	@param selected Whether the item is selected
 *	@param idx		Item index.
 */
void SceneBrowserList::drawItemColumn( CDC & dc, int ctrlCol, CRect & rect,
						  const BW::wstring & text, bool selected, int idx )
{
	BW_GUARD;

	if (!text.empty())
	{
		const ValueType & colType =
						columns_[ ctrlToCol( ctrlCol ) ].type().valueType();

		if (colType.desc() == ValueTypeDesc::BOOL)
		{
			// Draw a custom checkbox
			BW::string boolStr;
			bw_wtoutf8( text, boolStr );

			bool checked;
			colType.fromString( boolStr, checked );

			bool odd = (idx % 2) != 0;

			CPoint pt;
			::GetCursorPos( &pt );
			ScreenToClient( &pt );

			checkboxHelper_.draw( dc, rect, pt, selected, odd, checked );
		}
		else
		{
			// Default case, just display the text.
			DWORD alignment = colType.isNumeric() ? DT_RIGHT : DT_LEFT;
			dc.DrawText( text.c_str(), int( text.length() ), &rect,
								alignment | DT_SINGLELINE | DT_WORD_ELLIPSIS );

			// Store whether the text fits in the items column rect for later
			// use when determining if a tooltip should be displayed.
			CRect calcRect = rect;
			dc.DrawText( text.c_str(), int( text.length() ), &calcRect,
								alignment | DT_SINGLELINE | DT_CALCRECT );
			itemTextOverflow_[ ((uint64) idx << 32) + ctrlCol ] =
												calcRect.right > rect.right;
		}
	}
}


/**
 *	This method calculates the rectangle of a checkbox.
 *
 *	@param idx		Item index
 *	@param ctrlIdx	Column index (from the control, subitem index)
 *	@param retRect	Returns a the checkbox rect that fits into the subitem's
 *					rect.
 */
void SceneBrowserList::checkboxRect( int idx, int ctrlIdx,
														CRect & retRect )
{
	BW_GUARD;

	this->GetSubItemRect( idx, ctrlIdx, LVIR_BOUNDS, retRect );
	checkboxHelper_.rect( retRect );
}


/**
 *	This helper method restores the selection in the list control from the
 *	internal vector.
 */
void SceneBrowserList::restoreSelection()
{
	BW_GUARD;

	bool oldIgnore = ignoreSelMessages_;
	ignoreSelMessages_ = true;
	selHelper_.restore( itemIndex_ );
	ignoreSelMessages_ = oldIgnore;
}


/**
 *	This method handles click in an item's column.
 *
 *	@param pItem	Item clicked.
 *	@param idx		Index of the item in the list.
 *	@param ctrlCol	Header control column index.
 *	@param pt		Point where the click occured.
 *	@return		True if the message was processed, false otherwise.
 */
bool SceneBrowserList::handleItemClick( ItemInfoDB::ItemPtr pItem,
									int idx, int ctrlCol, const CPoint & pt  )
{
	BW_GUARD;

	CRect cbRect;
	checkboxRect( idx, ctrlCol, cbRect );
	int colIdx = ctrlToCol( ctrlCol );
	if (columns_[ colIdx ].type().valueType().desc() == ValueTypeDesc::BOOL &&
		cbRect.PtInRect( pt ))
	{
		const ItemInfoDB::Type & type = columns_[ colIdx ].type();
		
		bool curVal;
		type.valueType().fromString( pItem->propertyAsString( type ), curVal );
		
		BW::string newVal;
		type.valueType().toString( !curVal, newVal );

		const ItemList & selection = selHelper_.selection( false );
		ItemList::const_iterator it =
					std::find( selection.begin(), selection.end(), pItem );
		
		if (it == selection.end())
		{
			// Item not in selection, toggle the bool only for this one.
			BW::vector< ChunkItemPtr > undoItems;
			undoItems.push_back( pItem->chunkItem() );

			SinglePropertyEditor * pEditor = new SinglePropertyEditor(
											pItem->chunkItem(), type, newVal );

			pItem->chunkItem()->edEdit( *pEditor );

			Py_DECREF( pEditor );
		}
		else
		{
			// Item in selection, toggle the bool for all selection.
			BW::vector< ChunkItemPtr > undoItems;
			for (ItemList::const_iterator it = selection.begin();
				it != selection.end(); ++it)
			{
				undoItems.push_back( (*it)->chunkItem() );
			}

			for (ItemList::const_iterator it = selection.begin();
				it != selection.end(); ++it)
			{
				SinglePropertyEditor * pEditor = new SinglePropertyEditor(
											(*it)->chunkItem(), type, newVal );

				(*it)->chunkItem()->edEdit( *pEditor );

				Py_DECREF( pEditor );
			}
		}

		UndoRedo::instance().barrier(
			LocaliseUTF8( L"SCENEBROWSER/BOOL_COLUMN_BARRIER",
							columns_[ colIdx ].name().c_str() ), false );

		// Update the properties panel
		bool propsPanelUpdateOK = false;
		PyObject * pModule = PyImport_ImportModule( "WorldEditorDirector" );
		if (pModule != NULL)
		{
			PyObject * pScriptObject = PyObject_GetAttrString( pModule, "bd" );

			if (pScriptObject != NULL)
			{
				PyObject * pFunction = PyObject_GetAttrString( pScriptObject, "resetSelUpdate" );
				if (pFunction)
				{
					propsPanelUpdateOK = Script::call( pFunction, PyTuple_New(0),
														"SceneBrowserList::handleItemClick" );
				}
				Py_DECREF( pScriptObject );
			}
			Py_DECREF( pModule );
		}

		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		
		if (!propsPanelUpdateOK)
		{
			ERROR_MSG( "Failed to update the properties panel after clicking in the Scene Browser.\n" );
		}

		needsTick_ = true; // force a tick as soon as possible.
		return true;
	}
	return false;
}


/**
 *	This helper method updates the control with the item count.
 */
void SceneBrowserList::finishSetupItems()
{
	BW_GUARD;

	// Ignore selection messages, we are about to regenerate the selection.
	bool oldIgnore = ignoreSelMessages_;
	ignoreSelMessages_ = true;

	// deselect all
	this->SetItemState( -1, 0, LVIS_SELECTED );

	this->SetItemCountEx( (int)itemIndex_.size(), LVSICF_NOSCROLL );

	// Restore selection
	restoreSelection();

	ignoreSelMessages_ = oldIgnore;
}


/**
 *	This method finishes seting up the items done in the thread.
 */
void SceneBrowserList::finishSetupItemsThread()
{
	BW_GUARD;

	MF_ASSERT( pSetupItemsTask_ );

	pSetupItemsTask_->results(
				itemIndex_, numItems_, numTris_, numPrimitives_ );

	pSetupItemsTask_ = NULL;

	finishSetupItems();
}


/**
 *	This method calculates and returns the current sorting function.
 *
 *	@return		Sort class to use when sorting items.
 */
ItemInfoDB::ComparerPtr SceneBrowserList::currentComparer() const
{
	BW_GUARD;

	ItemInfoDB::ComparerPtr pComparer;

	if (columnStates_.sorting() == -1)
	{
		pComparer = columnStates_.sortingType().comparer( true );
	}
	else if (columnStates_.sorting() == 1)
	{
		pComparer = columnStates_.sortingType().comparer( false );
	}

	return pComparer;
}


/**
 *	This method simply forces a redraw of the window.
 */
void SceneBrowserList::redraw()
{
	BW_GUARD;

	// Clean the overflow map to get rid of any non-visible items in it.
	itemTextOverflow_.clear();

	RedrawWindow( NULL, NULL, RDW_INVALIDATE );
}
BW_END_NAMESPACE

