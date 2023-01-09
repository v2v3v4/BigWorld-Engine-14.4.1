#ifndef MYSQL_STRING_LIKE_MAPPING_HPP
#define MYSQL_STRING_LIKE_MAPPING_HPP

#include "property_mapping.hpp"
#include "../wrapper.hpp" // For enum_field_types
#include "../column_type.hpp"


BW_BEGIN_NAMESPACE

class Namer;

class StringLikeMapping : public PropertyMapping
{
public:
	StringLikeMapping( const Namer & namer, const BW::string & propName,
				ColumnIndexType indexType, uint charLength, 
				uint byteLength = 0 );

	// Overrides from PropertyMapping
	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const;

	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
				ResultStream & results,
				BinaryOStream & strm ) const;

	virtual void defaultToStream( BinaryOStream & strm ) const;

	virtual bool visitParentColumns( ColumnVisitor & visitor );

	// New virtual method
	virtual bool isBinary() const = 0;
	virtual enum_field_types getColumnType() const;

protected:
	BW::string 			colName_;
	uint					charLength_;
	uint					byteLength_;
	BW::string				defaultValue_;
};

BW_END_NAMESPACE

#endif // MYSQL_STRING_LIKE_MAPPING_HPP
