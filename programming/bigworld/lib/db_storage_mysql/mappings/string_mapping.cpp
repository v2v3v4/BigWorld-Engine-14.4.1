#include "script/first_include.hpp"

#include "string_mapping.hpp"

#include "cstdmf/debug.hpp"
#include "resmgr/datasection.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 */
StringMapping::StringMapping( const Namer & namer,
		const BW::string & propName,
		ColumnIndexType indexType,
		uint charLength,
		DataSectionPtr pDefaultValue ) :
	StringLikeMapping( namer, propName, indexType, charLength )
{
	if (pDefaultValue)
	{
		defaultValue_ = pDefaultValue->as<BW::string>();

		if (defaultValue_.size() > charLength_)
		{
			defaultValue_.resize( charLength_ );

			WARNING_MSG( "StringMapping::StringMapping: "
					"Default value (non-UTF8) for property %s has been "
					"truncated to '%s'\n",
				propName.c_str(), defaultValue_.c_str() );
		}
	}
}


/*
 *	Override from StringLikeMapping.
 */
enum_field_types StringMapping::getColumnType() const
{
	// __kyl__ (24/7/2006) Special handling of STRING < 255 characters
	// because this is how we magically pass the size of the name index
	// field. If type is not VARCHAR then index size is assumed to be
	// 255 (see createEntityTableIndex()).
	return (charLength_ < 256) ?
		MYSQL_TYPE_VAR_STRING :
		this->StringLikeMapping::getColumnType();
}

BW_END_NAMESPACE

// string_mapping.cpp
