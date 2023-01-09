
#include "pch.hpp"
#include "list_column_states.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor
 */
ListColumnStates::ListColumnStates() :
	sorting_( 0 ),
	groupBySorting_( false )
{
	BW_GUARD;
}


/**
 *	This method initialises the default column states.
 */
void ListColumnStates::initDefaultStates( DataSectionPtr pDS,
														CImageList & imgList )
{
	BW_GUARD;

	int hdrImgWidth = pDS->readInt( "headerImageWidth", 16 );
	int hdrImgHeight = pDS->readInt( "headerImageHeight", 13 );

	imgList.Create( hdrImgWidth, hdrImgHeight, ILC_COLOR24 | ILC_MASK, 4, 4 );
	imgList.SetBkColor( GetSysColor( COLOR_BTNFACE ) );

	BW::vector< DataSectionPtr > sections;
	pDS->openSections( "Column", sections );
	int curOrder = 0;
	for (BW::vector< DataSectionPtr >::iterator it = sections.begin();
		it != sections.end(); ++it)
	{
		ItemInfoDB::Type type;
		if (!type.fromDataSection( *it ))
		{
			continue;
		}

		int width = (*it)->readInt( "width" );
		bool allowGrouping = (*it)->readBool( "allowGrouping" );
		int order = (*it)->readInt( "order", curOrder++ );

		BW::string defaultDisplayName;
		BW::wstring imageName = (*it)->readWideString( "image" );
		int imgIdx = -1;
		if (!imageName.empty())
		{
			imageName = BWResource::instance().resolveFilenameW( imageName );
			CBitmap * img = CBitmap::FromHandle( (HBITMAP)LoadImage(
									AfxGetInstanceHandle(), imageName.c_str(),
									IMAGE_BITMAP, hdrImgWidth, hdrImgHeight,
									LR_LOADFROMFILE | LR_SHARED ) );
			if (img)
			{
				imgIdx = imgList.Add( img, 0xFF00FF /* mask is magenta */ );
			}
		}
		else
		{
			defaultDisplayName = ListColumn::defaultColName( type );
		}

		BW::string displayName =
				LocaliseUTF8( (*it)->asString( defaultDisplayName ).c_str() );

		columnDisplayMap_.insert( ListColumnsTypePair( type,
			ListColumn( displayName, width, type, allowGrouping, order, true,
						imgIdx ) ) );
	}
}


/**
 *	This method resets the columns to their default state.
 */
void ListColumnStates::resetDefaultStates( ListColumns & columns )
{
	BW_GUARD;

	sorting_ = 0;
	sortingType_ = ItemInfoDB::Type();
	groupBySorting_ = false;

	columnStateMap_.clear();

	for (ListColumnsTypeMap::const_iterator it = columnDisplayMap_.begin();
		it != columnDisplayMap_.end(); ++it)
	{
		columnStateMap_.insert( *it );
	}

	applyColumnStates( columns );
}



/**
 *	This method returns the default column setup for the type.
 */
ListColumn ListColumnStates::defaultColumn( const ItemInfoDB::Type & type )
{
	BW_GUARD;

	ListColumnsTypeMap::iterator itDispInfo =
							columnDisplayMap_.find( type );
	if (itDispInfo != columnDisplayMap_.end())
	{
		return ListColumn(
						(*itDispInfo).second.name(),
						(*itDispInfo).second.width(),
						type,
						(*itDispInfo).second.allowGrouping(),
						(*itDispInfo).second.order(),
						(*itDispInfo).second.visible(),
						(*itDispInfo).second.imageIdx() );
	}
	else
	{
		return ListColumn(
						ListColumn::defaultColName( type ),
						ListColumn::defaultColWidth( type ),
						type,
						false,
						ListColumn::defaultColOrder( type ),
						ListColumn::defaultColVisibility( type ) );
	}
}


/**
 *	This method returns the list of columns that allow grouping.
 *
 *	@param retGroups	Returns here the vector of types that allow grouping.
 */
void ListColumnStates::groups( ListGroups & retGroups ) const
{
	BW_GUARD;

	retGroups.clear();
	for (ListColumnsTypeMap::const_iterator it = columnDisplayMap_.begin();
		it != columnDisplayMap_.end(); ++it)
	{
		if ((*it).second.allowGrouping())
		{
			retGroups.push_back(
							ListGroup( (*it).second.name(), (*it).first ) );
		}
	}
}


