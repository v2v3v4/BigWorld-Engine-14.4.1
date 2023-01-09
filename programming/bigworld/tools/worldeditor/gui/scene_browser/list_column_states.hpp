#ifndef LIST_COLUMN_STATES_HPP
#define LIST_COLUMN_STATES_HPP


#include "list_column.hpp"
#include "list_group_states.hpp"

BW_BEGIN_NAMESPACE

// Typedefs
typedef std::pair< ItemInfoDB::Type, ListColumn > ListColumnsTypePair;
typedef BW::map< ItemInfoDB::Type, ListColumn > ListColumnsTypeMap;


/**
 *	This class manages column UI states and attributes, including visibility, 
 *	order and width.
 */
class ListColumnStates
{
public:
	ListColumnStates();

	void initDefaultStates( DataSectionPtr pDS, CImageList & imgList );
	void resetDefaultStates( ListColumns & columns );
	ListColumn defaultColumn( const ItemInfoDB::Type & type );

	void groups( ListGroups & retGroups ) const;

	int sorting() const { return sorting_; }
	void sorting( int newVal ) { sorting_ = newVal; }
	ItemInfoDB::Type sortingType() const { return sortingType_; }
	void sortingType( const ItemInfoDB::Type & newVal )
													{ sortingType_ = newVal; }
	void isGroupBySorting( bool newVal ) { groupBySorting_ = newVal; }
	bool isGroupBySorting() const { return groupBySorting_; }

	void loadColumnStates( DataSectionPtr pDS );
	void saveColumnStates( DataSectionPtr pDS ) const;

	void syncColumnStates( const ListColumns & columns );
	void applyColumnStates( ListColumns & columns );

	void listPresets( DataSectionPtr pDS,
							BW::vector< BW::string > & retPresets ) const;

	bool loadPreset( DataSectionPtr pDS, const BW::string & preset );

	bool savePreset( DataSectionPtr pDS, const BW::string & preset ) const;

	bool renamePreset( DataSectionPtr pDS, const BW::string & preset,
									const BW::string & newPresetName ) const;

	bool deletePreset( DataSectionPtr pDS, const BW::string & preset ) const;

private:
	ListColumnsTypeMap columnStateMap_;		// loaded column states
	int sorting_;
	ItemInfoDB::Type sortingType_;
	bool groupBySorting_;
	ListColumnsTypeMap columnDisplayMap_;	// default displaying column info

	DataSectionPtr findPresetSection(
					DataSectionPtr pDS, const BW::string & preset ) const;

	void validateColumnOrders();
};

BW_END_NAMESPACE

#endif //  LIST_COLUMN_STATES_HPP
