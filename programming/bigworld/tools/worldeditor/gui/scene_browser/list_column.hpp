#ifndef LIST_COLUMN_HPP
#define LIST_COLUMN_HPP

BW_BEGIN_NAMESPACE

/**
 *	This subclass stores info for a column of the list. A column
 *	corresponds to a type of the ItemInfoDB.
 */
class ListColumn
{
	static const int INCREMENTAL_ORDER_START = 1000;

public:
	ListColumn( const BW::string & name, int width, ItemInfoDB::Type type,
			bool allowGrouping, int order, bool visible,
			int imageIdx = -1 /* no image */ );

	const BW::string & name() const { return name_; }
	int width() const { return width_; }
	void width( int value ) { width_ = value; }
	const ItemInfoDB::Type & type() const { return type_; }
	bool allowGrouping() const { return allowGrouping_; }
	int order() const { return order_; }
	void order( int value ) { order_ = value; }
	bool visible() const { return visible_; }
	void visible( bool value ) { visible_ = value; }
	int imageIdx() const { return imageIdx_; }

	inline bool operator<( const ListColumn & other ) const
	{
		return this->order_ < other.order_;
	}

	static BW::string defaultColName( const ItemInfoDB::Type & type );
	static int defaultColWidth( const ItemInfoDB::Type & type );
	static int defaultColOrder( const ItemInfoDB::Type & type );
	static bool defaultColVisibility( const ItemInfoDB::Type & type );

	static void updateOrderCounter( int order )
		{ s_incrementalOrder_ = std::max( INCREMENTAL_ORDER_START, order ); }

private:
	static int s_incrementalOrder_;

	BW::string name_;
	int width_;
	ItemInfoDB::Type type_;
	bool allowGrouping_;
	int order_;
	bool visible_;
	int imageIdx_;
};


// Typedefs
typedef BW::vector< ListColumn > ListColumns;

BW_END_NAMESPACE

#endif // LIST_COLUMN_HPP
