#ifndef MYSQL_USER_TYPE_MAPPING_HPP
#define MYSQL_USER_TYPE_MAPPING_HPP

#include "composite_property_mapping.hpp"


BW_BEGIN_NAMESPACE

class UserDataType;

/**
 *	This class maps USER_TYPE in MySQL. It's a CompositePropertyMapping with
 *	special handling for serialisation.
 */
class UserTypeMapping : public CompositePropertyMapping
{
public:
	UserTypeMapping( const BW::string & propName );

	// Overrides from PropertyMappingPtr
	virtual void fromStreamToDatabase( StreamToQueryHelper & helper,
			BinaryIStream & strm,
			QueryRunner & queryRunner ) const;

	virtual void fromDatabaseToStream( ResultToStreamHelper & helper,
				ResultStream & results,
				BinaryOStream & strm ) const;

	static PropertyMappingPtr create( const Namer & namer,
			const BW::string & propName, const UserDataType & type,
			DataSectionPtr pDefaultValue );
};

BW_END_NAMESPACE

#endif // MYSQL_USER_TYPE_MAPPING_HPP
