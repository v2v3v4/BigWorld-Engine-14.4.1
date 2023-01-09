
#include "pch.hpp"
#include "item_info_db.hpp"
#include "cstdmf/date_time_utils.hpp"
#include "cstdmf/dogwatch.hpp"
#include "chunk/chunk.hpp"
#include <algorithm>

BW_BEGIN_NAMESPACE

namespace
{
	// This constant helps in detecting a "lock" without an "unlock".
	const int MAX_LOCK_COUNT = 16;
	const uint64 MIN_MILLIS_BETWEEN_TICK_REQUESTS = 500;


	/**
	 *	This class is used to grab property information from chunk items.
	 */
	class ItemInfoDBEditor : public GeneralEditor
	{
	public:
		ItemInfoDBEditor() :
			addingAssetMetadata_( false )
		{
		}

		virtual void addingAssetMetadata( bool assetMetadata )
		{
			addingAssetMetadata_ = assetMetadata;
		}

		virtual bool useFullDateFormat()
		{
			return true;
		}


		/**
		 *	This method getsa chunk item's property and adds it to the internal
		 *	ItemInfoDB properties array.
		 */
		virtual void addProperty( GeneralProperty * pProp )
		{
			BW_GUARD;

			// Add to the base class so it deletes it later.
			properties_.push_back( pProp );

			const ValueType & valueType = pProp->valueType();
			if (valueType.isValid())
			{
				if (PyErr_Occurred())
				{
					PyErr_Clear();
				}
				PyObject * pVal = pProp->pyGet();
				if (pVal)
				{
					BW::string strVal;
					if (valueType.toString( pVal, strVal ))
					{
						BW::string name;
						if (addingAssetMetadata_)
						{
							// Storing localised string on a static for speed.
							static BW::string s_assetPrefix =
								LocaliseUTF8( L"WORLDEDITOR/WORLDEDITOR/SCENE_BROWSER/ASSET_MD_PREFIX" ) + " ";
							name = s_assetPrefix + pProp->name().c_str();
						}
						else
						{
							name = pProp->name().c_str();
						}
						dbProperties_.push_back(
							new ItemInfoDB::PropertyPair(
								ItemInfoDB::Type( name, valueType ),
								strVal ) );
					}

					Py_DECREF( pVal );
				}
			}
		}


		/**
		 *	This method returns the db properties gathered for the chunk item.
		 */
		const ItemInfoDB::ItemProperties & infoDbProperties()
		{
			return dbProperties_;
		}

	private:
		ItemInfoDB::ItemProperties dbProperties_;
		bool addingAssetMetadata_;
	};


