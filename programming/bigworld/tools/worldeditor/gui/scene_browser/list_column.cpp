
#include "pch.hpp"
#include "list_column.hpp"

BW_BEGIN_NAMESPACE

/*static*/ int ListColumn::s_incrementalOrder_ = INCREMENTAL_ORDER_START;


/**
 *	Constructor
 *
 *	@param name				Display name for the column.
 *	@param width			Initial width of the column.
 *	@param type				Item info to process.
 *	@param allowGrouping	Whether or not the column allows grouping.
 *	@param order			Order, the bigger the number the farther right.
 *	@param visible			Whether or not the column is visible.
 *	@param imageIdx			Index of the image in the image list, or -1.
 */
ListColumn::ListColumn( const BW::string & name, int width,
		ItemInfoDB::Type type, bool allowGrouping, int order, bool visible,
		int imageIdx /* = -1,  no image */ ) :
	name_( name ),
	width_( width ),
	type_( type ),
	allowGrouping_( allowGrouping ),
	order_( order ),
	visible_( visible ),
	imageIdx_( imageIdx )
{
	BW_GUARD;
}


/**
 *	This method returns the default display name for a column.
 *
 *	@param type		Column's type
 *	@return			Default display name for a column.
 */
/*static*/ BW::string ListColumn::defaultColName(
												const ItemInfoDB::Type & type )
{
	BW_GUARD;

	if (type.name().substr( 0, 8 ) == "builtin_")
	{
		return type.name().substr( 8 );
	}

	return type.name();
}


/**
 *	This method returns the default width for a column.
 *
 *	@param type		Column's type
 *	@return			Default width for a column.
 */
/*static*/ int ListColumn::defaultColWidth( const ItemInfoDB::Type & type )
{
	BW_GUARD;

	if (type.valueType().desc() == ValueTypeDesc::BOOL)
	{
		return 20;
	}
	else if (type.valueType().desc() == ValueTypeDesc::INT)
	{
		return 40;
	}
	else if (type.valueType().desc() == ValueTypeDesc::FLOAT)
	{
		return 60;
	}
	else if (type.valueType().desc() == ValueTypeDesc::STRING)
	{
		return 100;
	}
	else if (type.valueType().desc() == ValueTypeDesc::FILEPATH)
	{
		return 120;
	}

	return 100;
}


/**
 *	This method returns the default order for a column.
 *
 *	@param type		Column's type
 *	@return			Default order for a column.
 */
/*static*/ int ListColumn::defaultColOrder( const ItemInfoDB::Type & type )
{
	BW_GUARD;

	if (type == ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_ASSETNAME ))
	{
		return 0;
	}
	else if (type == ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_CHUNKID ))
	{
		return 1;
	}
	else if (type == ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_NUMTRIS ))
	{
		return 2;
	}
	else if (type == ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_NUMPRIMS ))
	{
		return 3;
	}
	else if (type == ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_ASSETTYPE ))
	{
		return 4;
	}
	else if (type == ItemInfoDB::Type::builtinType(
									ItemInfoDB::Type::TYPEID_FILEPATH ))
	{
		return 5;
	}

	return s_incrementalOrder_++;
}


/**
 *	This method returns the default visibility for a column.
 *
 *	@param type		Column's type
 *	@return			Default visibility for a column.
 */
/*static*/ bool ListColumn::defaultColVisibility(
												const ItemInfoDB::Type & type )
{
	BW_GUARD;

	// For non-default columns, don't show them by default.
	return false;
}

BW_END_NAMESPACE

