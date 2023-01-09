
#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/debug.hpp"

#include "ual_history.hpp"


DECLARE_DEBUG_COMPONENT( 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
UalHistory::UalHistory() :
	maxItems_( 50 ),
	changedCallback_( 0 ),
	preparedItemValid_( false )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
UalHistory::~UalHistory()
{
	BW_GUARD;
}


/**
 *	This method stores the dragged item info. It is much easier to know an
 *	item's info on click, but it should only be added to the history on drop
 *	and if successful.
 *
 *	@param item		Item information.
 */
void UalHistory::prepareItem( const XmlItem& item )
{
	BW_GUARD;

	preparedItem_ = item;
	preparedItemValid_ = true;
}


/**
 *	This method adds the previously stored item to the history.
 *
 *	@return	True if there was an item prepared, false if not.
 */
bool UalHistory::addPreparedItem()
{
	BW_GUARD;

	if ( !preparedItemValid_ )
		return false;
	preparedItemValid_ = false;
	return !!add( preparedItem_ );
}


/**
 *	This method simply discards the prepared item, typically called when a drag
 *	operation is canceled or unsuccessful.
 */
void UalHistory::discardPreparedItem()
{
	BW_GUARD;

	preparedItemValid_ = false;
}


/**
 *	This method returns the previously prepared item.
 *
 *	@return	Previously prepared item, or an empty XmlItem if no item has been
 *			prepared.
 */
const XmlItem UalHistory::getPreparedItem()
{
	BW_GUARD;

	if ( preparedItemValid_ )
		return preparedItem_;
	else
		return XmlItem();
}


/**
 *	This method saves the current time to a history item data section.
 *
 *	@param ds	Data section to put the timestamp in.
 */
void UalHistory::saveTimestamp( DataSectionPtr ds )
{
	BW_GUARD;

	time_t secs;
	time( &secs );

	if ( sizeof( time_t ) == 8 )
	{
		ds->writeLong( "timestamp1", (long(secs>>32)) );
		ds->writeLong( "timestamp2", (long(secs)) );
	}
	else
	{
		ds->writeLong( "timestamp", (long)secs );
	}
}


/**
 *	This method loads a history item's timestamp from its data section.
 *
 *	@return		Timestamp retrieved from the data section.
 */
time_t UalHistory::loadTimestamp( DataSectionPtr ds )
{
	BW_GUARD;

	time_t ret;
	if ( sizeof( time_t ) == 8 )
	{
		ret =
			((time_t)ds->readLong( "timestamp1" )) << 32 |
			((time_t)ds->readLong( "timestamp2" ));
	}
	else
	{
		ret = (time_t)ds->readLong( "timestamp" );
	}
	return ret;
}


/**
 *	This method adds an item to the history.
 *
 *	@param item	Item information to add to the history.
 *	@return		Data section corresponding to the new item, or NULL on failure.
 */
DataSectionPtr UalHistory::add( const XmlItem& item )
{
	BW_GUARD;

	if ( !path_.length() || item.empty() )
		return 0;

	DataSectionPtr section = lockSection();
	if ( !section )
		return 0;

	// See if it's already in the history
	DataSectionPtr dsitem = getItem( item );
	if ( !!dsitem )
	{
		saveTimestamp( dsitem );
		section->save();
		unlockSection();
		return dsitem;
	}

	// Remove old items if the history is full
	BW::vector<DataSectionPtr> sections;
	section->openSections( "item", sections );
	while ( (int)sections.size() >= maxItems_ )
	{
		time_t oldestTime = 0;
		BW::vector<DataSectionPtr>::iterator oldestSection = sections.begin();
		for( BW::vector<DataSectionPtr>::iterator s = sections.begin(); s != sections.end(); ++s )
		{
			time_t ts = loadTimestamp( *s );
			if ( oldestTime == 0 || ts < oldestTime )
			{
				oldestTime = ts;
				oldestSection = s;
			}
		}

		section->delChild( *oldestSection );
		sections.erase( oldestSection );
	}
	section->save();

	// Add it to the history and save
	dsitem = XmlItemList::add( item );
	if ( !dsitem )
	{
		unlockSection();
		return 0;
	}
	saveTimestamp( dsitem );
	section->save();
	unlockSection();
	if ( changedCallback_ )
		(*changedCallback_)();
	return dsitem;
}


/**
 *	This method removes an item from the history.
 *
 *	@param item	Item information to remove from the history.
 *	@param callCallback True if the registered callback should be called.
 */
void UalHistory::remove( const XmlItem& item, bool callCallback )
{
	BW_GUARD;

	XmlItemList::remove( item );
	if ( callCallback && changedCallback_ )
		(*changedCallback_)();
}


/**
 *	This method removes all items from the history.
 */
void UalHistory::clear()
{
	BW_GUARD;

	XmlItemList::clear();
	if ( changedCallback_ )
		(*changedCallback_)();
}

BW_END_NAMESPACE

