#ifndef MYSQL_UDO_REF_MAPPING_HPP
#define MYSQL_UDO_REF_MAPPING_HPP

#include "unique_id_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class maps a UDO_REF property into the database
 */
class UDORefMapping : public UniqueIDMapping
{
public:
	UDORefMapping( const Namer & namer, const BW::string & propName,
			DataSectionPtr pDefaultValue );

private:
	static DataSectionPtr getGuidSection( DataSectionPtr pParentSection );
};

BW_END_NAMESPACE

#endif // MYSQL_UDO_REF_MAPPING_HPP
