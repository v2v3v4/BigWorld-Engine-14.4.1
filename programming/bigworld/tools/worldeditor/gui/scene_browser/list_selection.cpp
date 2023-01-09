
#include "pch.hpp"
#include "list_selection.hpp"
#include "world/item_info_db.hpp"
#include "gizmo/item_view.hpp"
#include "framework/world_editor_app.hpp"
#include "scripting/we_python_adapter.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
ListSelection::ListSelection() :
	pList_( NULL ),
	needsUpdate_( false ),
	changed_( false )
{
	BW_GUARD;
}


/**
 *	This method sets a valid CListCtrl pointer, required by this class.
 */
void ListSelection::init( CListCtrl * pList )
{
	BW_GUARD;

	pList_ = pList;
}


/**
 *	This helper method removes unloaded items from the selection list.
 *
 *	@param eraseMissingItems	If true, it remove items from the selection if the
 *								item is no longer loaded.
 */
void ListSelection::filter( bool eraseMissingItems /*= true */ )
{
	BW_GUARD;

	if (ItemInfoDB::instance().hasChanged() && !selection_.empty())
	{
		// Using quicker lock/unlock mechanism
		ItemInfoDB::instance().lock();
		const ItemInfoDB::ChunkItemsMap & chunkItems =
						ItemInfoDB::instance().chunkItemsMapLocked();

		for (ItemList::iterator itSel = selection_.begin();
			itSel != selection_.end(); )
		{
			ItemInfoDB::ChunkItemsMap::const_iterator it =
									chunkItems.find( (*itSel)->chunkItem() );
			if (it == chunkItems.end())
			{
				if (eraseMissingItems)
				{
					selectionHash_.erase( (*itSel)->chunkItem() );
					itSel = selection_.erase( itSel );
					// No need to flag the selection as changed, whoever tossed
					// pSelItem must have taken care of their own selection.
				}
				else
				{
					++itSel;
				}
			}
			else
			{
				if ((*itSel) != (*it).second)
				{
					// The chunk item's db item changed, update the selection.
					(*itSel) = (*it).second;
				}
				++itSel;
			}
		}
		ItemInfoDB::instance().unlock();
	}
}


/**
 *	This helper method restores the selection in the list control from the
 *	internal vector.
 */
void ListSelection::restore( const BW::vector< ItemInfoDB::ItemPtr > & items )
{
	BW_GUARD;

	for (size_t item = 0; item < items.size(); ++item)
	{
		if (selectionHash_.find( items[ item ]->chunkItem() ) != selectionHash_.end())
		{
			pList_->SetItemState( static_cast<int>(item), LVIS_SELECTED, LVIS_SELECTED );
		}
	}
}


/**
 *	This helper method updates the selection vector from the list control.
 */
void ListSelection::update( const BW::vector< ItemInfoDB::ItemPtr > & items )
{
	BW_GUARD;

	selection_.clear();
	selectionHash_.clear();
	int numSel = pList_->GetSelectedCount();
	int selItem = -1;
	for (int i = 0; i < numSel; i++)
	{
		selItem = pList_->GetNextItem( selItem, LVNI_SELECTED );
		ItemInfoDB::ItemPtr pItem = items[ selItem ];
		if (pItem && pItem->chunkItem())
		{
			selection_.push_back( pItem );
			selectionHash_.insert( pItem->chunkItem() );
		}
	}
	changed_ = true;
}


/**
 *	This method returns the number of triangles in items selected.
 *
 *	@return			The number of triangles in items selected.
 */
int ListSelection::numSelectedTris() const
{
	BW_GUARD;

	int numTris = 0;
	for (ItemList::const_iterator it = selection_.begin();
		it != selection_.end(); ++it)
	{
		numTris += (*it)->numTris();
	}
	return numTris;
}


/**
 *	This method returns the number of primitive groups in items selected.
 *
 *	@return			The number of primitive groups in items selected.
 */