	/**
	 *	This class compares two ItemInfoDB items by asset name.
	 */
	class AssetNameComparer : public ItemInfoDB::ComparerHelper
	{
	public:
		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			int cmp = strcmpi( pItem1->assetName().c_str(), pItem2->assetName().c_str() );
			if (cmp == 0)
			{
				// same name, sort by chunk
				cmp = pItem1->chunkId().compare( pItem2->chunkId() );
			}
			return cmp;
		}
	};


	/**
	 *	This class compares two ItemInfoDB items by chunk id.
	 */
	class ChunkIdComparer : public ItemInfoDB::ComparerHelper
	{
	public:
		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			int cmp = pItem1->chunkId().compare( pItem2->chunkId() );
			if (cmp == 0)
			{
				// same chunk, sort by file path
				cmp = strcmpi( pItem1->filePath().c_str(), pItem2->filePath().c_str() );
			}
			return cmp;
		}
	};


	/**
	 *	This class compares two ItemInfoDB items by asset type.
	 */
	class AssetTypeComparer : public ItemInfoDB::ComparerHelper
	{
	public:
		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			int cmp = pItem1->assetType().compare( pItem2->assetType() );
			if (cmp == 0)
			{
				// same assetType, sort by file path
				cmp = strcmpi( pItem1->filePath().c_str(), pItem2->filePath().c_str() );
			}
			return cmp;
		}
	};


	/**
	 *	This class compares two ItemInfoDB items by file path.
	 */
	class FilePathComparer : public ItemInfoDB::ComparerHelper
	{
	public:
		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			return strcmpi( pItem1->filePath().c_str(), pItem2->filePath().c_str() );
		}
	};


	/**
	 *	This class compares two ItemInfoDB items by an int property.
	 */
	class ItemIntComparer : public ItemInfoDB::ComparerHelper
	{
	public:
		enum INT_TYPE
		{
			NUMTRIS,
			NUMPRIMS
		};


		ItemIntComparer( const ItemInfoDB::Type & type ) : type_( type )
		{
			if (type_ == ItemInfoDB::Type::builtinType( ItemInfoDB::Type::TYPEID_NUMTRIS ))
			{
				intType_ = NUMTRIS;
			}
			else if (type_ == ItemInfoDB::Type::builtinType( ItemInfoDB::Type::TYPEID_NUMPRIMS ))
			{
				intType_ = NUMPRIMS;
			}
		}


		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			if (intType_ == NUMTRIS)
			{
				return pItem1->numTris() - pItem2->numTris();
			}
			else if (intType_ == NUMPRIMS)
			{
				return pItem1->numPrimitives() - pItem2->numPrimitives();
			}
			else
			{
				return	atoi( pItem1->propertyAsString( type_ ).c_str() ) -
						atoi( pItem2->propertyAsString( type_ ).c_str() );
			}
		}

	private:
		ItemInfoDB::Type type_;
		INT_TYPE intType_;
	};


	/**
	 *	This class compares two ItemInfoDB items by a float property.
	 */
	class ItemFloatComparer : public ItemInfoDB::ComparerHelper
	{
	public:
		ItemFloatComparer( const ItemInfoDB::Type & type ) : type_( type ) {}

		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			float a = (float)atof( pItem1->propertyAsString( type_ ).c_str() );
			float b = (float)atof( pItem2->propertyAsString( type_ ).c_str() );
			if (almostEqual( a , b ))
			{
				return 0;
			}
			else
			{
				return (a < b) ? -1 : 1;
			}
		}

	private:
		ItemInfoDB::Type type_;
	};


	/**
	 *	This class compares two ItemInfoDB items by a string property.
	 */
	class ItemStringComparer : public ItemInfoDB::ComparerHelper
	{
	public:
		ItemStringComparer( const ItemInfoDB::Type & type ) : type_( type ) {}

		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			return strcmpi( pItem1->propertyAsString( type_ ).c_str(),
				pItem2->propertyAsString( type_ ).c_str() );
		}

	private:
		ItemInfoDB::Type type_;
	};


	/**
	 *	This class compares two ItemInfoDB items by a date property. Format
	 *	must match "hh:mm:ss, dd/mm/yyyy".
	 */
	class DateStringComparer : public ItemInfoDB::ComparerHelper
	{
		typedef BW::map< ItemInfoDB::Item *, uint64 > ItemDateMap;

	public:
		DateStringComparer( const ItemInfoDB::Type & type ) : type_( type ) {}

		int compare( const ItemInfoDB::ItemPtr & pItem1,
							const ItemInfoDB::ItemPtr & pItem2 ) const
		{
			// By default sort from newest to oldest
			return int( cachedItemDateIndex( pItem2 ) - cachedItemDateIndex( pItem1 ) );
		}

	private:
		ItemInfoDB::Type type_;
		mutable ItemDateMap itemDateMap_;


		uint64 cachedItemDateIndex( const ItemInfoDB::ItemPtr & pItem ) const
		{
			uint64 dateIndex = 0;

			ItemDateMap::iterator it = itemDateMap_.find( pItem.get() );
			if (it != itemDateMap_.end())
			{
				dateIndex = (*it).second;
			}
			else
			{
				BW::string date = pItem->propertyAsString( type_ );

				DateTimeUtils::DateTime itemTime;
				if (DateTimeUtils::parse( itemTime, date ))
				{
					static const uint64 SECONDS_PER_MINUTE	= 60;
					static const uint64 SECONDS_PER_HOUR	= 60 * SECONDS_PER_MINUTE;
					static const uint64 SECONDS_PER_DAY		= 24 * SECONDS_PER_HOUR;
					static const uint64 SECONDS_PER_MONTH	= 31 * SECONDS_PER_DAY;
					static const uint64 SECONDS_PER_YEAR	= 12 * SECONDS_PER_MONTH;

					dateIndex =
						itemTime.year	* SECONDS_PER_YEAR +
						itemTime.month	* SECONDS_PER_MONTH +
						itemTime.day	* SECONDS_PER_DAY +
						itemTime.hour	* SECONDS_PER_HOUR +
						itemTime.minute	* SECONDS_PER_MINUTE +
						itemTime.second;

					itemDateMap_.insert(
							std::make_pair( pItem.get(), dateIndex ) ).first;
				}
			}
			return dateIndex;
		}
	};


} // anonymous namespace


