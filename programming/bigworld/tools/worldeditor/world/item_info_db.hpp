#ifndef ITEM_INFO_DB_HPP
#define ITEM_INFO_DB_HPP


#include "cstdmf/smartpointer.hpp"
#include "cstdmf/shared_string.hpp"
#include "cstdmf/singleton.hpp"
#include "gizmo/general_editor.hpp"
#include "gizmo/value_type.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkItem;
class ChunkItem;


/**
 *	This class keeps very lean info about chunk items without dealing with them
 *	directly.
 */
class ItemInfoDB : public Singleton< ItemInfoDB >
{
public:
	// Forward Declarations for Item class
	class Item;
	typedef SmartPointer< Item > ItemPtr;


	/**
	 *	This is the base class that implements the actual comparison to be used
	 *	in the Comparer class.
	 */
	class ComparerHelper : public ReferenceCount
	{
	public:
		virtual ~ComparerHelper() {}

		virtual int compare( const ItemPtr & a, const ItemPtr & b )
																	const = 0;
	};
	typedef SmartPointer< ComparerHelper > ComparerHelperPtr;


	/**
	 *	This is an interface to a STL-friendly item property compare class.
	 */
	class Comparer : public ReferenceCount
	{
	public:
		Comparer( bool ascending, ComparerHelperPtr pHelper ) :
			ascending_( ascending ),
			pHelper_( pHelper )
		{}
		
		bool operator() ( const ItemPtr & a, const ItemPtr & b ) const
		{
			intptr cmp = (ascending_ ? pHelper_->compare( a, b ) :
									pHelper_->compare( b, a ) );
			if (cmp == 0)
			{
				// To keep the list stable, sort by chunkItem pointer if
				// everything else fails.
				cmp = (ascending_ ?
							intptr( a->chunkItem() ) - intptr( b->chunkItem() ) :
							intptr( b->chunkItem() ) - intptr( a->chunkItem() ) );
			}
			return cmp < 0;
		}

	protected:
		bool ascending_;
		ComparerHelperPtr pHelper_;
	};
	typedef SmartPointer< Comparer > ComparerPtr;


	/**
	 *	This class helps in making decisions about how to handle a chunk item's
	 *	property. For example, the user of the ItemInfoDB might want to
	 *	handle bools in a different way than the rest (display a check box
	 *	instead of a 'True'/'False' string).
	 */
	class Type
	{
	public:

		// IDs for built-in types.
		enum BuiltinTypeId
		{
			TYPEID_ASSETNAME,
			TYPEID_CHUNKID,
			TYPEID_NUMTRIS,
			TYPEID_NUMPRIMS,
			TYPEID_ASSETTYPE,
			TYPEID_FILEPATH,
			TYPEID_HIDDEN,
			TYPEID_FROZEN,
			TYPEID_CUSTOMPROPERTY,
			TYPEID_TEAMID = TYPEID_CUSTOMPROPERTY
		};

		Type() {}
		Type( const BW::string & name, const ValueType & valueType );

		bool operator==( const Type & other ) const
		{
			return this->name_ == other.name_ &&
										this->valueType_ == other.valueType_;
		}

		bool operator!=( const Type & other ) const
		{
			return !this->operator==( other );
		}

		bool operator<( const Type & other ) const
		{
			return this->name_ < other.name_;
		}

		const BW::string & name() const { return name_; }
		const ValueType & valueType() const { return valueType_; }

		ComparerPtr comparer( bool ascending ) const;

		static const Type & builtinType( BuiltinTypeId typeId );

		bool fromDataSection( DataSectionPtr pDS );
		bool toDataSection( DataSectionPtr pDS ) const;

	private:
		BW::string name_;
		ValueType valueType_;
	};

	typedef std::pair< Type, BW::string > PropertyPair;
	typedef BW::vector< PropertyPair * > ItemProperties;
	typedef std::pair< Type, int > TypeUsagePair;
	typedef BW::map< Type, int > TypeUsage;
	typedef std::pair< Type, BW::string > TypeOwnerPair;
	typedef BW::map< Type, BW::string > TypeOwner;


	/**
	 *	This class keeps lean info about one chunk item.
	 */
	class Item : public SafeReferenceCount
	{
	public:
		Item( const BW::string & chunkId, ChunkItem * pChunkItem,
			int numTris, int numPrimitives, const BW::string & assetName, 
			const BW::string & assetType, const BW::string & filePath,
			const ItemProperties & properties );
			
