#ifndef COLUMN_MENU_ITEM_HPP
#define COLUMN_MENU_ITEM_HPP

BW_BEGIN_NAMESPACE

/**
 *	This helper class is used for the columns popup menu, to allow sorting
 *	and grouping of column types by the asset type that created it.
 */
class ColumnMenuItem
{
public:
	ColumnMenuItem( const BW::string & item, const BW::string & prefix,
					const BW::string & owner, int colIdx );
	
	//	This method returns the asset type that created this type.
	const BW::string & owner() const { return owner_; }

	//	This method returns the type of this column.
	BW::string menuItem() const { return prefix_ + item_; }

	//	This method returns the index of this column.
	int colIdx() const { return colIdx_; }

	bool operator<( const ColumnMenuItem & other ) const;

private:
	BW::string item_;
	BW::string prefix_;
	BW::string owner_;
	int colIdx_;
};

BW_END_NAMESPACE

#endif // COLUMN_MENU_ITEM_HPP