///////////////////////////////////////////////////////////////////////////////
// Section: ItemInfoDB::Type
///////////////////////////////////////////////////////////////////////////////

/**
 *	Constructor
 *
 *	@param name			Name of the type
 *	@param valueType	Kind of this type, whether its a bool, int, float, etc.
 */
ItemInfoDB::Type::Type( const BW::string & name,
						const ValueType & valueType ) :
	name_( name ),
	valueType_( valueType )
{
}


/**
 *	This method returns the appropriate compare function for the type.
 *
 *	@param ascending	True if ascending sorting is desired, false otherwise.
 *	@return				Comapare class that is appropriate for the type.
 */
ItemInfoDB::ComparerPtr ItemInfoDB::Type::comparer( bool ascending ) const
{
	BW_GUARD;

	// Special sorting for some types.
	if (*this == builtinType( TYPEID_ASSETNAME ))
	{
		return new Comparer( ascending, new AssetNameComparer() );
	}
	else if (*this == builtinType( TYPEID_CHUNKID ))
	{
		return new Comparer( ascending, new ChunkIdComparer() );
	}
	else if (*this == builtinType( TYPEID_ASSETTYPE ))
	{
		return new Comparer( ascending, new AssetTypeComparer() );
	}
	else if (*this == builtinType( TYPEID_FILEPATH ))
	{
		return new Comparer( ascending, new FilePathComparer() );
	}

	// Generic sorting
	if (this->valueType_.desc() == ValueTypeDesc::INT)
	{
		return new Comparer( ascending, new ItemIntComparer( *this ) );
	}
	else if (this->valueType_.desc() == ValueTypeDesc::FLOAT)
	{
		return new Comparer( ascending, new ItemFloatComparer( *this ) );
	}
	else if (this->valueType_.desc() == ValueTypeDesc::DATE_STRING)
	{
		return new Comparer( ascending, new DateStringComparer( *this ) );
	}

	// By default, sort by the property's string representation.
	return new Comparer( ascending, new ItemStringComparer( *this ) );
}


/**
 *	This method returns the Type corresponding to a built-in type id.
 *
 *	@param typeId	Built-in type id.
 *	@return		Type object that corresponds to this typeId.
 */
/*static*/
const ItemInfoDB::Type & ItemInfoDB::Type::builtinType( BuiltinTypeId typeId )
{
	if (typeId == TYPEID_ASSETNAME)
	{
		static Type s_assetName( "builtin_assetName", ValueTypeDesc::STRING );
		return s_assetName;
	}
	else if (typeId == TYPEID_CHUNKID)
	{
		static Type s_chunkId( "builtin_chunkId", ValueTypeDesc::STRING );
		return s_chunkId;
	}
	else if (typeId == TYPEID_NUMTRIS)
	{
		static Type s_numTris( "builtin_numTris", ValueTypeDesc::INT );
		return s_numTris;
	}
	else if (typeId == TYPEID_NUMPRIMS)
	{
		static Type s_numPrims( "builtin_numPrims", ValueTypeDesc::INT );
		return s_numPrims;
	}
	else if (typeId == TYPEID_ASSETTYPE)
	{
		static Type s_assetType( "builtin_assetType", ValueTypeDesc::STRING );
		return s_assetType;
	}
	else if (typeId == TYPEID_FILEPATH)
	{
		static Type s_filePath( "builtin_filePath", ValueTypeDesc::FILEPATH );
		return s_filePath;
	}
	else if (typeId == TYPEID_HIDDEN)
	{
		static Type s_hidden( "builtin_hidden", ValueTypeDesc::BOOL );
		return s_hidden;
	}
	else if (typeId == TYPEID_FROZEN)
	{
		static Type s_frozen( "builtin_frozen", ValueTypeDesc::BOOL );
		return s_frozen;
	}
	else if (typeId == TYPEID_TEAMID)
	{
		static Type s_teamid( "teamID", ValueTypeDesc::INT );
		return s_teamid;
	}


	ERROR_MSG( "SceneBrowser: Invalid BuiltinTypeId from 'builtinType'\n" );
	static Type s_empty;
	return s_empty;
}