		Item( const Item & other );

		virtual ~Item();

		virtual BW::string propertyAsString( const Type & type ) const;

		void startProperties() const
		{
			iterator_ = Type::TYPEID_ASSETNAME;
		}

		bool isPropertiesEnd() const
		{
			return iterator_ == 
				( Type::TYPEID_CUSTOMPROPERTY + properties_.size() );
		}

		const PropertyPair nextProperty() const
		{
			MF_ASSERT_DEBUG( !isPropertiesEnd() );
			int iterator = iterator_++;
			if (iterator < Type::TYPEID_CUSTOMPROPERTY)
			{
				PropertyPair property;
				property.first = ItemInfoDB::Type::builtinType( 
					(Type::BuiltinTypeId)iterator );
				property.second = propertyAsString( property.first );
				return property;
			}
			return *properties_[iterator - Type::TYPEID_CUSTOMPROPERTY];
		}

		const ItemProperties & properties() const { return properties_; }

		const BW::string & assetName() const { return assetName_.getString(); }
		const BW::string & filePath() const { return filePath_.getString(); }
		const BW::string & chunkId() const { return chunkId_.getString(); }
		ChunkItem * chunkItem() const { return pChunkItem_; }
		const BW::string & assetType() const { return assetType_.getString(); }
		int numTris() const { return numTris_; }
		int numPrimitives() const { return numPrimitives_; }
		bool isHidden() const { return hidden_; }
		bool isFrozen() const { return frozen_; }

		const Item & operator=( const Item & other );

	protected:
		// Basic properties
		SharedString		chunkId_;
		ChunkItem *			pChunkItem_;
		int					numTris_;
		int					numPrimitives_;
		SharedString		assetName_;
		SharedString		assetType_;
		SharedString		filePath_;
		bool				hidden_;
		bool				frozen_;

		// Generic Properties
		mutable ItemProperties		properties_;

		// Other
		mutable int			iterator_;

		// Private methods
		void clearItemProps( ItemProperties & props );
	};


	// ItemInfoDB specific declarations.

	typedef BW::list< ItemPtr > ItemList;
	typedef BW::list< ChunkItem * > ChunkItemList;
	typedef BW::map< ChunkItem *, ChunkItemList::iterator > PendingChunkItemItersMap;
	typedef BW::map< ChunkItem *, ItemList::iterator > ChunkItemItersMap;
	typedef BW::map< ChunkItem *, ItemPtr > ChunkItemsMap;


	ItemInfoDB();
	~ItemInfoDB();

	void itemModified( EditorChunkItem * pItem );
	void itemDeleted( EditorChunkItem * pItem );

	void clearChanged() { hasChanged_ = false; typesChanged_ = false; }
	bool hasChanged() const { return hasChanged_; }
	bool typesChanged() const { return typesChanged_; }
	bool needsTick() const { return numNeedsTick_ > 0; }

	void tick( int maxMillis = 15 );

	void toss( ChunkItem * pChunkItem, bool tossingIn,
					bool ignoreExistence = true, bool highPriority = false );

	void knownTypes( TypeUsage & retKnownTypes ) const;

	const BW::string typeOnwer( const Type & type ) const;

	void items( ItemList & retItems ) const;

	void chunkItemsMap( ChunkItemsMap & retChunkItems ) const;

	uint32 numPending() const { return numPending_; }

	void lock() const;
	void unlock() const;
	const ItemList & itemsLocked() const;
	const ChunkItemsMap & chunkItemsMapLocked() const;

private:
	ItemList items_;
	ChunkItemItersMap chunkItemIters_;
	ChunkItemsMap chunkItems_;
	TypeOwner knownTypeOwners_;
	TypeUsage knownTypes_;

	ChunkItemList pendingChunkItems_;
	PendingChunkItemItersMap pendingChunkItemsIters_;
	mutable SimpleMutex mutex_;
	mutable int lockCount_;

	bool hasChanged_;
	bool typesChanged_;
	int numNeedsTick_;

	uint32 numPending_;

	BWFunctor1< ItemInfoDB, EditorChunkItem * > * onModifyCallback_;
	BWFunctor1< ItemInfoDB, EditorChunkItem * > * onDeleteCallback_;
	 
	void updateItemType( ItemPtr pItem, int usageAdd );
	void updateType( const Type & type, int usageAdd,
												const BW::string & owner  );
};

BW_END_NAMESPACE

#endif // ITEM_INFO_DB_HPP
