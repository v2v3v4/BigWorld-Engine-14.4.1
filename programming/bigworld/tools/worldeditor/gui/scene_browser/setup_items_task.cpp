
#include "pch.hpp"
#include "setup_items_task.hpp"
#include "group_item.hpp"
#include <shlwapi.h>

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
// Section: SetupItemsTask
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor
 *
 *	@param search		Search expresion to filter items with
 *	@param groupStates	Class for managing groups' expanded/collapsed states.
 *	@param pComparer	Comparer class to use, or NULL for default sorting.
 */
SetupItemsTask::SetupItemsTask( const BW::string & search,
		const ListGroupStates & groupStates, const Types & allowedTypes,
		ItemInfoDB::ComparerPtr pComparer ) :
	groupStates_( groupStates ),
	allowedTypes_( allowedTypes ),
	pComparer_( pComparer ),
	retNumItems_( 0 ),
	retNumTris_( 0 ),
	retNumPrimitives_( 0 ),
	executed_( false )
{
	BW_GUARD;

	splitSearchInWords( search );
}


/**
 *	This method sets up the items (time-consuming operation).
 */
void SetupItemsTask::execute()
{
	BW_GUARD;

	ItemInfoDB::ItemList items;
	ItemInfoDB::instance().items( items );

	ItemInfoDB::ComparerPtr pComparer = pComparer_;
	if (!pComparer)
	{
		pComparer = ItemInfoDB::Type::builtinType(
						ItemInfoDB::Type::TYPEID_FILEPATH ).comparer( true );
	}
	if (pComparer)
	{
		items.sort( *pComparer );
	}

	executed_ = true;

	retItems_.clear();

	ItemInfoDB::Item * pPrevItem = NULL;
	GroupItem * pLastGroup = NULL;
	retNumItems_ = 0;
	retNumTris_ = 0;
	retNumPrimitives_ = 0;
	int lastGroupNumItems = 0;
	int lastGroupNumTris = 0;
	int lastGroupNumPrims = 0;
	for (ItemInfoDB::ItemList::iterator it = items.begin();
		it != items.end(); ++it)
	{
		ItemInfoDB::Item * pItem = (*it).get();
		if (searchWords_.empty() || itemInSearch( pItem ))
		{
			bool addItem = true;
			ItemInfoDB::Type groupByType = groupStates_.groupByType();
			if (groupByType.valueType().isValid())
			{
				BW::string group = pItem->propertyAsString( groupByType );
				if (!pPrevItem ||
					pPrevItem->propertyAsString( groupByType ) != group)
				{
					// It's group start, add statistics to the last group...
					if (pLastGroup)
					{
						pLastGroup->numItems( lastGroupNumItems );
						pLastGroup->numTris( lastGroupNumTris );
						pLastGroup->numPrimitives( lastGroupNumPrims );
					}

					// ...and insert a new group.
					lastGroupNumItems = 0;
					lastGroupNumTris = 0;
					lastGroupNumPrims = 0;
					pLastGroup = new GroupItem( group, groupByType );
					retItems_.push_back( pLastGroup );
				}

				// check to see if the group is expanded or not.
				if (!groupStates_.groupExpanded( group ))
				{
					addItem = false;
				}
			}
			if (addItem)
			{
				retItems_.push_back( pItem );
			}
			
			lastGroupNumItems++;
			lastGroupNumTris += pItem->numTris();
			lastGroupNumPrims += pItem->numPrimitives();
			
			pPrevItem = pItem;

			retNumItems_++;
			retNumTris_ += pItem->numTris();
			retNumPrimitives_ += pItem->numPrimitives();
		}
	}

	// It's group start, add statistics to the last group...
	if (pLastGroup)
	{
		pLastGroup->numItems( lastGroupNumItems );
		pLastGroup->numTris( lastGroupNumTris );
		pLastGroup->numPrimitives( lastGroupNumPrims );
	}
}


/**
 *	This method returns the result of setting up the items.
 *
 *	@param retItems		Receives the resulting items.
 *	@param retNumItems	Receives the resulting number of items.
 *	@param retNumTris	Receives the resulting number of triangles.
 *	@param retNumPrimitives	Receives the resulting number of primitives.
 */
void SetupItemsTask::results( Items & retItems, int & retNumItems,
									int & retNumTris, int & retNumPrimitives )
{
	BW_GUARD;

	MF_ASSERT( executed_ );

	retItems = retItems_;
	retNumItems = retNumItems_;
	retNumTris = retNumTris_;
	retNumPrimitives = retNumPrimitives_;
}