/**
 *	This method creates an ItemInfoDB Type from a data section.
 *
 *	@param pDS		Input data section
 *	@return		True if the datasection was valid, false otherwise.
 */
bool ItemInfoDB::Type::fromDataSection( DataSectionPtr pDS )
{
	BW_GUARD;

	if (!pDS)
	{
		return false;
	}

	BW::string typeName = pDS->readString( "typeName" );
	BW::string typeDesc = pDS->readString( "type" );

	if (typeName.empty() || typeDesc.empty())
	{
		return false;
	}

	if (!typeName.empty() && typeName.substr( 0, 1 ) == "`")
	{
		typeName = LocaliseUTF8( typeName.c_str() );
	}

	ValueType valueType = ValueType::fromSectionName( typeDesc );
	if (!valueType.isValid())
	{
		return false;
	}
	name_ = typeName;
	valueType_ = valueType;
	return true;
}


/**
 *	This method stores an ItemInfoDB Type into a data section.
 *
 *	@param pDS	Input data section
 *	@return		True if the datasection was saved, false otherwise.
 */
bool ItemInfoDB::Type::toDataSection( DataSectionPtr pDS ) const
{
	BW_GUARD;

	if (!pDS)
	{
		return false;
	}

	if (name_.empty() || !valueType_.isValid())
	{
		return false;
	}

	pDS->writeString( "typeName", name_ );
	pDS->writeString( "type", valueType_.toSectionName() );

	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Section: ItemInfoDB::Item
///////////////////////////////////////////////////////////////////////////////


/**
 *	Constructor
 *
 *	@param chunkId			Chunk id for the chunk item.
 *	@param pChunkItem		Chunk item pointer.
 *	@param numTris			Number of in-game triangles in the item.
 *	@param numPrimitives	Number of in-game primnitive groups for the item.
 *	@param assetName		Asset name.
 *	@param assetType		Asset type.
 *	@param filePath			Full path for the asset's file.
 *	@param properties		Map of the item's properties names and values.
 */
ItemInfoDB::Item::Item( const BW::string & chunkId, ChunkItem * pChunkItem,
	int numTris, int numPrimitives, const BW::string & assetName,
	const BW::string & assetType, const BW::string & filePath,
	const ItemProperties & properties ):
	chunkId_( chunkId ),
	pChunkItem_( pChunkItem ),
	numTris_( numTris ),
	numPrimitives_( numPrimitives ),
	assetName_( assetName ),
	assetType_( assetType ),
	filePath_( filePath ),
	hidden_( (pChunkItem && pChunkItem->edHidden()) ),
	frozen_( (pChunkItem && pChunkItem->edFrozen()) ),
	properties_( properties )
{
	BW_GUARD;

	// Helper value types to aid in conversion to string.
	static ValueType s_valueTypeBool( ValueTypeDesc::BOOL );
	static ValueType s_valueTypeInt( ValueTypeDesc::INT );
}


/**
 *	Copy constructor.
 */
ItemInfoDB::Item::Item( const Item & other )
{
	*this = other;
}


/**
 *	Destructor
 */
ItemInfoDB::Item::~Item()
{
	BW_GUARD;

	clearItemProps( properties_ );
}


/**
 *	This method retrieves a property as a string from an item.
 *
 *	@param type		Type for the desired property
 *	@return		The property matching the type, or "" if non found.
 */
BW::string ItemInfoDB::Item::propertyAsString( const Type & type ) const
{
	static ValueType s_valueTypeBool( ValueTypeDesc::BOOL );
	static ValueType s_valueTypeInt( ValueTypeDesc::INT );

	BW::string property;
	if (type == Type::builtinType( Type::TYPEID_ASSETNAME ))
	{
		return assetName_.getString();
	}
	else if (type == Type::builtinType( Type::TYPEID_CHUNKID ))
	{
		return chunkId_.getString();
	}
	else if (type == Type::builtinType( Type::TYPEID_NUMTRIS ))
	{
		s_valueTypeInt.toString( numTris_, property );
	}
	else if (type == Type::builtinType( Type::TYPEID_NUMPRIMS ))
	{
		s_valueTypeInt.toString( numPrimitives_, property );
	}
	else if (type == Type::builtinType( Type::TYPEID_ASSETTYPE ))
	{
		return assetType_.getString();
	}
	else if (type == Type::builtinType( Type::TYPEID_FILEPATH ))
	{
		return filePath_.getString();
	}
	else if (type == Type::builtinType( Type::TYPEID_HIDDEN ))
	{
		s_valueTypeBool.toString( hidden_, property );
	}
	else if (type == Type::builtinType( Type::TYPEID_FROZEN ))
	{
		s_valueTypeBool.toString( frozen_, property );
	}
	else
	{
		for (ItemProperties::iterator it = properties_.begin();
			it != properties_.end(); ++it)
		{
			if (type == (*it)->first)
			{
				return (*it)->second;
			}
		}
	}

	// Unknown property requested, which it's usualy some other asset's Type
	// so it's OK to ignore.
	return property;
}


/**
 *	Assignment operator.
 *
 *	@param other	Other item.
 *	@return		This item.
 */
const ItemInfoDB::Item & ItemInfoDB::Item::operator=( const Item & other )
{
	BW_GUARD;

	if (this != &other)
	{
		chunkId_ = other.chunkId_;
		pChunkItem_ = other.pChunkItem_;
		numTris_ = other.numTris_;
		numPrimitives_ = other.numPrimitives_;
		assetName_ = other.assetName_;
		assetType_ = other.assetType_;
		filePath_ = other.filePath_;
		hidden_ = other.hidden_;
		frozen_ = other.frozen_;

		clearItemProps( properties_ );
		for (ItemProperties::const_iterator it = other.properties_.begin();
			it != other.properties_.end(); ++it)
		{
			properties_.push_back(
				new PropertyPair( (*it)->first, (*it)->second ) );
		}
	}

	return *this;
}


/**
 *	This method deletes the properties and clears the vector passed in.
 *
 *	@param props	Properties.
 */
void ItemInfoDB::Item::clearItemProps( ItemProperties & props )
{
	BW_GUARD;

	for (ItemProperties::iterator it = props.begin(); it != props.end(); ++it)
	{
		bw_safe_delete(*it);
	}
	props.clear();
}


///////////////////////////////////////////////////////////////////////////////
// Section: ItemInfoDB
///////////////////////////////////////////////////////////////////////////////

BW_SINGLETON_STORAGE( ItemInfoDB );


/**
 *	Constructor
 */
ItemInfoDB::ItemInfoDB() :
	lockCount_( 0 ),
	hasChanged_( true ),
	typesChanged_( true ),
	numNeedsTick_( 0 ),
	numPending_( 0 ),
	onModifyCallback_( NULL ),
	onDeleteCallback_( NULL )
{
	BW_GUARD;

	int n = std::numeric_limits<int>::max();

	onModifyCallback_ =
		new BWFunctor1< ItemInfoDB, EditorChunkItem * >( this, &ItemInfoDB::itemModified );
	EditorChunkItem::addOnModifyCallback( onModifyCallback_ );

	onDeleteCallback_ =
		new BWFunctor1< ItemInfoDB, EditorChunkItem * >( this, &ItemInfoDB::itemDeleted );
	EditorChunkItem::addOnDeleteCallback( onDeleteCallback_ );

	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_ASSETNAME ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_CHUNKID ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_NUMTRIS ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_NUMPRIMS ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_ASSETTYPE ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_FILEPATH ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_HIDDEN ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_FROZEN ), n ) );
	knownTypes_.insert( TypeUsagePair(
							Type::builtinType( Type::TYPEID_TEAMID ), n ) );
}