int ListSelection::numSelectedPrimitives() const
{
	BW_GUARD;

	int numPrims = 0;
	for (ItemList::const_iterator it = selection_.begin();
		it != selection_.end(); ++it)
	{
		numPrims += (*it)->numPrimitives();
	}
	return numPrims;
}


/**
 *	This method returns the selection in the list.
 *
 *	@return			List of items currently selected.
 */
const ListSelection::ItemList & ListSelection::selection( bool validate )
{
	BW_GUARD;

	if (validate)
	{
		filter();
	}

	return selection_;
}


/**
 *	This method allows setting the selection in the list
 *
 *	@param selection	List of chunk items to select
 *	@param items		Selectable items
 */
void ListSelection::selection( const BW::vector< ChunkItemPtr > & selection,
							const BW::vector< ItemInfoDB::ItemPtr > & items )
{
	BW_GUARD;

	// Check if it's the same, and if so, don't do anything.
	bool identical = false;
	
	if (selection.size() == selection_.size())
	{
		identical = true;
		for (BW::vector< ChunkItemPtr >::const_iterator it = selection.begin();
			it != selection.end(); ++it)
		{
			if (selectionHash_.find( (*it).get() ) == selectionHash_.end())
			{
				identical = false;
				break;
			}
		}
	}

	if (identical)
	{
		return;
	}

	// Set the new selection
	pList_->SetItemState( -1, 0, LVIS_SELECTED );
	selection_.clear();
	selectionHash_.clear();

	bool cursorSet = false;

	// Set the selection in the visible list
	ChunkItemHash tmpSel;
	for (BW::vector< ChunkItemPtr >::const_iterator it = selection.begin();
		it != selection.end(); ++it)
	{
		tmpSel.insert( (*it).get() );
	}

	for (size_t idx = 0; idx < items.size(); ++idx)
	{
		ChunkItemHash::iterator itSel =
									tmpSel.find( items[ idx ]->chunkItem() );

		if (itSel != tmpSel.end())
		{
			pList_->SetItemState( static_cast<int>(idx), LVIS_SELECTED, LVIS_SELECTED );
			if (!cursorSet)
			{
				pList_->SetItemState( static_cast<int>(idx), LVIS_FOCUSED, LVIS_FOCUSED );
				cursorSet = true;
			}
			selection_.push_back( items[ idx ] );
			selectionHash_.insert( items[ idx ]->chunkItem() );
			tmpSel.erase( itSel );
		}
	}

	// For remaining chunk items not in the tmpSel list, set the selection if
	// they are in the database (rare, unloaded chunks)
	if (!tmpSel.empty())
	{
		// Using quicker lock/unlock mechanism
		ItemInfoDB::instance().lock();
		const ItemInfoDB::ItemList & dbItems =
										ItemInfoDB::instance().itemsLocked();
		for (ItemInfoDB::ItemList::const_iterator it = dbItems.begin();
			it != dbItems.end(); ++it)
		{
			ChunkItemHash::iterator itSel = tmpSel.find( (*it)->chunkItem() );
			if (itSel != tmpSel.end())
			{
				selection_.push_back( *it );
				selectionHash_.insert( (*it)->chunkItem() );
			}
		}
		ItemInfoDB::instance().unlock();
	}
}


/**
 *	This method deletes the currently selected items.
 */
void ListSelection::deleteSelectedItems() const
{
	BW_GUARD;

	if (!selection_.empty())
	{
		ChunkItemRevealer::ChunkItems selItems;

		for (ItemInfoDB::ItemList::const_iterator it = selection_.begin();
			it != selection_.end(); ++it)
		{
			selItems.push_back( (*it)->chunkItem() );
		}

		ChunkItemGroupPtr group = ChunkItemGroupPtr( new ChunkItemGroup( selItems ), PyObjectPtr::STEAL_REFERENCE );
		WorldEditorApp::instance().pythonAdapter()->deleteChunkItems( group );
	}
}

BW_END_NAMESPACE