/**
 *	This method returns whether or not an item matches the search criteria.
 *
 *	@param pItem	Item to match against the search criteria
 *	@return		Whether or not an item matches the search criteria.
 */
bool SetupItemsTask::itemInSearch( ItemInfoDB::Item * pItem ) const
{
	BW_GUARD;

	bool inSearch = true;
	for (SearchWords::const_iterator it = searchWords_.begin();
		inSearch && it != searchWords_.end(); ++it)
	{
		inSearch = false;
		pItem->startProperties();
		if ((*it).second)
		{
			while (!pItem->isPropertiesEnd())
			{
				const ItemInfoDB::PropertyPair prop = pItem->nextProperty();
				const ItemInfoDB::Type & type = prop.first;
				if (!prop.second.empty() &&
					(allowedTypes_.empty() ||
						allowedTypes_.find( type ) != allowedTypes_.end()) &&
					PathMatchSpecA( prop.second.c_str(), (*it).first.c_str() ))
				{
					inSearch = true;
					break;
				}
			}
		}
		else
		{
			while (!pItem->isPropertiesEnd())
			{
				const ItemInfoDB::PropertyPair prop = pItem->nextProperty();
				const ItemInfoDB::Type & type = prop.first;
				if (!prop.second.empty() &&
					(allowedTypes_.empty() ||
						allowedTypes_.find( type ) != allowedTypes_.end()))
				{
					BW::string propStrLwr = prop.second;
					std::transform( propStrLwr.begin(), propStrLwr.end(),
												propStrLwr.begin(), tolower );

					if (propStrLwr.find( (*it).first ) != BW::string::npos)
					{
						inSearch = true;
						break;
					}
				}
			}
		}
	}
	return inSearch;
}


/**
 *	This method splits the search string in words, allowing for a search using
 *	multiple keywords.
 *
 *	@param search	Search line as typed by the user.
 */
void SetupItemsTask::splitSearchInWords( const BW::string & search )
{
	BW_GUARD;

	searchWords_.clear();
	BW::string::size_type spacePos = 0;
	BW::string::size_type nextSpacePos = 0;
	while (nextSpacePos != BW::string::npos)
	{
		if (spacePos < search.length())
		{
			nextSpacePos = search.find_first_of( ' ', spacePos );
		}
		else
		{
			nextSpacePos = BW::string::npos;
		}

		size_t wordLen = nextSpacePos - spacePos;

		BW::string word;
		if (nextSpacePos == BW::string::npos)
		{
			word = search.substr( spacePos );
		}
		else if (wordLen > 0)
		{
			word = search.substr( spacePos, wordLen );
		}

		spacePos = nextSpacePos + 1;

		if (!word.empty())
		{
			bool useWildcards = word.find( '*' ) != BW::string::npos ||
								word.find( '?' ) != BW::string::npos;

			searchWords_.push_back( SearchWord( word, useWildcards ) );
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Section: SetupItemsTaskThread
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor
 *
 *	@param search		Search expresion to filter items with
 *	@param groupStates	Class for managing groups' expanded/collapsed states.
 *	@param pComparer	Comparer class to use, or NULL for default sorting.
 */
SetupItemsBackgroundTask::SetupItemsBackgroundTask( const BW::string & search,
		const ListGroupStates & groupStates, const SetupItemsTask::Types & allowedTypes,
		ItemInfoDB::ComparerPtr pComparer ) :
	BackgroundTask( "SetupItemsBackgroundTask" ),
	task_( search, groupStates, allowedTypes, pComparer ),
	finished_(false)
{
	BW_GUARD;
}


/**
 *	This method should be called if "finished()" returns true only.
 */
void SetupItemsBackgroundTask::results( SetupItemsTask::Items & retItems, int & retNumItems,
								int & retNumTris, int & retNumPrimitives )
{
	BW_GUARD;

	task_.results( retItems, retNumItems, retNumTris, retNumPrimitives );
}



/**
 *	This method calls the base class' execute.
 */
void SetupItemsBackgroundTask::doBackgroundTask( TaskManager & mgr )
{
	BW_GUARD;

	task_.execute();
	mgr.addMainThreadTask( this );
}

/**
 * This method is called on the main thread when the task has completed
 */
void SetupItemsBackgroundTask::doMainThreadTask( TaskManager & mgr )
{
	BW_GUARD;

	finished_ = true;
}

BW_END_NAMESPACE