/**
 *	Destructor
 */
ItemInfoDB::~ItemInfoDB()
{
	BW_GUARD;

	// Since the EditorChunkItem is the only one that keeps a reference to
	// these callbacks, removing them from EditorChunkItem deletes them.
	EditorChunkItem::delOnDeleteCallback( onDeleteCallback_ );
	EditorChunkItem::delOnModifyCallback( onModifyCallback_ );
}


/**
 *	This method is called back whenever a chunk item is modified, so we can
 *	tell the database to update the info on it.
 *
 *	@param	pItem	Chunk item object whose properties were modified.
 */
void ItemInfoDB::itemModified( EditorChunkItem * pItem )
{
	BW_GUARD;

	if (pItem)
	{
		this->toss( static_cast< ChunkItem * >( pItem ), true, false, true );
	}
}


/**
 *	This method is called from the EditorChunkItem destructor, to make sure we
 *	don't have pointer holding onto deleted items.
 *
 *	@param	pItem	Chunk item object about to be destroyed.
 */
void ItemInfoDB::itemDeleted( EditorChunkItem * pItem )
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	PendingChunkItemItersMap::iterator it =
		pendingChunkItemsIters_.find( static_cast< ChunkItem * >( pItem ) );

	if (it != pendingChunkItemsIters_.end())
	{
		pendingChunkItems_.erase( (*it).second );
		pendingChunkItemsIters_.erase( it );
	}
}


