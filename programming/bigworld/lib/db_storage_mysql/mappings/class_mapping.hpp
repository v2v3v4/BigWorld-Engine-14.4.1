#ifndef MYSQL_CLASS_MAPPING_HPP
#define MYSQL_CLASS_MAPPING_HPP

#include "composite_property_mapping.hpp"

#include "../column_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class maps CLASS and FIXED_DICT types into MySQL columns and tables.
 *	It's basically a CompositePropertyMapping with support for object being
 *	null.
 */
class ClassMapping : public CompositePropertyMapping
{
public:
	ClassMapping( const Namer & namer, const BW::string & propName,
				bool allowNone );

	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const;

	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
				ResultStream & results,
				BinaryOStream & strm ) const;

	virtual void defaultToStream( BinaryOStream & strm ) const;

	bool  isAllowNone() const			{ return allowNone_; }
	uint8 getHasProps() const			{ return hasProps_; }
	void setHasProps( uint8 hasProps )	{ hasProps_ = hasProps; }

	virtual bool visitParentColumns( ColumnVisitor & visitor );

private:
	bool		allowNone_;
	BW::string	colName_;
	uint8		hasProps_;
};

BW_END_NAMESPACE

#endif // MYSQL_CLASS_MAPPING_HPP