/**
 *	This method loads the column states from a data section, and also updates
 *	the sorting type and direction.
 */
void ListColumnStates::loadColumnStates( DataSectionPtr pDS )
{
	BW_GUARD;

	groupBySorting_ = false;
	sorting_ = 0;
	sortingType_ = ItemInfoDB::Type();

	BW::vector< DataSectionPtr > states;
	pDS->openSections( "State", states );
	for (BW::vector< DataSectionPtr >::iterator it = states.begin();
		it != states.end(); ++it)
	{
		ItemInfoDB::Type type;
		if (type.fromDataSection( *it ))
		{
			ListColumnsTypeMap::iterator itCS = columnStateMap_.find( type );
			if (itCS == columnStateMap_.end())
			{
				columnStateMap_.insert( ListColumnsTypePair( type,
					ListColumn( "" /*name is ignored*/,
					(*it)->readInt( "width" ),
					type, false /*allow grouping flag is ignored*/,
					(*it)->readInt( "order" ),
					(*it)->readBool( "visible", true ) ) ) );
			}
			else
			{
				(*itCS).second.width( (*it)->readInt( "width" ) );
				(*itCS).second.order( (*it)->readInt( "order" ) );
				(*itCS).second.visible( (*it)->readBool( "visible" ) );
			}

			int sorting = (*it)->readInt( "sorting", 0 );
			if (sorting != 0)
			{
				sorting_ = sorting;
				sortingType_ = type;
				groupBySorting_ = (*it)->readBool( "isGroupBySorting", false );
			}
		}
	}
}


/**
 *	This method saves the column states from a data section, and also saves
 *	the sorting type and direction.
 */
void ListColumnStates::saveColumnStates( DataSectionPtr pDS ) const
{
	BW_GUARD;

	for (ListColumnsTypeMap::const_iterator it = columnStateMap_.begin();
		it != columnStateMap_.end(); ++it)
	{
		DataSectionPtr pStateDS = pDS->newSection( "State" );
		if ((*it).first.toDataSection( pStateDS ))
		{
			pStateDS->writeInt( "width", (*it).second.width() );
			pStateDS->writeInt( "order", (*it).second.order() );
			pStateDS->writeBool( "visible", (*it).second.visible() );
			if ((*it).first == sortingType_)
			{
				pStateDS->writeInt( "sorting", sorting_ );
				pStateDS->writeBool( "isGroupBySorting", groupBySorting_ );
			}
		}
	}
}


/**
 *	This method transfers the column states from a vector of columns into the
 *	internal state map.
 */
void ListColumnStates::syncColumnStates( const ListColumns & columns )
{
	BW_GUARD;

	for (ListColumns::const_iterator itCol = columns.begin();
		itCol != columns.end(); ++itCol)
	{
		ListColumnsTypeMap::iterator it =
									columnStateMap_.find( (*itCol).type() );

		if (it == columnStateMap_.end())
		{
			columnStateMap_.insert(
							ListColumnsTypePair( (*itCol).type(), (*itCol) ) );
		}
		else
		{
			(*it).second = (*itCol);
		}
	}

	validateColumnOrders();
}


/**
 *	This method transfers the column states into a vector of columns from the
 *	internal state map.
 */
void ListColumnStates::applyColumnStates( ListColumns & columns )
{
	BW_GUARD;

	validateColumnOrders();

	if (!columnStateMap_.empty())
	{
		for (ListColumns::iterator itCol = columns.begin();
			itCol != columns.end(); ++itCol)
		{
			ListColumnsTypeMap::const_iterator it =
										columnStateMap_.find( (*itCol).type() );
			if (it != columnStateMap_.end())
			{
				(*itCol).width( (*it).second.width() );
				(*itCol).order( (*it).second.order() );
				(*itCol).visible( (*it).second.visible() );
			}
			else
			{
				(*itCol).visible( false );
			}
		}
	}
}


/**
 *	This method returns a list of the available column layout presets.
 */
void ListColumnStates::listPresets( DataSectionPtr pDS,
							BW::vector< BW::string > & retPresets ) const
{
	BW_GUARD;

	retPresets.clear();

	BW::vector< DataSectionPtr > presets;
	pDS->openSections( "Preset", presets );
	for (BW::vector< DataSectionPtr >::iterator it = presets.begin();
		it != presets.end(); ++it)
	{
		retPresets.push_back( (*it)->asString() );
	}
}