/**
 *	This method updates the database with items that have been tossed in or 
 *	out.
 */
void ItemInfoDB::tick( int maxMillis )
{
	BW_GUARD;

	static DogWatch dw( "ItemInfoDB_pending" );
	ScopedDogWatch sdw( dw );

	if (maxMillis == 0)
	{
		// Don't even try to grab the mutex.
		return;
	}

	uint64 maxPendingTickTime = maxMillis * stampsPerSecond() / 1000;
	uint64 endTime = timestamp() + maxPendingTickTime;

	SimpleMutexHolder smh( mutex_ );

	ChunkItemList::iterator itChunkItem = pendingChunkItems_.begin();
	while (itChunkItem != pendingChunkItems_.end() && timestamp() < endTime)
	{
		ChunkItem * pChunkItem = *itChunkItem;
		if (pChunkItem->chunk() && pChunkItem->chunk()->isBound())
		{
			itChunkItem = pendingChunkItems_.erase( itChunkItem );
			pendingChunkItemsIters_.erase( pChunkItem );
		}
		else
		{
			++itChunkItem;
			continue;
		}

		ItemInfoDBEditor * pEditor = new ItemInfoDBEditor;

		bool oldCreateViews = GeneralEditor::createViews();
		GeneralEditor::createViews( false ); // quicker adding of properties.

		pChunkItem->edEdit( *pEditor );

		GeneralEditor::createViews( oldCreateViews );

		ItemPtr pItem = new ItemInfoDB::Item(
			pChunkItem->chunk()->identifier(), pChunkItem,
			pChunkItem->edNumTriangles(), pChunkItem->edNumPrimitives(),
			pChunkItem->edAssetName(),
			pChunkItem->edClassName().c_str(), pChunkItem->edFilePath(),
			pEditor->infoDbProperties() );

		Py_DECREF( pEditor );
		pEditor = NULL;

		if (numNeedsTick_ > 0)
		{
			--numNeedsTick_;
		}

		ChunkItemsMap::iterator itChunkItemMap =
										chunkItems_.find( pChunkItem );

		if (itChunkItemMap == chunkItems_.end())
		{
			chunkItems_.insert( std::make_pair( pChunkItem, pItem ) );
			ItemList::iterator itInserted = items_.insert( items_.end(), pItem );
			if (itInserted != items_.end() && (*itInserted) == pItem)
			{
				chunkItemIters_.insert( std::make_pair( pChunkItem, itInserted ) );
			}
			else
			{
				ERROR_MSG( "ItemInfoDB::tick: inconsistent internal state "
							"(itInserted iterator and pItem mismatch).\n" );
			}
			updateItemType( pItem, +1 );
		}
		else
		{
			*((*itChunkItemMap).second) = *pItem;
			updateItemType( pItem, 0 );
		}
		hasChanged_ = true;
	}

	numPending_ = static_cast<uint32>(pendingChunkItems_.size());
}


/**
 *	This method is called by chunk items to report when they are being tossed
 *	in or out of a chunk.
 *
 *	@param pChunkItem		Chunk item.
 *	@param tossingIn		True if the item is being tossed into a chunk,
 *							false if it's being tossed out of a chunk.
 *	@param ignoreExistence	If the item is not in the database, add a new one.
 *	@param highPriority		If true, the item will be processed ASAP.
 */
