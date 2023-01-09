#ifndef STRING_MAPPING_HPP
#define STRING_MAPPING_HPP

#include "string_like_mapping.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class maps the STRING type to the database.
 */
class StringMapping : public StringLikeMapping
{
public:
	/**
	 *	Constructor.
	 */
	StringMapping( const Namer & namer, const BW::string & propName,
			ColumnIndexType indexType, uint charLength, 
			DataSectionPtr pDefaultValue );

	virtual bool isBinary() const	{ return true; }
	virtual enum_field_types getColumnType() const;
};

BW_END_NAMESPACE

#endif // STRING_MAPPING_HPP
