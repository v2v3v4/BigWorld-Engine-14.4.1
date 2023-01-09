#ifndef MYSQL_UNIQUE_ID_MAPPING_HPP
#define MYSQL_UNIQUE_ID_MAPPING_HPP

#include "property_mapping.hpp"

#include "../column_type.hpp"

#include "cstdmf/unique_id.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Maps a UniqueID into MySQL. This is a base class to properties that
 * 	stores a UniqueID into the database instead of the actual object data.
 */
class UniqueIDMapping : public PropertyMapping
{
public:
	UniqueIDMapping( const Namer & namer, const BW::string & propName,
			DataSectionPtr pDefaultValue );

	UniqueID getValue() const;

	// Overrides from PropertyMapping
	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const;

	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
				ResultStream & results,
				BinaryOStream & strm ) const;

	virtual void defaultToStream( BinaryOStream & strm ) const;

	virtual bool visitParentColumns( ColumnVisitor & visitor );

private:
	BW::string	colName_;
	UniqueID 	defaultValue_;
};

BW_END_NAMESPACE

#endif // MYSQL_UNIQUE_ID_MAPPING_HPP