void ItemInfoDB::toss( ChunkItem * pChunkItem, bool tossingIn,
			bool ignoreExistence /*= true*/, bool highPriority /*= false*/ )
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	PendingChunkItemItersMap::iterator it =
									pendingChunkItemsIters_.find( pChunkItem );

	if (tossingIn)
	{
		bool needsToss = false;
		if (it == pendingChunkItemsIters_.end())
		{
			if (ignoreExistence)
			{
				needsToss = true;
			}
			else
			{
				ChunkItemsMap::iterator itChunkItem =
										chunkItems_.find( pChunkItem );

				if (itChunkItem != chunkItems_.end())
				{
					// !ignoreExistence and item already exists, so update.
					needsToss = true;
				}
			}
		}
		if (needsToss)
		{
			ChunkItemList::iterator itWhereToInsert;
			if (highPriority)
			{
				itWhereToInsert = pendingChunkItems_.begin();
				++numNeedsTick_;
			}
			else
			{
				itWhereToInsert = pendingChunkItems_.end();
			}
			ChunkItemList::iterator itInserted =
					pendingChunkItems_.insert( itWhereToInsert, pChunkItem );
			pendingChunkItemsIters_.insert(
									std::make_pair( pChunkItem, itInserted ) );
		}
	}
	else
	{
		if (it != pendingChunkItemsIters_.end())
		{
			pendingChunkItems_.erase( (*it).second );
			pendingChunkItemsIters_.erase( it );
		}

		ChunkItemsMap::iterator itChunkItem = chunkItems_.find( pChunkItem );
		if (itChunkItem != chunkItems_.end())
		{
			updateItemType( (*itChunkItem).second, -1 );
			ChunkItemItersMap::iterator itChunkItemIter =
											chunkItemIters_.find( pChunkItem );
			if (itChunkItemIter != chunkItemIters_.end())
			{
				items_.erase( (*itChunkItemIter).second );
				chunkItemIters_.erase( itChunkItemIter );
			}
			else
			{
				// This should never happen.
				ERROR_MSG( "ItemInfoDB::toss: inconsistent internal state "
							"(missing item in chunkItemIters)\n" );

				// Erase item the slow way.
				for (ItemList::iterator itItem = items_.begin();
					itItem != items_.end(); ++itItem)
				{
					if (pChunkItem == (*itItem)->chunkItem())
					{
						items_.erase( itItem );
						break;
					}
				}
			}
			chunkItems_.erase( itChunkItem );
			hasChanged_ = true;
		}
	}
}


/**
 *	This method returns a list of the current known types. Types get added to
 *	the list of known types as new types are registered by chunk items.
 *
 *	@param retKnownTypes		Returns here the map of known types
 */
void ItemInfoDB::knownTypes( TypeUsage & retKnownTypes ) const
{
	SimpleMutexHolder smh( mutex_ );

	retKnownTypes = knownTypes_;
}


/**
 *	This method returns the asset type that added the type, or the empty string
 *	if the type has none or multiple owners.
 *
 *	@param type		Type to search for.
 *	@return		Asset type if there's only one owner, empty string otherwise.
 */
const BW::string ItemInfoDB::typeOnwer( const Type & type ) const
{
	BW_GUARD;

	SimpleMutexHolder smh( mutex_ );

	TypeOwner::const_iterator itOwner = knownTypeOwners_.find( type );
	if (itOwner != knownTypeOwners_.end())
	{
		return (*itOwner).second;
	}

	return "";
}


/**
 *	This method returns the current list of registered items, containing Chunk
 *	Item properties.
 *
 *	@param retItems		Returns here the list of ItemInfoDB items.
 */
void ItemInfoDB::items( ItemList & retItems ) const
{
	SimpleMutexHolder smh( mutex_ );

	retItems = items_;
}


/**
 *	This method returns a map of the the current list of registered items
 *	indexed by ChunkItem* (ChunkItem*,ItemPtr)
 *
 *	@param retChunkItems	Returns here the (ChunkItem*,ItemPtr) map.
 */