/**
 *	This method returns loads a column layout presets into the internal column
 *	state map.
 */
bool ListColumnStates::loadPreset( DataSectionPtr pDS,
									const BW::string & preset )
{
	BW_GUARD;

	bool ret = false;

	DataSectionPtr pPresetDS = findPresetSection( pDS, preset );
	if (pPresetDS)
	{
		loadColumnStates( pPresetDS );
		ret = true;
	}

	return ret;
}


/**
 *	This method returns saves internal column state map into the preset, or
 *	creates a new preset if it doens't exist.
 */
bool ListColumnStates::savePreset( DataSectionPtr pDS,
									const BW::string & preset ) const
{
	BW_GUARD;

	bool ret = false;

	DataSectionPtr pSaveDS = findPresetSection( pDS, preset );
	if (!pSaveDS)
	{
		pSaveDS = pDS->newSection( "Preset" );
		pSaveDS->setString( preset );
	}

	if (pSaveDS)
	{
		saveColumnStates( pSaveDS );
		ret = true;
	}

	return ret;
}


/**
 *	This method renames a preset.
 */
bool ListColumnStates::renamePreset( DataSectionPtr pDS,
		const BW::string & preset, const BW::string & newPresetName ) const
{
	BW_GUARD;

	bool ret = false;

	DataSectionPtr pPresetDS = findPresetSection( pDS, preset );
	if (pPresetDS)
	{
		pPresetDS->setString( newPresetName );
		ret = true;
	}
	
	return ret;
}


/**
 *	This method deletes a preset.
 */
bool ListColumnStates::deletePreset( DataSectionPtr pDS,
											const BW::string & preset ) const
{
	BW_GUARD;

	bool ret = false;

	DataSectionPtr pPresetDS = findPresetSection( pDS, preset );
	if (pPresetDS)
	{
		pDS->delChild( pPresetDS );
		ret = true;
	}
	
	return ret;
}



/**
 *	This private method finds the child section where the desired preset is.
 */
DataSectionPtr ListColumnStates::findPresetSection(
						DataSectionPtr pDS, const BW::string & preset ) const
{
	BW_GUARD;

	BW::vector< DataSectionPtr > presets;
	pDS->openSections( "Preset", presets );
	BW::vector< DataSectionPtr >::iterator it = presets.begin();
	for ( ; it != presets.end(); ++it)
	{
		if ((*it)->asString() == preset)
		{
			break;
		}
	}

	DataSectionPtr ret;
	if (it != presets.end())
	{
		ret = (*it);
	}

	return ret;
}


/**
 *	This method ensures column order is unique, to avoid issues when sorting
 *	columns, etc. It also sets the column order counter used for adding new
 *	columns to a unique value.
 */
void ListColumnStates::validateColumnOrders()
{
	BW_GUARD;

	int maxColumnOrder = -1;

	for (ListColumnsTypeMap::iterator itCurrent = columnStateMap_.begin();
		itCurrent != columnStateMap_.end(); ++itCurrent)
	{
		bool isOrderDuplicated = false;

		int currentOrder = (*itCurrent).second.order();

		for (ListColumnsTypeMap::iterator itCheck = columnStateMap_.begin();
			itCheck != columnStateMap_.end(); ++itCheck)
		{
			if ((*itCheck).second.order() == currentOrder &&
				(*itCheck).first != (*itCurrent).first)
			{
				isOrderDuplicated = true;
				break;
			}
		}

		if (isOrderDuplicated)
		{
			// increment by 1 all order values equal or greater than the
			// duplicated one.
			for (ListColumnsTypeMap::iterator itFix = columnStateMap_.begin();
				itFix != columnStateMap_.end(); ++itFix)
			{
				if ((*itFix).second.order() >= currentOrder &&
					(*itFix).first != (*itCurrent).first)
				{
					(*itFix).second.order( (*itFix).second.order() + 1 );
				}
			}
		}

		if (currentOrder > maxColumnOrder)
		{
			maxColumnOrder = currentOrder;
		}
	}

	ListColumn::updateOrderCounter( maxColumnOrder + 1 );
}

BW_END_NAMESPACE