void ItemInfoDB::chunkItemsMap( ChunkItemsMap & retChunkItems ) const
{
	SimpleMutexHolder smh( mutex_ );

	retChunkItems = chunkItems_;
}


/**
 *	This method grabs the internal mutex in order to allow direct access to
 *	the class' big containers, such as items_ and chunkItems_, which is faster
 *	than the copy method of the items(...) and chunkItemsMap( ... ) methods.
 *	IMPORTANT:
 *		- These methods only work in the main thread.
 *		- You must match calls to lock 1:1 with calls to unlock
 */
void ItemInfoDB::lock() const
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	MF_ASSERT( lockCount_ < MAX_LOCK_COUNT );

	++lockCount_;

	mutex_.grab();
}


/**
 *	This method releases the internal mutex previously grabbed to allow direct
 *	access to the class' big containers, such as items_ and chunkItems_, which
 *	is faster than the copy method of the items(...) and chunkItemsMap( ... )
 *	methods.
 *	IMPORTANT:
 *		- These methods only work in the main thread.
 *		- You must match calls to lock 1:1 with calls to unlock
 */
void ItemInfoDB::unlock() const
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	MF_ASSERT( lockCount_ > 0 );

	--lockCount_;

	mutex_.give();
}


/**
 *	This method returns a const reference to the items_ member, which is faster
 *	than the copying done in the items(...) method.
 *	IMPORTANT:
 *		- You must call "lock" before using this method and "unlock" after you
 *		  have finished using the reference to items_.
 *		- These methods only work in the main thread.
 *
 *	@return		Const reference to the items_ list.
 */
const ItemInfoDB::ItemList & ItemInfoDB::itemsLocked() const
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	MF_ASSERT( lockCount_ > 0 );

	return items_;
}


/**
 *	This method returns a const reference to the chunkItems_ member, which is
 *	faster than the copying done in the chunkItemsMap(...) method.
 *	IMPORTANT:
 *		- You must call "lock" before using this method and "unlock" after you
 *		  have finished using the reference to items_.
 *		- These methods only work in the main thread.
 *
 *	@return		Const reference to the chunkItems_ map.
 */
const ItemInfoDB::ChunkItemsMap & ItemInfoDB::chunkItemsMapLocked() const
{
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );
	MF_ASSERT( lockCount_ > 0 );

	return chunkItems_;
}


/**
 *	This method is used internally to update the known types map when an item
 *	is added/removed from the database.
 *
 *  IMPORTANT: To call this method, you must grab the mutex.
 *
 *	@param pItem		Item info to process.
 *	@param usageAdd		+1 to add to add a ref to the type, -1 to remove a ref.
 */
void ItemInfoDB::updateItemType( ItemPtr pItem, int usageAdd )
{
	BW_GUARD;

	for (ItemProperties::const_iterator itProp = pItem->properties().begin();
		itProp != pItem->properties().end(); ++itProp)
	{
		updateType( (*itProp)->first, usageAdd,
			pItem->chunkItem()->edClassName().c_str() );
	}
}


/**
 *	This method is used internally to update a type into the known types map,
 *	when an item is added/removed from the database.
 *
 *  IMPORTANT: To call this method, you must grab the mutex.
 *
 *	@param type			Type to info to process
 *	@param usageAdd		+1 to add to add a ref to the type, -1 to remove a ref.
 *	@param owner		Class name of the owner of this type.
 */
void ItemInfoDB::updateType( const Type & type, int usageAdd,
												const BW::string & owner )
{
	BW_GUARD;

	TypeUsage::iterator itType = knownTypes_.find( type );
	if (itType != knownTypes_.end())
	{
		(*itType).second += usageAdd;
	}
	else if (usageAdd > 0)
	{
		knownTypes_.insert( TypeUsagePair( type, usageAdd ) );
		typesChanged_ = true;
	}

	TypeOwner::iterator itOwner = knownTypeOwners_.find( type );
	if (itOwner == knownTypeOwners_.end())
	{
		knownTypeOwners_.insert( TypeOwnerPair( type, owner ) );
	}
	else if (usageAdd > 0)
	{
		if (owner != (*itOwner).second)
		{
			// more than one owner, clear the existing entry
			(*itOwner).second = BW::string();
		}
	}
}

BW_END_NAMESPACE

